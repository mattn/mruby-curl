= mruby-curl

mruby-curl is an [mruby](http://mruby.org) wrapper for
[libcurl](https://curl.haxx.se/libcurl/).

== Usage

Example:

```ruby
curl = Curl.new

headers = {
  'User-Agent' => 'mruby-curl'
}

response = curl.get("http://www.ruby-lang.org/ja/", headers)

puts response.body
```

mruby-curl has support for HTTP methods DELETE, GET, PATCH, POST, and PUT
through instance methods on the Curl object and supports arbitrary HTTP
requests using `Curl#send` with an `HTTP::Request` object from
[mruby-http](https://github.com/mattn/mruby-http).

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

