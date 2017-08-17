#include <ngx_http.h>

ngx_int_t
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

ngx_int_t
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
