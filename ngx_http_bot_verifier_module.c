#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <hiredis/hiredis.h>

#define SUCCESS 0
#define NOT_FOUND 1
#define FAILURE 3
#define ERROR 2


ngx_module_t ngx_http_bot_verifier_module;

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

static ngx_int_t
remote_address(char *connected_address, char *xff_header, char *address)
{
  if ((connected_address == NULL && xff_header == NULL) || address == NULL) {
    return NGX_DECLINED;
  }

  int length;
  memset(address, '\0', INET_ADDRSTRLEN);

  if (xff_header != NULL) {
    char *p;

    for (p = xff_header; p < (xff_header + strlen(xff_header)); p++) {
      if (*p == ' ' || *p == ',') {
        break;
      }
    }

    length = p - xff_header;
    char test_address[length + 1];
    memcpy(test_address, xff_header, length);
    test_address[length] = '\0';

    unsigned char buf[sizeof(struct in_addr)];

    if (inet_pton(AF_INET, (const char *)test_address, buf) == 1) {
      memcpy(address, test_address, length);
      return NGX_OK;
    } else {
      return NGX_ERROR;
    }
  } else {
    memcpy(address, connected_address, strlen(connected_address));
    return NGX_OK;
  }
}

static ngx_int_t
ngx_http_bot_verifier_module_determine_address(ngx_http_request_t *r, char *address)
{
  ngx_int_t result;
  ngx_table_elt_t *xff = NULL;
  ngx_array_t *ngx_array = &r->headers_in.x_forwarded_for;
  if (ngx_array != NULL && ngx_array->nelts > 0) {
    ngx_table_elt_t **first_elt = ngx_array->elts;
    xff = first_elt[0];
  }

  if (xff == NULL) {
    memcpy(address, r->connection->addr_text.data, r->connection->addr_text.len);
    address[r->connection->addr_text.len] = '\0';
    return NGX_OK;
  } else {
    result = remote_address((char *)r->connection->addr_text.data, (char*)xff->value.data, address);
    if (result == NGX_OK) {
      return NGX_OK;
    } else if (result == NGX_DECLINED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Not enough information to determine connecting IP address");
      return NGX_ERROR;
    } else {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "The address supplied is not a valid IP address");
      return NGX_ERROR;
    }
  }
}

static ngx_int_t
ngx_http_bot_verifier_module_verify_bot(ngx_http_request_t *r)
{
  char dervied_address[INET_ADDRSTRLEN];
  ngx_int_t error = ngx_http_bot_verifier_module_determine_address(r, dervied_address);
  if (error == NGX_ERROR || error == NGX_DECLINED) {
    return NGX_ERROR;
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Using %s as connected address", &dervied_address);

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  inet_pton(AF_INET, dervied_address, &(sa.sin_addr));
  char hostname[NI_MAXHOST];

  error = getnameinfo((struct sockaddr *) &sa, sizeof(sa), hostname, sizeof(hostname), NULL, 0, 0);
  if (error != 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "getnameinfo() error: %s", gai_strerror(error));
    return NGX_ERROR;
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Lookup hostname %s", &hostname);

  struct addrinfo *result;
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "getaddrinfo() error: %s", gai_strerror(error));
    return NGX_ERROR;
  }

  struct sockaddr_in *forward = (struct sockaddr_in*)result->ai_addr;
  char *forward_result = inet_ntoa(forward->sin_addr);

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Lookup IP %s", forward_result);

  if (strcmp(dervied_address, forward_result) == 0) {
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
    return NGX_OK;
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "User Agent %V not identified", &user_agent);

  return NGX_DECLINED;
}

static ngx_int_t
check_connection(redisContext *context) {
  if (context == NULL || context->err) {
    return NGX_ERROR;
  }

  redisReply *reply;
  reply = redisCommand(context, "PING");
  if (reply) {
    if (reply->type == REDIS_REPLY_ERROR) {
      freeReplyObject(reply);
      return NGX_ERROR;
    } else {
      freeReplyObject(reply);
      return NGX_OK;
    }
  } else {
    return NGX_ERROR;
  }
}

static void
cleanup_connection(ngx_http_bot_verifier_module_loc_conf_t *loc_conf)
{
  if (loc_conf->redis.connection != NULL) {
    redisFree(loc_conf->redis.connection);
    loc_conf->redis.connection = NULL;
  }
}

static ngx_int_t
reset_connection(ngx_http_bot_verifier_module_loc_conf_t *loc_conf)
{
  cleanup_connection(loc_conf);
  const char *host = (const char *)loc_conf->redis.host.data;
  int port = loc_conf->redis.port;
  int connection_timeout = loc_conf->redis.connection_timeout;
  int read_timeout = loc_conf->redis.read_timeout;

  struct timeval ct = {0, (connection_timeout > 0) ? (connection_timeout * 1000) : 10000};
  struct timeval rt = {0, (read_timeout > 0) ? (read_timeout * 1000) : 10000};

  redisContext *context = redisConnectWithTimeout(host, port, ct);
  if (context == NULL || context->err) {
    if (context) {
      redisFree(context);
    }
    return NGX_ERROR;
  } else {
    redisSetTimeout(context, rt);
    loc_conf->redis.connection = context;
    return NGX_OK;
  }
}

