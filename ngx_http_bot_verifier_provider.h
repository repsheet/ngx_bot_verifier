#ifndef __NGX_HTTP_BOT_VERIFIER_PROVIDER_H__
#define __NGX_HTTP_BOT_VERIFIER_PROVIDER_H__

#include <stdlib.h>
#include <ngx_core.h>

typedef struct {
  size_t len;
  const char *name;
  const char *valid_domains[];
} ngx_http_bot_verifier_module_provider_t;

ngx_http_bot_verifier_module_provider_t
*ngx_http_bot_verifier_module_make_provider(ngx_pool_t *pool, char *name, char *valid_domains[], size_t len);

#endif
