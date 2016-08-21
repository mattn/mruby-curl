= mruby-curl

== Threaded use

By default mruby-curl does not call
[curl_global_init](https://curl.haxx.se/libcurl/c/curl_global_init.html).  If
you are using mruby-curl in a multithreaded environment you must call it
yourself.

If threads are started from within mruby the `Curl.global_init` method will
initialize curl with the default flags.  You must call it before starting
threads that will use mruby-curl methods.

If mruby is started from a multi-threaded program you must call
`curl_global_init` before starting any mruby threads.

See the
[curl_global_init](https://curl.haxx.se/libcurl/c/curl_global_init.html)
documentation for more details.

