#include <stdlib.h>

#include "ngx_http_bot_verifier_provider.h"

ngx_http_bot_verifier_module_provider_t
*ngx_http_bot_verifier_module_make_provider(ngx_pool_t *pool, char *name, char *valid_domains[], size_t len) {
  ngx_http_bot_verifier_module_provider_t *provider = (ngx_http_bot_verifier_module_provider_t*) ngx_palloc(pool, sizeof(ngx_http_bot_verifier_module_provider_t) + sizeof(char*) * len);
  provider->name = name;
  provider->len = len;

  int i;
  for (i = 0; i < (int)provider->len; i++) {
    provider->valid_domains[i] = valid_domains[i];
  }

  return provider;
}
