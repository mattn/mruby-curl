#!mruby

url = "http://ajax.googleapis.com/ajax/services/search/web?v=1.0&q=mruby&rsz=large"
for result in JSON::parse(Curl.new.get(url).body)['responseData']['results']
	puts "#{result['titleNoFormatting']}\n  #{result['url']}"
end
