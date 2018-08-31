#include <ngx_http.h>

ngx_int_t
ngx_http_bot_verifier_module_identifies_as_known_bot(ngx_http_request_t *r)
{
  ngx_regex_t *re;
  ngx_regex_compile_t rc;
  u_char errstr[NGX_MAX_CONF_ERRSTR];

  if (r->headers_in.user_agent == NULL) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent was not provided");
    return NGX_DECLINED;
  }

  ngx_str_t user_agent = r->headers_in.user_agent->value;
  if (user_agent.data == NULL) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent was not provided");
    return NGX_DECLINED;
  }

  ngx_str_t pattern = ngx_string("google|bing");
  ngx_memzero(&rc, sizeof(ngx_regex_compile_t));
  rc.pattern = pattern;
  rc.pool = r->pool;
  rc.err.len = NGX_MAX_CONF_ERRSTR;
  rc.err.data = errstr;

  if (ngx_regex_compile(&rc) != NGX_OK) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Regex error: %V", &rc.err);
    return NGX_ERROR;
  }

  re = rc.regex;

  ngx_int_t n;
  int captures[(1 + rc.captures) * 3];
  n = ngx_regex_exec(re, &user_agent, captures, (1 + rc.captures) * 3);

  if (n >= 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Search engine bot identified for %V", &user_agent);
    return NGX_OK;
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent %V not identified", &user_agent);

  return NGX_DECLINED;
}
