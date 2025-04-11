#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct
{
    ngx_flag_t enable;
    ngx_str_t server_addr;
} ngx_http_appguard_srv_conf_t;

static ngx_int_t ngx_http_appguard_handler(ngx_http_request_t *r);

static ngx_int_t ngx_http_appguard_init(ngx_conf_t *cf)
{
    ngx_http_core_main_conf_t *cmcf;
    ngx_http_handler_pt *h;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
    if (h == NULL)
    {
        return NGX_ERROR;
    }

    *h = ngx_http_appguard_handler;

    return NGX_OK;
}

static void *ngx_http_appguard_create_srv_conf(ngx_conf_t *cf)
{
    ngx_http_appguard_srv_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_appguard_srv_conf_t));
    if (conf == NULL)
    {
        return NULL;
    }

    conf->enable = NGX_CONF_UNSET;
    return conf;
}

static char *ngx_http_appguard_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_appguard_srv_conf_t *prev = parent;
    ngx_http_appguard_srv_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_str_value(conf->server_addr, prev->server_addr, "");

    return NGX_CONF_OK;
}

// Directive handlers
static ngx_command_t appguard_nginx_module_commands[] = {
    {ngx_string("appguard_enabled"),
     NGX_HTTP_SRV_CONF | NGX_CONF_FLAG,
     ngx_conf_set_flag_slot,
     NGX_HTTP_SRV_CONF_OFFSET,
     offsetof(ngx_http_appguard_srv_conf_t, enable),
     NULL},
    {ngx_string("appguard_server"),
     NGX_HTTP_SRV_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot,
     NGX_HTTP_SRV_CONF_OFFSET,
     offsetof(ngx_http_appguard_srv_conf_t, server_addr),
     NULL},
    ngx_null_command};

// Module context
static ngx_http_module_t appguard_nginx_module_ctx = {
    NULL,                   // preconf
    ngx_http_appguard_init, // postconf

    NULL, // create main
    NULL, // init main

    ngx_http_appguard_create_srv_conf, // create srv
    ngx_http_appguard_merge_srv_conf,  // merge srv

    NULL, // create loc
    NULL  // merge loc
};

// Module definition
ngx_module_t appguard_nginx_module = {
    NGX_MODULE_V1,
    &appguard_nginx_module_ctx,
    appguard_nginx_module_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING};

static ngx_int_t ngx_http_appguard_handler(ngx_http_request_t *r)
{
    ngx_http_appguard_srv_conf_t *conf;
    conf = ngx_http_get_module_srv_conf(r, appguard_nginx_module);

    if (!conf->enable)
    {
        return NGX_DECLINED;
    }

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "appguard called: %V", &r->uri);

    return NGX_DECLINED;
}