#include <ngx_http.h>

ngx_int_t
remote_address(u_char *connected_address, u_char *xff_header, ngx_str_t *address)
{
  if ((connected_address == NULL && xff_header == NULL) || address == NULL) {
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
    memcpy(test_address, xff_header, length);
    test_address[length] = '\0';

    unsigned char buf[sizeof(struct in_addr)];

    if (inet_pton(AF_INET, (const char *)test_address, buf) == 1) {
      address->data = malloc(sizeof(u_char *) * length + 1);
      ngx_memcpy(address->data, test_address, length + 1);
      address->len = length + 1;
      return NGX_OK;
    } else {
      return NGX_ERROR;
    }
  } else {
    address->data = connected_address;
    address->len = strlen((const char *)connected_address);
    return NGX_OK;
  }
}

ngx_int_t
ngx_http_bot_verifier_module_determine_address(ngx_http_request_t *r, ngx_str_t *address)
{
  ngx_int_t result;
  ngx_table_elt_t *xff = NULL;
  ngx_array_t *ngx_array = &r->headers_in.x_forwarded_for;
  if (ngx_array != NULL && ngx_array->nelts > 0) {
    ngx_table_elt_t **first_elt = ngx_array->elts;
    xff = first_elt[0];
  }

  if (xff == NULL) {
    address->data = r->connection->addr_text.data;
    address->len = r->connection->addr_text.len;

    return NGX_OK;
  } else {
    result = remote_address(r->connection->addr_text.data, xff->value.data, address);
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
