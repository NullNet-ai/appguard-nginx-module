#include "appguard.wrapper.hpp"
#include "appguard.inner.utils.hpp"
#include "appguard.nginx.module.hpp"
#include "appguard.uclient.exception.hpp"
#include "appguard.uclient.info.hpp"

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

        {ngx_string("appguard_tls"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_FLAG,
         ngx_conf_set_flag_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, tls),
         nullptr},

        {ngx_string("appguard_server_addr"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, server_addr),
         nullptr},

        {ngx_string("appguard_app_id"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, app_id),
         nullptr},

        {ngx_string("appguard_app_secret"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, app_secret),
         nullptr},

        {ngx_string("appguard_default_policy"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, default_policy),
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

static ngx_int_t ActOnPolicy(appguard::FirewallPolicy policy, appguard::FirewallPolicy default_policy)
{
    switch (policy)
    {
    case appguard::FirewallPolicy::ALLOW:
        return NGX_DECLINED;
    case appguard::FirewallPolicy::DENY:
        return NGX_HTTP_FORBIDDEN;
    default:
        return (default_policy == appguard::FirewallPolicy::ALLOW) ? NGX_DECLINED : NGX_HTTP_FORBIDDEN;
    }
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
    void *memory = ngx_pcalloc(cf->pool, sizeof(Config));
    if (!memory)
        return nullptr;

    return new (memory) Config();
}

char *AppGuardNginxModule::MergeSrvConfig(ngx_conf_t *cf, void *parent, void *child)
{
    auto *prev = static_cast<AppGuardNginxModule::Config *>(parent);
    auto *conf = static_cast<AppGuardNginxModule::Config *>(child);

    ngx_conf_merge_value(conf->enabled, prev->enabled, 0);
    ngx_conf_merge_value(conf->tls, prev->tls, 0);

    ngx_conf_merge_str_value(conf->server_addr, prev->server_addr, "");
    ngx_conf_merge_str_value(conf->app_id, prev->app_id, "");

    ngx_conf_merge_str_value(conf->app_secret, prev->app_secret, "");
    ngx_conf_merge_str_value(conf->default_policy, prev->default_policy, "");

    return NGX_CONF_OK;
}

ngx_int_t AppGuardNginxModule::Handler(ngx_http_request_t *request)
{
    auto *conf = static_cast<AppGuardNginxModule::Config *>(ngx_http_get_module_srv_conf(request, appguard_nginx_module));
    if (!conf || !conf->enabled)
        return NGX_DECLINED;

    auto default_policy_str = appguard::inner_utils::NgxStringToStdString(&conf->default_policy);
    auto default_policy = appguard::inner_utils::StringToFirewallPolicy(default_policy_str);
    auto app_id = appguard::inner_utils::NgxStringToStdString(&conf->app_id);
    auto app_secret = appguard::inner_utils::NgxStringToStdString(&conf->app_secret);
    auto server_addr = appguard::inner_utils::NgxStringToStdString(&conf->server_addr);

    if (app_id.empty() || app_secret.empty())
    {
        ngx_log_error(
            NGX_LOG_ERR,
            request->connection->log,
            0,
            "AppGuard: App Id and/or App Secret haven't been set; falling back to default policy '%s'",
            default_policy_str.data());

        return ActOnPolicy(appguard::FirewallPolicy::UNKNOWN, default_policy);
    }

    try
    {
        AppGaurdClientInfo client_info{
            .app_id = app_id,
            .app_secret = app_secret,
            .server_addr = server_addr,
            .tls = !!conf->tls};

        auto client = AppGuardWrapper::CreateClient(client_info);

        auto connection = appguard::inner_utils::ExtractTcpConnectionInfo(request);
        auto tcp_response = client.HandleTcpConnection(connection);

        auto http_request = appguard::inner_utils::ExtractHttpRequestInfo(request);
        http_request.set_allocated_tcp_info(new appguard::AppGuardTcpInfo(tcp_response.tcp_info()));
        auto http_response = client.HandleHttpRequest(http_request);

        return ActOnPolicy(http_response.policy(), default_policy);
    }
    catch (AppGuardClientException &ex)
    {
        ngx_log_error(
            NGX_LOG_ERR,
            request->connection->log,
            0,
            "AppGuardClientException: %s; falling back to default policy '%s'",
            ex.what(),
            default_policy_str.data());

        return ActOnPolicy(appguard::FirewallPolicy::UNKNOWN, default_policy);
    }
}
