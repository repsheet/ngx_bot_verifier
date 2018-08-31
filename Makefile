export PATH := $(shell pwd)/build/nginx/sbin:$(PATH)

test:
	redis-cli flushdb
	prove t/*.t

compile:
	script/bootstrap compile

bootstrap:
	script/bootstrap

.PHONY: test
