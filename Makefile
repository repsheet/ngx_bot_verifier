export PATH := $(shell pwd)/build/nginx/sbin:$(PATH)

.PHONY: test
test:
	redis-cli flushdb
	prove t/*.t

compile:
	script/bootstrap compile

bootstrap:
	script/bootstrap

.PHONY: clean
clean:
	script/bootstrap clean
