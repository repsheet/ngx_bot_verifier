# NGINX Search Index Bot Verification Module [![Build Status](https://secure.travis-ci.org/abedra/ngx_bot_verifier.png)](http://travis-ci.org/abedra/ngx_bot_verifier?branch=master) ![Coverity Build Status](https://scan.coverity.com/projects/16736/badge.svg)

## Table of Contents

* [Status](#status)
* [Version](#version)
* [Synopsis](#synopsis)
* [Description](#description)
* [Directives](#directives)
  * [bot_verifier](#bot_verifier)
  * [bot_verifier_redis_host](#bot_verifier_redis_host)
  * [bot_verifier_redis_port](#bot_verifier_redis_port)
  * [bot_verifier_redis_connection_timeout](#bot_verifier_redis_connection_timeout)
  * [bot_verifier_redis_read_timeout](#bot_verifier_redis_read_timeout)
  * [bot_verifier_redis_expiry](#bot_verifier_redis_expiry)
  * [bot_verifier_repsheet_enabled](#bot_verifier_repsheet_enabled)
* [Installation](#installation)
* [Verifying Functionality](#verifying-functionality)
* [Developer Setup](#developer-setup)
* [Running the Test Suite](#running-the-test-suite)

## Status

[BETA] This module has been tested on a handful of production websites. It has not yet been evaluated at scale. If you would like to consider testing this at scale I would be happy to assist and allocate time to correct any issues.

[Back to TOC](#table-of-contents)

## Version

This document describes version [0.0.5](https://github.com/abedra/ngx_bot_verifier/tags) released on 9/12/2018.

[Back to TOC](#table-of-contents)

## Synopsis

```nginx
location / {
    bot_verifier on;
    bot_verifier_redis_host localhost;
    bot_verifier_redis_port 6379;
    bot_verifier_redis_connection_timeout 10;
    bot_verifier_redis_read_timeout 10;
    bot_verifier_redis_expiry 3600;
	bot_verifier_repsheet_enabled on;
}
```

[Back to TOC](#table-of-contents)

## Description

This is an NGINX module designed to validate actors claiming to be search engine indexers. It is right to disable security mechanisms for valid search engine bots to ensure your controls do not interfere with page rankings on any of the search providers. The issue is that the [User Agent](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/User-Agent) header cannot be trusted. In order to ensure you are allowing only valid search engine indexers, you must validate according to their published standards. This module performs that validation and caches the results to ensure you do not pay validation penalties on every request.

[Back to TOC](#table-of-contents)

Directives
==========

The following directives are used only for module configuration.

[Back to TOC](#table-of-contents)

bot_verifier
------------

**syntax:** *bot_verifier* \[on|off\]

**default:** *off*

**context:** *location*

**phase:** *access*

Enables or disables the module. The module will not act unless it is set to *on*.

[Back to TOC](#table-of-contents)

bot_verifier_redis_host
-----------------------

**syntax:** *bot_verifier_redis_host* &lt;string&gt;

**default:** *localhost*

**context:** *location*

**phase:** *access*

Sets the Redis host. This setting is used to connect to the Redis database used for caching lookup results.

[Back to TOC](#table-of-contents)

bot_verifier_redis_port
-----------------------

**syntax:** *bot_verifier_redis_port* &lt;int&gt;

**default:** *6379*

**context:** *location*

**phase:** *access*

Sets the Redis port. This setting is used to connect to the Redis database used for caching lookup results.

[Back to TOC](#table-of-contents)

bot_verifier_redis_connection_timeout
-------------------------------------

**syntax:** *bot_verifier_redis_connection_timeout* &lt;int&gt;

**default:** *10*

**context:** *location*

**phase:** *access*

Sets the timeout when connecting to Redis. This setting is used to connect to the Redis database used for caching lookup results.

[Back to TOC](#table-of-contents)

bot_verifier_redis_read_timeout
-------------------------------------

**syntax:** *bot_verifier_redis_read_timeout* &lt;int&gt;

**default:** *10*

**context:** *location*

**phase:** *access*

Sets the timeout when querying Redis. This setting is used to connect to the Redis database used for caching lookup results.

[Back to TOC](#table-of-contents)

bot_verifier_redis_expiry
-------------------------

**syntax:** *bot_verifier_redis_expiry* &lt;seconds&gt;

**default:** *3600*

**context:** *location*

**phase:** *access*

Sets the timeout when querying Redis. This setting is used to connect to the Redis database used for caching lookup results.

[Back to TOC](#table-of-contents)

bot_verifier_repsheet_enabled
-------------------------

**syntax:** *bot_verifier_repsheet_enabled* \[on|off\]

**default:** *off*

**context:** *location*

**phase:** *access*

Enables blacklisting of failed actors in Repsheet. Assumes Repsheet cache lives on already configured redis server.

[Back to TOC](#table-of-contents)

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

```nginx
load_module modules/ngx_http_bot_verifier_module.so;
```

This will ensure the module is loaded and available for use.

[Back to TOC](#table-of-contents)

## Verifying Functionality

In order to ensure the module is working properly you will need to issue a query that will trigger failure and success cases. To trigger a failure case issue the following request:

```
curl -A "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html" localhost:8888
```

This will issue a query that identifies itself as a Google bot. The reverse and forward lookup routine will fail and you will get a `403` response. To ensure the verification works when a bot is identified issue the following request:

```
curl -H "X-Forwarded-For: 66.249.66.1" -A "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html" localhost:8888
```

This will spoof the `X-Forwarded-For` header and pretend to be from a valid google address. The request should succeed and return a normal response.

[Back to TOC](#table-of-contents)

## Developer Setup

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

[Back to TOC](#table-of-contents)

## Running the Test Suite

This repository comes with a test suite that uses the `Test::Nginx` library. To run the test you will need to install the following libraries:

```
cpanm -S install Test::Nginx Test::Nginx::Socket
```

Once the libraries are installed just run `make` and the suite will run. If you are submitting a change to this module please make sure to run the test suite before you do. Any changes that break the test suite will not be accepted.

[Back to TOC](#table-of-contents)
