use Test::Nginx::Socket 'no_plan';

no_shuffle();
run_tests();

__DATA__

=== TEST 1: No user agent provided (200)
--- config
location = /t {
  bot_verifier on;
  bot_verifier_redis_host localhost;
  bot_verifier_redis_port 6379;
  bot_verifier_redis_connection_timeout 10;
  bot_verifier_redis_read_timeout 10;
  bot_verifier_redis_expiry 3600;
  echo 'test';
}
--- request
GET /t
--- error_code: 200

=== TEST 2: User agent provided, not identified (200)
--- config
location = /t {
  bot_verifier on;
  bot_verifier_redis_host localhost;
  bot_verifier_redis_port 6379;
  bot_verifier_redis_connection_timeout 10;
  bot_verifier_redis_read_timeout 10;
  bot_verifier_redis_expiry 3600;
  echo 'test';
}
--- raw_request eval
"GET /t HTTP/1.1\r
Host: 127.0.0.1\r
Connection: close\r
User-Agent: test\r
\r
"
--- error_code: 200

=== TEST 3: User agent provided, identified, not valid (403)
--- config
location = /t {
  bot_verifier on;
  bot_verifier_redis_host localhost;
  bot_verifier_redis_port 6379;
  bot_verifier_redis_connection_timeout 10;
  bot_verifier_redis_read_timeout 10;
  bot_verifier_redis_expiry 3600;
  echo 'test';
}
--- raw_request eval
"GET /t HTTP/1.1\r
Host: 127.0.0.1\r
Connection: close\r
User-Agent: Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html\r
\r
"
--- error_code: 403

=== TEST 4: google agent provided, identified, valid (200)
--- config
location = /t {
  bot_verifier on;
  bot_verifier_redis_host localhost;
  bot_verifier_redis_port 6379;
  bot_verifier_redis_connection_timeout 10;
  bot_verifier_redis_read_timeout 10;
  bot_verifier_redis_expiry 3600;
  echo 'test';
}
--- raw_request eval
"GET /t HTTP/1.1\r
Host: 127.0.0.1\r
Connection: close\r
X-Forwarded-For: 66.249.66.1\r
User-Agent: Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html\r
\r
"
--- error_code: 200

=== TEST 5: bing agent provided, identified, valid (200)
--- config
location = /t {
  bot_verifier on;
  bot_verifier_redis_host localhost;
  bot_verifier_redis_port 6379;
  bot_verifier_redis_connection_timeout 10;
  bot_verifier_redis_read_timeout 10;
  bot_verifier_redis_expiry 3600;
  echo 'test';
}
--- raw_request eval
"GET /t HTTP/1.1\r
Host: 127.0.0.1\r
Connection: close\r
X-Forwarded-For: 157.55.39.5\r
User-Agent: Mozilla/5.0 (compatible; bingbot/2.0; +http://www.bing.com/bingbot.htm)\r
\r
"
--- error_code: 200

=== TEST 6: yahoo agent provided, identified, valid (200)
--- config
location = /t {
  bot_verifier on;
  bot_verifier_redis_host localhost;
  bot_verifier_redis_port 6379;
  bot_verifier_redis_connection_timeout 10;
  bot_verifier_redis_read_timeout 10;
  bot_verifier_redis_expiry 3600;
  echo 'test';
}
--- raw_request eval
"GET /t HTTP/1.1\r
Host: 127.0.0.1\r
Connection: close\r
X-Forwarded-For: 8.12.144.5\r
User-Agent: Mozilla/5.0 (compatible; Yahoo! Slurp; http://help.yahoo.com/help/us/ysearch/slurp)\r
\r
"
--- error_code: 200

=== TEST 7: agent provided, address does not resolve to valid domain (403)
--- config
location = /t {
  bot_verifier on;
  bot_verifier_redis_host localhost;
  bot_verifier_redis_port 6379;
  bot_verifier_redis_connection_timeout 10;
  bot_verifier_redis_read_timeout 10;
  bot_verifier_redis_expiry 3600;
  echo 'test';
}
--- raw_request eval
"GET /t HTTP/1.1\r
Host: 127.0.0.1\r
Connection: close\r
X-Forwarded-For: 120.40.14.1\r
User-Agent: Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html\r
\r
"
--- error_code: 403
