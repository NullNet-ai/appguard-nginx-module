#include "appguard.wrapper.hpp"
#include "appguard.inner.utils.hpp"
#include "appguard.nginx.module.hpp"
#include "appguard.client.exception.hpp"

extern "C"
{

    static ngx_int_t ngx_http_appguard_handler(ngx_http_request_t *r)
    {
        return AppGuardNginxModule::Handler(r);
    }

    static ngx_int_t ngx_http_appguard_init(ngx_conf_t *cf)
    {
        return AppGuardNginxModule::Initialize(cf);
    }

    static void *ngx_http_appguard_create_srv_conf(ngx_conf_t *cf)
    {
        return AppGuardNginxModule::CreateSrvConfig(cf);
    }

    static char *ngx_http_appguard_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
    {
        return AppGuardNginxModule::MergeSrvConfig(cf, parent, child);
    }

    static ngx_command_t appguard_nginx_module_commands[] = {
        {ngx_string("appguard_enabled"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_FLAG,
         ngx_conf_set_flag_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, enabled),
         nullptr},

        {ngx_string("appguard_server_addr"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, server_addr),
         nullptr},

        ngx_null_command};

    static ngx_http_module_t appguard_nginx_module_ctx = {
        nullptr,
        ngx_http_appguard_init,

        nullptr,
        nullptr,

        ngx_http_appguard_create_srv_conf,
        ngx_http_appguard_merge_srv_conf,

        nullptr,
        nullptr};

    ngx_module_t appguard_nginx_module = {
        NGX_MODULE_V1,
        &appguard_nginx_module_ctx,
        appguard_nginx_module_commands,
        NGX_HTTP_MODULE,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        NGX_MODULE_V1_PADDING};
}

ngx_int_t AppGuardNginxModule::Initialize(ngx_conf_t *cf)
{
    auto *cmcf = static_cast<ngx_http_core_main_conf_t *>(
        ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module));

    auto *h = static_cast<ngx_http_handler_pt *>(
        ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers));

    if (h == nullptr)
        return NGX_ERROR;

    *h = ngx_http_appguard_handler;
    return NGX_OK;
}

void *AppGuardNginxModule::CreateSrvConfig(ngx_conf_t *cf)
{
    auto *conf = static_cast<Config *>(ngx_pcalloc(cf->pool, sizeof(Config)));
    if (!conf)
        return nullptr;

    conf->enabled = NGX_CONF_UNSET;

    return conf;
}

char *AppGuardNginxModule::MergeSrvConfig(ngx_conf_t *cf, void *parent, void *child)
{
    auto *prev = static_cast<AppGuardNginxModule::Config *>(parent);
    auto *conf = static_cast<AppGuardNginxModule::Config *>(child);

    ngx_conf_merge_value(conf->enabled, prev->enabled, 0);
    ngx_conf_merge_str_value(conf->server_addr, prev->server_addr, "");

    return NGX_CONF_OK;
}

ngx_int_t AppGuardNginxModule::Handler(ngx_http_request_t *request)
{
    auto *conf = static_cast<AppGuardNginxModule::Config *>(ngx_http_get_module_srv_conf(request, appguard_nginx_module));
    if (!conf || !conf->enabled)
        return NGX_DECLINED;

    try
    {
        std::string server_address(
            reinterpret_cast<char *>(conf->server_addr.data),
            conf->server_addr.len);

        auto client = AppGuardWrapper::create(server_address);

        auto connection = appguard::inner_utils::ExtractTcpConnectionInfo(request);
        auto tcp_response = client.HandleTcpConnection(connection);

        auto http_request = appguard::inner_utils::ExtractHttpRequestInfo(request);
        http_request.set_allocated_tcp_info(new appguard::AppGuardTcpInfo(tcp_response.tcp_info()));
        auto http_response = client.HandleHttpRequest(http_request);

        switch (http_response.policy())
        {
        case appguard::FirewallPolicy::ALLOW:
            return NGX_DECLINED;
        case appguard::FirewallPolicy::DENY:
            return NGX_ABORT;
        case appguard::FirewallPolicy::UNKNOWN:
            return NGX_DECLINED;
        default:
            auto log = request->connection->log;

            ngx_log_error(
                NGX_LOG_ERR,
                log, 0, "AppGuard: Unkown firewall policy: %d",
                static_cast<uint32_t>(http_response.policy()));

            return NGX_DECLINED;
        }
    }
    catch (AppGuardClientException &ex)
    {
        auto log = request->connection->log;

        ngx_log_error(
            NGX_LOG_ERR,
            log,
            0,
            "AppGuard Error: %s, Status Code: %d",
            ex.what(),
            static_cast<uint32_t>(ex.code()));

        return NGX_DECLINED;
    }
}
