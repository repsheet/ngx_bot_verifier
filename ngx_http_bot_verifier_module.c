#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

ngx_module_t ngx_http_bot_verifier_module;

typedef struct {
  ngx_flag_t enabled;
} ngx_http_bot_verifier_module_loc_conf_t;

static ngx_int_t
ngx_http_bot_verifier_module_verify_bot(ngx_http_request_t *r)
{
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  inet_pton(AF_INET, (char *)r->connection->addr_text.data, &(sa.sin_addr));
  char hostname[NI_MAXHOST];

  int error = getnameinfo((struct sockaddr *) &sa, sizeof(sa), hostname, sizeof(hostname), NULL, 0, 0);
  if (error != 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "getnameinfo() error: %s", gai_strerror(error));
    return NGX_ERROR;
  }

  struct addrinfo *result;
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "getaddrinfo() error: %s", gai_strerror(error));
    return NGX_ERROR;
  }

  struct sockaddr_in *forward = (struct sockaddr_in*)result->ai_addr;
  char *forward_result = inet_ntoa(forward->sin_addr);

  if (strcmp(hostname, forward_result) == 0) {
    freeaddrinfo(result);
    return NGX_OK;
  } else {
    freeaddrinfo(result);
    return NGX_DECLINED;
  }
}

static ngx_int_t
ngx_http_bot_verifier_module_identifies_as_known_bot(ngx_http_request_t *r)
{
  ngx_regex_t *re;
  ngx_regex_compile_t rc;
  u_char errstr[NGX_MAX_CONF_ERRSTR];

  ngx_str_t user_agent = r->headers_in.user_agent->value;
  if (user_agent.data == NULL) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent was not provided");
    return NGX_DECLINED;
  }

  ngx_str_t pattern = ngx_string("Google");
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
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Google bot identified for %V", &user_agent);
    ngx_int_t ret = ngx_http_bot_verifier_module_verify_bot(r);
    if (ret == NGX_OK) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Verification successful");
    } else if (ret == NGX_DECLINED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Verification failed");
      return NGX_HTTP_FORBIDDEN;
    }
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent %V not identified", &user_agent);
  return NGX_DECLINED;
}

static ngx_int_t
ngx_http_bot_verifier_module_handler(ngx_http_request_t *r)
{
  if (r->main->internal) {
    return NGX_DECLINED;
  }

  ngx_http_bot_verifier_module_loc_conf_t *loc_conf = ngx_http_get_module_loc_conf(r, ngx_http_bot_verifier_module);

  if (!loc_conf->enabled || loc_conf->enabled == NGX_CONF_UNSET) {
    return NGX_DECLINED;
  }

  ngx_int_t ret = ngx_http_bot_verifier_module_identifies_as_known_bot(r);
  if (ret == NGX_OK) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Bot identity detected");
  } else if (ret == NGX_HTTP_FORBIDDEN) {
    return NGX_HTTP_FORBIDDEN;
  } else {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Bot does not identify");
  }

  return NGX_OK;
}

static ngx_int_t
ngx_http_bot_verifier_module_init(ngx_conf_t *cf)
{
  ngx_http_handler_pt *h;
  ngx_http_core_main_conf_t *cmcf;

  cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
  h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);

  if (h == NULL) {
    return NGX_ERROR;
  }

  *h = ngx_http_bot_verifier_module_handler;

  return NGX_OK;
}

static ngx_command_t
ngx_http_bot_verifier_module_commands[] = {
  {
    ngx_string("bot_verifier"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_bot_verifier_module_loc_conf_t, enabled),
    NULL
  },
  ngx_null_command
};

static void*
ngx_http_bot_verifier_module_create_loc_conf(ngx_conf_t *cf)
{
  ngx_http_bot_verifier_module_loc_conf_t *conf;

  conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_bot_verifier_module_loc_conf_t));
  if (conf == NULL) {
    return NULL;
  }

  conf->enabled = NGX_CONF_UNSET;

  return conf;
}

static char*
ngx_http_bot_verifier_module_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  ngx_http_bot_verifier_module_loc_conf_t *prev = (ngx_http_bot_verifier_module_loc_conf_t *) parent;
  ngx_http_bot_verifier_module_loc_conf_t *conf = (ngx_http_bot_verifier_module_loc_conf_t *) child;

  ngx_conf_merge_value(conf->enabled, prev->enabled, 0);

  return NGX_CONF_OK;
}

static ngx_http_module_t ngx_http_bot_verifier_module_ctx = {
  NULL,                                         /* preconfiguration */
  ngx_http_bot_verifier_module_init,            /* postconfiguration */
  NULL,                                         /* create main configuration */
  NULL,                                         /* init main configuration */
  NULL,                                         /* create server configuration */
  NULL,                                         /* merge server configuration */
  ngx_http_bot_verifier_module_create_loc_conf, /* create location configuration */
  ngx_http_bot_verifier_module_merge_loc_conf   /* merge location configuration */
};

ngx_module_t ngx_http_bot_verifier_module = {
  NGX_MODULE_V1,
  &ngx_http_bot_verifier_module_ctx,     /* module context */
  ngx_http_bot_verifier_module_commands, /* module directives */
  NGX_HTTP_MODULE,                       /* module type */
  NULL,                                  /* init master */
  NULL,                                  /* init module */
  NULL,                                  /* init process */
  NULL,                                  /* init thread */
  NULL,                                  /* exit thread */
  NULL,                                  /* exit process */
  NULL,                                  /* exit master */
  NGX_MODULE_V1_PADDING
};
