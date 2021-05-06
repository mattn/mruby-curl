#!mruby

curl = Curl.new

headers = {
  'User-Agent' => 'mruby-curl',
}

curl.timeout_ms = 3000
response = curl.get("http://www.ruby-lang.org/ja/", headers)

puts response.body
