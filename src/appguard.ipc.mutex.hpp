#pragma once

#include <string>

namespace appguard
{
    class IPCMutex
    {
    public:
        IPCMutex(std::string_view name);
        ~IPCMutex();

        void lock();
        void unlock();

    private:
        std::string name;
        pthread_mutex_t *mutex;
        int shm_fd;
        bool created;
    };
}