static ngx_int_t
lookup_verification_status(redisContext *context, char *address)
{
  redisReply *reply;

  reply = redisCommand(context, "GET %s:bvs", address);
  if (reply) {
    if (reply->type == REDIS_REPLY_STRING) {
      if (strncmp("failure", reply->str, strlen("failure")) == 0) {
        freeReplyObject(reply);
        return FAILURE;
      } else if (strncmp("success", reply->str, strlen("success")) == 0) {
        freeReplyObject(reply);
        return SUCCESS;
      } else {
        freeReplyObject(reply);
        return ERROR;
      }
    } else if (reply->type == REDIS_REPLY_NIL) {
      freeReplyObject(reply);
      return NOT_FOUND;
    } else {
      freeReplyObject(reply);
      return ERROR;
    }
  } else {
    return ERROR;
  }
}

static ngx_int_t
persist_verification_status(redisContext *context, char *address, ngx_int_t status)
{
  redisReply *reply = NULL;

  if (status == NGX_OK) {
    reply = redisCommand(context, "SET %s:bvs %s", address, "success");
  } else if (status == NGX_DECLINED) {
    reply = redisCommand(context, "SET %s:bvs %s", address, "failure");
  }

  if (reply) {
    freeReplyObject(reply);
  }

  return NGX_OK;
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

  ngx_int_t connection_status = check_connection(loc_conf->redis.connection);
  if (connection_status == NGX_ERROR) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "No cache connection found, creating a new connection");

    if (loc_conf->redis.connection != NULL) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Cache connection error: %s", loc_conf->redis.connection->errstr);
    }

    connection_status = reset_connection(loc_conf);

    if (connection_status == NGX_ERROR) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Unable to establish a connection to cache, bypassing");

      if (loc_conf->redis.connection != NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Cache connection error: %s", loc_conf->redis.connection->errstr);
        cleanup_connection(loc_conf);
      }

      return NGX_DECLINED;
    }
  }

  char *address;
  address = malloc(sizeof(INET_ADDRSTRLEN));
  ngx_int_t address_status = ngx_http_bot_verifier_module_determine_address(r, address);
  if (address_status == NGX_ERROR) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Unable to determine connected address, bypassing");
    return NGX_DECLINED;
  }

  ngx_int_t verification_status = lookup_verification_status(loc_conf->redis.connection, address);
  if (verification_status == NGX_ERROR) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Unable to lookup verification status, bypassing");
    return NGX_DECLINED;
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Lookup result %d", verification_status);

  if (verification_status == SUCCESS) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Actor has already been verified, bypassing");
    return NGX_DECLINED;
  }

  if (verification_status == FAILURE) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Actor previously failed verification, blocking request");
    return NGX_HTTP_FORBIDDEN;
  }

  if (verification_status == ERROR) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "There was an error looking up the actor, failing open");
    return NGX_DECLINED;
  }

  if (verification_status == NOT_FOUND) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Actor has not been verified, initiating verification process");
  }

  ngx_int_t ret = ngx_http_bot_verifier_module_identifies_as_known_bot(r);

  if (ret == NGX_OK) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Bot identity detected");
    ret = ngx_http_bot_verifier_module_verify_bot(r);
    if (ret == NGX_OK) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Verification successful");
      persist_verification_status(loc_conf->redis.connection, address, ret);
    } else if (ret == NGX_DECLINED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Verification failed");
      persist_verification_status(loc_conf->redis.connection, address, ret);
      return NGX_HTTP_FORBIDDEN;
    }
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
  {
    ngx_string("bot_verifier_redis_host"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_str_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_bot_verifier_module_loc_conf_t, redis.host),
    NULL
  },
  {
    ngx_string("bot_verifier_redis_port"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_bot_verifier_module_loc_conf_t, redis.port),
    NULL
  },
  {
    ngx_string("bot_verifier_redis_connection_timeout"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_bot_verifier_module_loc_conf_t, redis.connection_timeout),
    NULL
  },
  {
    ngx_string("bot_verifier_redis_read_timeout"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_bot_verifier_module_loc_conf_t, redis.read_timeout),
    NULL
  },
  {
    ngx_string("bot_verifier_redis_expiry"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
    ngx_conf_set_num_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_bot_verifier_module_loc_conf_t, redis.expiry),
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
  conf->redis.port = NGX_CONF_UNSET_UINT;
  conf->redis.connection_timeout = NGX_CONF_UNSET_UINT;
  conf->redis.read_timeout = NGX_CONF_UNSET_UINT;
  conf->redis.expiry = NGX_CONF_UNSET_UINT;
  conf->redis.connection = NULL;

  return conf;
}

static char*
ngx_http_bot_verifier_module_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  ngx_http_bot_verifier_module_loc_conf_t *prev = (ngx_http_bot_verifier_module_loc_conf_t *) parent;
  ngx_http_bot_verifier_module_loc_conf_t *conf = (ngx_http_bot_verifier_module_loc_conf_t *) child;

  ngx_conf_merge_value(conf->enabled,                  prev->enabled,                  0);
  ngx_conf_merge_value(conf->redis.port,               prev->redis.port,               6379);
  ngx_conf_merge_value(conf->redis.connection_timeout, prev->redis.connection_timeout, 10);
  ngx_conf_merge_value(conf->redis.read_timeout,       prev->redis.read_timeout,       10);
  ngx_conf_merge_value(conf->redis.expiry,             prev->redis.expiry,             86400);

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
