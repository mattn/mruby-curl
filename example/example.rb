#!mruby

curl = Curl.new

headers = {
  'User-Agent' => 'mruby-curl',
}

response = curl.get("http://www.ruby-lang.org/ja/", headers)

puts response.body
