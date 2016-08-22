#!mruby

if ARGV.size != 2
  raise "twitterstream.rb [USER] [PASSWORD]"
end

Curl::SSL_VERIFYPEER = 0
Curl.new.get("https://stream.twitter.com/1.1/statuses/sample.json", {"Authorization"=> "Basic #{Base64::encode(ARGV[0] + ":" + ARGV[1])}"}) do |h,b|
  begin
    tweet = JSON::parse(b)
    puts Iconv.conv("char", "utf-8", "#{tweet['user']['screen_name']}: #{tweet['text']}") if tweet.has_key?('text')
  rescue RuntimeError, NameError
    puts "#{tweet['user']['screen_name']}: #{tweet['text']}" if tweet.has_key?('text')
  rescue ArgumentError
  end
end
