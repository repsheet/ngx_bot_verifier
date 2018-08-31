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
