#include "appguard.wrapper.hpp"
#include "appguard.inner.utils.hpp"
#include "appguard.nginx.module.hpp"
#include "appguard.uclient.exception.hpp"
#include "appguard.uclient.info.hpp"
#include "appguard.tcp.ucache.hpp"
#include "appguard.storage.hpp"

static ngx_http_output_header_filter_pt next_header_filter;

extern "C"
{
    static ngx_int_t ngx_http_appguard_request_handler(ngx_http_request_t *r)
    {
        return AppGuardNginxModule::RequestHandler(r);
    }

    static ngx_int_t ngx_http_appguard_response_handler(ngx_http_request_t *r)
    {
        return AppGuardNginxModule::ResponseHandler(r);
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

        {ngx_string("appguard_installation_code"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, installation_code),
         nullptr},

        {ngx_string("appguard_default_policy"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, default_policy),
         nullptr},

        {ngx_string("appguard_server_cert_path"),
         NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
         ngx_conf_set_str_slot,
         NGX_HTTP_SRV_CONF_OFFSET,
         offsetof(AppGuardNginxModule::Config, server_cert_path),
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

static ngx_int_t ActOnPolicy(appguard_commands::FirewallPolicy policy, appguard_commands::FirewallPolicy default_policy)
{
    switch (policy)
    {
    case appguard_commands::FirewallPolicy::ALLOW:
        return NGX_DECLINED;
    case appguard_commands::FirewallPolicy::DENY:
        return NGX_HTTP_FORBIDDEN;
    default:
        return (default_policy == appguard_commands::FirewallPolicy::ALLOW) ? NGX_DECLINED : NGX_HTTP_FORBIDDEN;
    }
}

ngx_int_t AppGuardNginxModule::Initialize(ngx_conf_t *cf)
{
    try
    {
        Storage::Initialize();
    }
    catch (...)
    {
        ngx_log_error(
            NGX_LOG_ERR,
            cf->log,
            0,
            "AppGuard: Failed to initialize persistent storage");
        return NGX_ERROR;
    }

    auto *cmcf = static_cast<ngx_http_core_main_conf_t *>(
        ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module));

    auto *h = static_cast<ngx_http_handler_pt *>(
        ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers));

    if (h == nullptr)
        return NGX_ERROR;

    *h = ngx_http_appguard_request_handler;

    next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_appguard_response_handler;

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
    ngx_conf_merge_str_value(conf->installation_code, prev->installation_code, "");

    ngx_conf_merge_str_value(conf->default_policy, prev->default_policy, "");
    ngx_conf_merge_str_value(conf->server_cert_path, prev->server_cert_path, "");

    return NGX_CONF_OK;
}

ngx_int_t AppGuardNginxModule::RequestHandler(ngx_http_request_t *request)
{
    auto *conf = static_cast<AppGuardNginxModule::Config *>(ngx_http_get_module_srv_conf(request, appguard_nginx_module));
    if (!conf || !conf->enabled)
        return NGX_DECLINED;

    auto default_policy_str = appguard::inner_utils::NgxStringToStdString(&conf->default_policy);
    auto default_policy = appguard::inner_utils::StringToFirewallPolicy(default_policy_str);
    auto installation_code = appguard::inner_utils::NgxStringToStdString(&conf->installation_code);
    auto server_addr = appguard::inner_utils::NgxStringToStdString(&conf->server_addr);
    auto server_cert_path = appguard::inner_utils::NgxStringToStdString(&conf->server_cert_path);

    if (installation_code.empty())
    {
        ngx_log_error(
            NGX_LOG_ERR,
            request->connection->log,
            0,
            "AppGuard: Installation Code hasn't been set; falling back to default policy '%s'",
            default_policy_str.data());

        return ActOnPolicy(appguard_commands::FirewallPolicy::UNKNOWN, default_policy);
    }

    try
    {
        AppGaurdClientInfo client_info {
            .installation_code = installation_code,
            .server_addr = server_addr,
            .server_cert_path = server_cert_path,
            .tls = !!conf->tls};

        auto client = AppGuardWrapper::CreateClient(client_info);

        auto connection = appguard::inner_utils::ExtractTcpConnectionInfo(request);
        auto tcp_response = client.HandleTcpConnection(connection);

        auto http_request = appguard::inner_utils::ExtractHttpRequestInfo(request);
        AppguardTcpInfoCache::Instance().Put(request->connection, tcp_response.tcp_info());
        http_request.set_allocated_tcp_info(new appguard::AppGuardTcpInfo(tcp_response.tcp_info()));

        auto ag_response = client.HandleHttpRequest(http_request);
        return ActOnPolicy(ag_response.policy(), default_policy);
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

        return ActOnPolicy(appguard_commands::FirewallPolicy::UNKNOWN, default_policy);
    }
}

ngx_int_t AppGuardNginxModule::ResponseHandler(ngx_http_request_t *request)
{
    auto *conf = static_cast<AppGuardNginxModule::Config *>(ngx_http_get_module_srv_conf(request, appguard_nginx_module));
    if (!conf || !conf->enabled)
        return next_header_filter(request);

    auto default_policy_str = appguard::inner_utils::NgxStringToStdString(&conf->default_policy);
    auto default_policy = appguard::inner_utils::StringToFirewallPolicy(default_policy_str);
    auto installation_code = appguard::inner_utils::NgxStringToStdString(&conf->installation_code);
    auto server_addr = appguard::inner_utils::NgxStringToStdString(&conf->server_addr);
    auto server_cert_path = appguard::inner_utils::NgxStringToStdString(&conf->server_cert_path);

    if (installation_code.empty())
    {
        ngx_log_error(
            NGX_LOG_ERR,
            request->connection->log,
            0,
            "AppGuard: Installation code hasn't been set; falling back to default policy '%s'",
            default_policy_str.data());

        ngx_int_t code = ActOnPolicy(appguard_commands::FirewallPolicy::UNKNOWN, default_policy);
        return code == NGX_DECLINED ? next_header_filter(request) : code;
    }

    try
    {
        AppGaurdClientInfo client_info{
            .installation_code = installation_code,
            .server_addr = server_addr,
            .server_cert_path = server_cert_path,
            .tls = !!conf->tls};

        auto client = AppGuardWrapper::CreateClient(client_info);

        auto http_response = appguard::inner_utils::ExtractHttpResponseInfo(request);

        if (auto tcp_info = AppguardTcpInfoCache::Instance().Get(request->connection); tcp_info.has_value())
        {
            http_response.set_allocated_tcp_info(new appguard::AppGuardTcpInfo(tcp_info.value()));
        }

        auto ag_response = client.HandleHttpResponse(http_response);
        ngx_int_t code = ActOnPolicy(ag_response.policy(), default_policy);
        return code == NGX_DECLINED ? next_header_filter(request) : code;
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

        ngx_int_t code = ActOnPolicy(appguard_commands::FirewallPolicy::UNKNOWN, default_policy);
        return code == NGX_DECLINED ? next_header_filter(request) : code;
    }
}
