tiny-distributer V0.4
=====================

This is a tiny tcp socket redirect application.
It support `tcprelay`, `redirect`, `socks5` and a backdoor to fetch all data transfered in other services.

Compile & Install
=================

```bash
make release && make install
```

By default, `make` will create a debug version of `tinydst`

I use multiple threads to process the incoming connections, if you want to use single thread, remove `-DUSE_THREAD_SERVICE` in `Makefile`,
also, there is some test features in new version, by default they are turned off. Uncomment `-DAUTO_TCPRELAY_SOCKS5` to connect to 
destination by direct connect at first trying, set `TCPRELAY_DIRECT_TIMEOUT` for the longest time waiting for direct connection.

Configuration
=============

The configuration file is a JSON file.

```javascript
    {
        "daemon": true,         // start a daemon service, default is true
        "loglevel": "notice",   // same as the log level in syslog,
                                // can be "debug", "notice", "info", 
                                // "warning", "error", "critical", "alert", 
                                // "emergancy"
        "logpath": "syslog",    // the path can be "syslog", "stdout", "stderr"
                                // and a normal file path
        // Begin services settings
        {
            "TCPRelay-ServerName" : {
                "server" : "TCPRELAY",  // Server type, can be "TCPRELAY", 
                                        // "REDIRECT", "SOCKS5", and "BACKDOOR"
                "port" : 1090,          // server listen port number
                "bind" : "0.0.0.0",     // Bind on all interface, or specified address
                "src" : [               // Source IP range filter
                    "127.0.0.1",
                    "192.168.1.0/24"
                ],
                "socks5" : {            // Redirect use a socks5 proxy
                    "addr": "127.0.0.1",
                    "port": 1080
                }
            },
            "REDIRECT-ServerName" : {
                "server" : "REDIRECT",
                "port" : 1091,
                "dst" : [               // Try to redirect destination
                    {
                        /*
                            if the first dst is not reachable, then use the second
                        */
                        "addr" : "8.8.8.8",
                        "port" : 443,
                        "socks5" : {    // this dst need a socks proxy
                            "addr" : "127.0.0.1",
                            "port" : 1080
                        }
                    },
                    {
                        "addr" : "8.8.4.4",
                        "port" : 443
                    }
                ]
            }
        }
    }
```

More configuration file settings can read the `example.json`.

Change logs
===========

* v0.1 sucks...but work.
* v0.2 Use new version `socklite`, support 4 different services
* v0.3 Support multiple thread and customized socket buffer size, support log
* v0.4 
    * Fix bugs: 
        * failed to open log file and crash
        * IP Range filter
    * test new features

Todo
====

* v0.5 Maintain socks5 proxy white list, make auto tcprelay socks proxy selecting the default action

Reference
=========

* [JsonCpp](https://github.com/open-source-parsers/jsoncpp): A C++ library for interacting with JSON.
* [socklite](https://github.com/littlepush/socklite): An easy socket library for Linux and Mac OS in C++

License
=======
See the `LICENSE` file for details. In summary, tinydst is licensed under GPL v3.
