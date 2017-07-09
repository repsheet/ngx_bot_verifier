use Test::Nginx::Socket 'no_plan';

run_tests();

__DATA__

=== TEST 1: bot_verifier on, not identified
Return a 200 if not identified

--- config
location = /t {
  bot_verifier on;
  echo 'test';
}

--- request
GET /t

--- error_code: 200

