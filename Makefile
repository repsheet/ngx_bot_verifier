export PATH := $(shell pwd)/build/nginx/sbin:$(PATH)

test:
	prove t/*.t

compile:
	script/bootstrap compile

bootstrap:
	script/bootstrap

.PHONY: test
