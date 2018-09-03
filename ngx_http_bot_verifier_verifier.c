#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_bot_verifier_address_tools.h"
#include "ngx_http_bot_verifier_provider.h"

ngx_int_t
hostname_matches_provider_domain(ngx_http_request_t *r, char *hostname)
{
  // TODO: move this into module init so it's not done every time
  size_t len;

  char *google_domains[2] = {"google.com", "googlebot.com"};
  len = sizeof(google_domains) / sizeof(google_domains[0]);
  provider_t *google = make_provider("Google", google_domains, len);

  char *bing_domains[1] = {"search.msn.com"};
  len = sizeof(bing_domains) / sizeof(bing_domains[0]);
  provider_t *bing = make_provider("Bing", bing_domains, len);

  char *yahoo_domains[1] = {"yahoo.com"};
  len = sizeof(yahoo_domains) / sizeof(yahoo_domains[0]);
  provider_t *yahoo = make_provider("Yahoo", yahoo_domains, len);

  provider_t *providers[] = { google, bing, yahoo };
  size_t provider_len = sizeof(providers) / sizeof(providers[0]);
  // END TODO


  ngx_regex_t *re;
  ngx_regex_compile_t rc;
  u_char errstr[NGX_MAX_CONF_ERRSTR];
  ngx_str_t pattern = ngx_string("[^.]*\\.[^.]{2,3}(?:\\.[^.]{2,3})?$");
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
      for (j = 0; j < provider_len; j++) {
	// TODO: This could be optimized capturing the name of the provider that matched and only iteration through that providers valid domains.
	for (k = 0; k < providers[j]->len; k++) {
	  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Comparing %s against: %s", capture.data, providers[j]->valid_domains[k]);
	  if (ngx_strncmp(capture.data, providers[j]->valid_domains[k], strlen(providers[j]->valid_domains[k])) == 0) {
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
  ngx_int_t match_result = hostname_matches_provider_domain(r, (char *)hostname);

  if (match_result == NGX_DECLINED) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Match result %d", match_result);
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
  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Derived Address %s", dervied_address);

  if (strcmp(dervied_address, forward_result) == 0) {
    freeaddrinfo(result);
    return NGX_OK;
  } else {
    freeaddrinfo(result);
    return NGX_DECLINED;
  }
}
