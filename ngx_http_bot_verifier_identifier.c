#include <ngx_http.h>

#include "ngx_http_bot_verifier_module.h"

ngx_int_t
ngx_http_bot_verifier_module_identifies_as_known_bot(ngx_http_request_t *r, ngx_http_bot_verifier_module_loc_conf_t *loc_conf)
{
  if (r->headers_in.user_agent == NULL) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent was not provided");
    return NGX_DECLINED;
  }

  ngx_str_t user_agent = r->headers_in.user_agent->value;
  if (user_agent.data == NULL) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent was not provided");
    return NGX_DECLINED;
  }

  ngx_regex_t *re = loc_conf->identifier_regex->regex;
  ngx_regex_compile_t rc = *loc_conf->identifier_regex;

  ngx_int_t n;
  int captures[(1 + rc.captures) * 3];
  n = ngx_regex_exec(re, &user_agent, captures, (1 + rc.captures) * 3);

  if (n >= 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent identified as provider %V", &user_agent);
    return NGX_OK;
  }

  return NGX_DECLINED;
}
