use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: bot_verifier on, not identified
Return a 200 if not identified

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

