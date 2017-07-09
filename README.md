# NGINX Search Index Bot Verification Module ![Build Status](https://circleci.com/gh/abedra/ngx_bot_verifier.svg?style=shield&circle-token=b1f4c421b282e62c253ca3aa8b5bcacec114e2bc)

## Installation

You can add this module to the static build of NGINX or as a dynamic module. To add as a static module add the following line to the `configure` command when compiling NGINX.

```
./configure --add-module=<path/to/ngx_bot_verifier>
```

If you wish to add this module as a dynamic module, change the argument to `configure` to the following:

```
./configure --add-dynamic-module=<path/to/ngx_bot_verifier>
```

You will then have to run `make modules` after you run `make` to ensure the module gets compiled and installed. You will also need to add a line to you `nginx.conf` telling it where to find the module. This must be added in the `MAIN CONF` section.

```
load_module modules/ngx_http_bot_verifier_module.so;
```

This will ensure the module is loaded and available for use.
