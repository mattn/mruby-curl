class Curl
  def self.delete(url, headers = nil, &block)
    @curl ||= new
    @curl.delete url, headers, &block
  end

  def self.get(url, headers = nil, &block)
    @curl ||= new
    @curl.get url, headers, &block
  end

  def self.patch(url, data, headers = nil, &block)
    @curl ||= new
    @curl.patch url, data, headers, &block
  end

  def self.post(url, data, headers = nil, &block)
    @curl ||= new
    @curl.post url, data, headers, &block
  end

  def self.put(url, data, headers = nil, &block)
    @curl ||= new
    @curl.put url, data, headers, &block
  end

  def self.send(url, req, headers = nil, &block)
    @curl ||= new
    @curl.send url, req, headers, &block
  end
end
