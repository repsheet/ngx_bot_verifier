#include <ngx_http.h>

ngx_int_t
ngx_http_bot_verifier_module_remote_address(ngx_http_request_t *r, u_char *xff_header, char *address)
{
  if ((r == NULL && xff_header == NULL) || address == NULL) {
    return NGX_DECLINED;
  }

  int length;

  if (xff_header != NULL) {
    u_char *p;

    for (p = xff_header; p < (xff_header + strlen((const char *)xff_header)); p++) {
      if (*p == ' ' || *p == ',') {
        break;
      }
    }

    length = p - xff_header;
    u_char test_address[length + 1];
    memset(test_address, '\0', length + 1);
    memcpy(test_address, xff_header, length);
    unsigned char buf[sizeof(struct in_addr)];

    if (inet_pton(AF_INET, (const char *)test_address, buf) == 1) {
      memcpy(address, test_address, length);
      return NGX_OK;
    } else {
      return NGX_ERROR;
    }
  } else {
    memcpy(address, r->connection->addr_text.data, r->connection->addr_text.len);
    return NGX_OK;
  }
}

ngx_int_t
ngx_http_bot_verifier_module_determine_address(ngx_http_request_t *r, char *address)
{
  ngx_int_t result;
  ngx_table_elt_t *xff = NULL;
#if defined(nginx_version) && nginx_version >= 1023000
  xff = r->headers_in.x_forwarded_for;
#else
  ngx_array_t *ngx_array = &r->headers_in.x_forwarded_for;
  if (ngx_array != NULL && ngx_array->nelts > 0) {
    ngx_table_elt_t **first_elt = ngx_array->elts;
    xff = first_elt[0];
  }
#endif

  if (xff == NULL) {
    memcpy(address, r->connection->addr_text.data, r->connection->addr_text.len);
    return NGX_OK;
  } else {
    result = ngx_http_bot_verifier_module_remote_address(r, xff->value.data, address);
    if (result == NGX_OK) {
      return NGX_OK;
    } else if (result == NGX_DECLINED) {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Cannot determine IP address");
      return NGX_ERROR;
    } else {
      ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "IP address is not valid");
      return NGX_ERROR;
    }
  }
}
