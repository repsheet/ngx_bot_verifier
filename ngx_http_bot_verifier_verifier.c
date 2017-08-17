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

  struct addrinfo *result;
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "getaddrinfo() error: %s", gai_strerror(error));
    return NGX_ERROR;
  }

  struct sockaddr_in *forward = (struct sockaddr_in*)result->ai_addr;
  char *forward_result = inet_ntoa(forward->sin_addr);

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Lookup IP %s", forward_result);

  if (strcmp(dervied_address, forward_result) == 0) {
    freeaddrinfo(result);
    return NGX_OK;
  } else {
    freeaddrinfo(result);
    return NGX_DECLINED;
  }
}
