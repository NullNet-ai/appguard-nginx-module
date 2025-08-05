#include "appguard.ipc.mutex.hpp"

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>

namespace appguard
{
    IPCMutex::IPCMutex(std::string_view name)
        : name(name), created(false)
    {
        shm_fd = shm_open(name.data(), O_RDWR | O_CREAT | O_EXCL, 0666);
        if (shm_fd >= 0)
        {
            created = true;
            if (ftruncate(shm_fd, sizeof(pthread_mutex_t)) == -1)
            {
                close(shm_fd);
                shm_unlink(name.data());
                throw std::runtime_error("Failed to truncate shared memory.");
            }
        }
        else
        {
            shm_fd = shm_open(name.data(), O_RDWR, 0666);
            if (shm_fd < 0)
            {
                throw std::runtime_error("Failed to open shared memory.");
            }
        }

        mutex = static_cast<pthread_mutex_t *>(
            mmap(nullptr, sizeof(pthread_mutex_t),
                 PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

        if (mutex == MAP_FAILED)
        {
            close(shm_fd);

            if (created)
            {
                shm_unlink(name.data());
            }

            throw std::runtime_error("Failed to mmap shared memory.");
        }

        if (created)
        {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

            if (pthread_mutex_init(mutex, &attr) != 0)
            {
                munmap(mutex, sizeof(pthread_mutex_t));
                close(shm_fd);
                shm_unlink(name.data());
                pthread_mutexattr_destroy(&attr);
                throw std::runtime_error("Failed to initialize mutex.");
            }

            pthread_mutexattr_destroy(&attr);
        }
    }

    IPCMutex::~IPCMutex()
    {
        if (mutex != nullptr && mutex != MAP_FAILED)
        {
            munmap(mutex, sizeof(pthread_mutex_t));
        }

        if (shm_fd >= 0)
        {
            close(shm_fd);
        }

        if (created)
        {
            shm_unlink(name.c_str());
        }
    }

    void IPCMutex::lock()
    {
        pthread_mutex_lock(mutex);
    }

    void IPCMutex::unlock()
    {
        pthread_mutex_unlock(mutex);
    }
}