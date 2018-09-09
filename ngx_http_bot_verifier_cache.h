#ifndef __NGX_HTTP_BOT_VERIFIER_CACHE_H__
#define __NGX_HTTP_BOT_VERIFIER_CACHE_H__

ngx_int_t check_connection(redisContext *context);
void cleanup_connection(ngx_http_bot_verifier_module_loc_conf_t *loc_conf);
ngx_int_t reset_connection(ngx_http_bot_verifier_module_loc_conf_t *loc_conf);
ngx_int_t lookup_verification_status(redisContext *context, ngx_str_t *address);
ngx_int_t persist_verification_status(redisContext *context, ngx_str_t *address, ngx_int_t status, ngx_int_t expiry);

#endif
