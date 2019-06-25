#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_bot_verifier_module.h"
#include "ngx_http_bot_verifier_address_tools.h"
#include "ngx_http_bot_verifier_provider.h"

ngx_int_t
ngx_http_bot_verifier_module_hostname_matches_provider_domain(ngx_http_request_t *r, char *hostname, ngx_http_bot_verifier_module_loc_conf_t *loc_conf)
{
  ngx_regex_t *re = loc_conf->domain_regex->regex;
  ngx_regex_compile_t rc = *loc_conf->domain_regex;

  ngx_int_t n;
  int captures[(1 + rc.captures) * 3];
  ngx_str_t ngx_hostname;
  ngx_hostname.data = (u_char *)hostname;
  ngx_hostname.len = strlen(hostname);
  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "HOSTNAME: %V", &ngx_hostname);
  n = ngx_regex_exec(re, &ngx_hostname, captures, (1 + rc.captures) * 3);

  ngx_str_t capture;
  int i, j, k;
  if (n >= 0) {
    for (i = 0; i < n * 2; i += 2) {
      capture.data = ngx_hostname.data + captures[i];
      capture.len = captures[i + 1] - captures[i];
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Capture: %V", &capture);
      for (j = 0; j < (int)loc_conf->provider_len; j++) {
	// TODO: This could be optimized capturing the name of the provider that matched and only iteration through that providers valid domains.
	for (k = 0; k < (int)loc_conf->providers[j]->len; k++) {
	  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Comparing %s against: %s", capture.data, loc_conf->providers[j]->valid_domains[k]);
	  if (ngx_strncmp(capture.data, loc_conf->providers[j]->valid_domains[k], strlen(loc_conf->providers[j]->valid_domains[k])) == 0) {
	    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Found match for %V with %V", &ngx_hostname, &capture);
	    return NGX_OK;
	  }
	}
      }
    }
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Result does not match known domain");
  return NGX_DECLINED;
}

ngx_int_t
ngx_http_bot_verifier_module_verify_bot(ngx_http_request_t *r, ngx_http_bot_verifier_module_loc_conf_t *loc_conf, char *address)
{
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  inet_pton(AF_INET, (const char *)address, &(sa.sin_addr));
  char hostname[NI_MAXHOST];

  int error = getnameinfo((struct sockaddr *) &sa, sizeof(sa), hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD);
  if (error != 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "getnameinfo() error: %s", gai_strerror(error));
    return NGX_DECLINED;
  }

  ngx_int_t match_result = ngx_http_bot_verifier_module_hostname_matches_provider_domain(r, hostname, loc_conf);

  if (match_result == NGX_DECLINED) {
    return match_result;
  }

  struct addrinfo *result;
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "getaddrinfo() error: %s", gai_strerror(error));
    return NGX_ERROR;
  }

  struct sockaddr_in *forward = (struct sockaddr_in*)result->ai_addr;
  char *forward_result = inet_ntoa(forward->sin_addr);

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Forward Result %s", forward_result);
  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Actor Address %s", address);

  if (strcmp((const char *)address, forward_result) == 0) {
    freeaddrinfo(result);
    return NGX_OK;
  } else {
    freeaddrinfo(result);
    return NGX_DECLINED;
  }
}
