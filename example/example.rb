#!mruby

puts Curl::get("http://www.ruby-lang.org/ja/", {"User-Agent"=>"curl"}).body
