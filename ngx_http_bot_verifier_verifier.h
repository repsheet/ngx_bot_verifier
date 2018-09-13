#ifndef __NGX_HTTP_BOT_VERIFIER_VERIFIER_H__
#define __NGX_HTTP_BOT_VERIFIER_VERIFIER_H__

ngx_int_t ngx_http_bot_verifier_module_verify_bot(ngx_http_request_t *r, ngx_http_bot_verifier_module_loc_conf_t *loc_conf, char *address);

#endif
