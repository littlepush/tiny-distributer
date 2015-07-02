tiny-distributer
================

This is a tiny tcp socket redirect application.
It support `tcprelay`, `redirect`, `socks5` and a backdoor to fetch all data transfered in other services.

Compile & Install
=================

```bash
make release && make install
```

By default, `make` will create a debug version of `tinydst`

Configuration
=============

The configuration file is a JSON file.


Change logs
===========

* v0.1 sucks...but work.
* v0.2 Use new version `socklite`, support 4 different services

Reference
=========

JsonCpp
socklite
