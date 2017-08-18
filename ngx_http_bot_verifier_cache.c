#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <hiredis/hiredis.h>

#include "ngx_http_bot_verifier_module.h"
#include "ngx_http_bot_verifier_cache.h"

ngx_int_t
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

void
cleanup_connection(ngx_http_bot_verifier_module_loc_conf_t *loc_conf)
{
  if (loc_conf->redis.connection != NULL) {
    redisFree(loc_conf->redis.connection);
    loc_conf->redis.connection = NULL;
  }
}

ngx_int_t
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

ngx_int_t
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

ngx_int_t
persist_verification_status(redisContext *context, char *address, ngx_int_t status, ngx_int_t expiry)
{
  redisReply *reply = NULL;

  if (status == NGX_OK) {
    reply = redisCommand(context, "SETEX %s:bvs %d %s", address, expiry, "success");
  } else if (status == NGX_DECLINED) {
    reply = redisCommand(context, "SETEX %s:bvs %d %s", address, expiry, "failure");
  }

  if (reply) {
    freeReplyObject(reply);
  }

  return NGX_OK;
}
