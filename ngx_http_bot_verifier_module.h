#ifndef __NGX_HTTP_BOT_VERIFIER_MODULE_H__
#define __NGX_HTTP_BOT_VERIFIER_MODULE_H__

#define SUCCESS 0
#define NOT_FOUND 1
#define FAILURE 3
#define ERROR 2

typedef struct {
  ngx_str_t host;
  ngx_uint_t port;
  ngx_uint_t connection_timeout;
  ngx_uint_t read_timeout;
  ngx_uint_t expiry;
  redisContext *connection;
} redis_t;

typedef struct {
  ngx_flag_t enabled;
  redis_t redis;
} ngx_http_bot_verifier_module_loc_conf_t;

#endif
