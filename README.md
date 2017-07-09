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

## Configuration Directives

The following configuration directives are available:

`bot_verifier <on|off>` - Enables or disables the module  

## Local Setup

This module contains a full self-contained development environment. This is done to ensure work on the module does not interfere with any other NGINX installations. To setup the environment run the `script/bootstrap` command. This will create the following directories:

```
vendor - The NGINX installation will live here  
build - The NGINX install will live here  
```

The `nginx.conf` file in the root of this repository will be symlinked to `build/nginx/conf/nginx.conf` to make configuration changes easier. You can start NGINX using the following command:

```
build/nginx/sbin/nginx
```

Log files are available at `build/nginx/logs`. You can stop the server by running

```
build/nginx/sbin/nginx -s stop
```

If you are making changes to the module, you can recompile them by running `make compile`. Remember to restart the NGINX after this completes successfully.

## Running the test suite

This repository comes with a test suite that uses the `Test::Nginx` library. To run the test you will need to install the following libraries:

```
cpan Test::Nginx Test::Nginx::Socket
```

Once the libraries are installed just run `make` and the suite will run. If you are submitting a change to this module please make sure to run the test suite before you do. Any changes that break the test suite will not be accepted.

