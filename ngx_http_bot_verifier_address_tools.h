#ifndef __NGX_HTTP_BOT_VERIFIER_ADDRESS_TOOLS_H__
#define __NGX_HTTP_BOT_VERIFIER_ADDRESS_TOOLS_H__

ngx_int_t remote_address(char *connected_address, char *xff_header, ngx_str_t *address);
ngx_int_t ngx_http_bot_verifier_module_determine_address(ngx_http_request_t *r, ngx_str_t *address);

#endif
