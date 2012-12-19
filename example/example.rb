#!mruby

puts Curl::get("http://www.ruby-lang.org/ja/").body
