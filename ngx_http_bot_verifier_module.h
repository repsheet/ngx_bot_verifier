#ifndef __NGX_HTTP_BOT_VERIFIER_MODULE_H__
#define __NGX_HTTP_BOT_VERIFIER_MODULE_H__

#define SUCCESS 0
#define NOT_FOUND 1
#define FAILURE 3
#define ERROR 2

#include <ngx_core.h>
#include <hiredis/hiredis.h>

#include "ngx_http_bot_verifier_provider.h"

typedef struct {
  ngx_str_t host;
  ngx_uint_t port;
  ngx_uint_t connection_timeout;
  ngx_uint_t read_timeout;
  ngx_uint_t expiry;
  redisContext *connection;
} ngx_http_bot_verifier_module_redis_t;

typedef struct {
  ngx_flag_t enabled;
  ngx_flag_t repsheet_enabled;
  ngx_http_bot_verifier_module_redis_t redis;
  size_t provider_len;
  ngx_http_bot_verifier_module_provider_t **providers;
  ngx_regex_compile_t *identifier_regex;
  ngx_regex_compile_t *domain_regex;
} ngx_http_bot_verifier_module_loc_conf_t;

#endif
