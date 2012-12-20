#!mruby

if ARGV.size != 1
  raise "gist.rb [GIST_TOKEN]"
end

req = HTTP::Request.new
req.method = "POST"
req.body = JSON::stringify({
  "description"=> "We love mruby!",
  "public"=> true,
  "files"=> {
    "file1.txt"=> {
      "content"=> "mruby is awesome!"
    }
  }
})
req.headers['Authorization'] = "token #{ARGV[0]}"
req.headers['Content-Type'] = "application/json"
Curl::SSL_VERIFYPEER = 0
res = Curl::send("https://api.github.com/gists", req)
puts JSON::parse(res.body)['html_url']
