#ifndef __NGX_HTTP_BOT_VERIFIER_PROVIDER_H__
#define __NGX_HTTP_BOT_VERIFIER_PROVIDER_H__

#include <stdlib.h>

typedef struct {
  size_t len;
  const char *name;
  const char *valid_domains[];
} provider_t;

provider_t *make_provider(char *name, char *valid_domains[], size_t len);

#endif
