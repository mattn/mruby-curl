#!mruby

if ARGV.size != 2
  raise "twitterstream.rb [USER] [PASSWORD]"
end

Curl::SSL_VERIFYPEER = 0
Curl::get("https://stream.twitter.com/1.1/statuses/sample.json", {"Authorization"=> "Basic #{Base64::encode(ARGV[0] + ":" + ARGV[1])}"}) do |h,b|
  begin
    puts JSON::parse(b)['text']
  rescue RuntimeError
  end
end
