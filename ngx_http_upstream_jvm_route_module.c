#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_int_t ngx_http_upstream_jvm_route_init_module(ngx_conf_t *cf);
static ngx_int_t ngx_http_upstream_jvm_route_init_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *uscf);

static ngx_http_module_t ngx_http_upstream_jvm_route_module_ctx = {
    NULL,                                     /* preconfiguration */
    ngx_http_upstream_jvm_route_init_module,  /* postconfiguration */

    NULL,                                     /* create main configuration */
    NULL,                                     /* init main configuration */

    NULL,                                     /* create server configuration */
    NULL,                                     /* merge server configuration */

    NULL,                                     /* create location configuration */
    NULL                                      /* merge location configuration */
};

ngx_module_t ngx_http_upstream_jvm_route_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_jvm_route_module_ctx,  /* module context */
    NULL,                                     /* module directives */
    NGX_HTTP_MODULE,                          /* module type */
    NULL,                                     /* init master */
    NULL,                                     /* init module */
    NULL,                                     /* init process */
    NULL,                                     /* init thread */
    NULL,                                     /* exit thread */
    NULL,                                     /* exit process */
    NULL,                                     /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t
ngx_http_upstream_jvm_route_init_module(ngx_conf_t *cf)
{
    ngx_http_upstream_main_conf_t  *umcf;
    ngx_http_upstream_srv_conf_t  **uscfp;
    ngx_uint_t i;

    umcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_upstream_module);
    uscfp = umcf->upstreams.elts;

    for (i = 0; i < umcf->upstreams.nelts; i++) {
        if (uscfp[i]->peer.init_upstream) {
            continue;
        }

        uscfp[i]->peer.init_upstream = ngx_http_upstream_jvm_route_init_peer;
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_upstream_jvm_route_init_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *uscf)
{
    ngx_http_upstream_rr_peer_data_t  *rrp;
    ngx_http_upstream_server_t        *server;
    ngx_str_t jsessionid;
    ngx_uint_t i;

    if (ngx_http_parse_multi_header_lines(&r->headers_in.cookies,
                                          &ngx_string("JSESSIONID"),
                                          &jsessionid)
        == NGX_DECLINED)
    {
        return NGX_OK;
    }

    server = uscf->servers->elts;

    for (i = 0; i < uscf->servers->nelts; i++) {
        if (server[i].srun_id.len > 0 &&
            jsessionid.len > server[i].srun_id.len &&
            ngx_strncmp(jsessionid.data + jsessionid.len - server[i].srun_id.len,
                        server[i].srun_id.data,
                        server[i].srun_id.len) == 0)
        {
            rrp = r->upstream->peer.data;
            rrp->current = &rrp->peers->peer[i];
            break;
        }
    }

    return NGX_OK;
}