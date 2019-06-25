#include <stdlib.h>

#include "ngx_http_bot_verifier_provider.h"

provider_t *make_provider(char *name, char *valid_domains[], size_t len) {
  provider_t *provider = (provider_t*) malloc(sizeof(provider_t) + sizeof(char*) * len);
  provider->name = name;
  provider->len = len;

  int i;
  for (i = 0; i < (int)provider->len; i++) {
    provider->valid_domains[i] = valid_domains[i];
  }

  return provider;
}
