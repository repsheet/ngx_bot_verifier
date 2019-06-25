#include "ngx_http_bot_verifier_module.h"
#include "ngx_http_bot_verifier_regex.h"

ngx_regex_compile_t *ngx_http_bot_verifier_module_make_regex(ngx_pool_t *pool, ngx_str_t *pattern) {
  ngx_regex_compile_t *rc = ngx_pcalloc(pool, sizeof(ngx_regex_compile_t));
  u_char errstr[NGX_MAX_CONF_ERRSTR];

  ngx_memzero(rc, sizeof(ngx_regex_compile_t));
  rc->pattern = *pattern;
  rc->pool = pool;
  rc->err.len = NGX_MAX_CONF_ERRSTR;
  rc->err.data = errstr;

  if (ngx_regex_compile(rc) != NGX_OK) {
    return NULL;
  }

  return rc;
}

