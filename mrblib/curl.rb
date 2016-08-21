class Curl
  def self.delete(url, headers = nil, &block)
    new.delete url, headers, &block
  end

  def self.get(url, headers = nil, &block)
    new.get url, headers, &block
  end

  def self.patch(url, data, headers = nil, &block)
    new.patch url, data, headers, &block
  end

  def self.post(url, data, headers = nil, &block)
    new.post url, data, headers, &block
  end

  def self.put(url, data, headers = nil, &block)
    new.put url, data, headers, &block
  end

  def self.send(url, req, headers = nil, &block)
    new.send url, req, headers, &block
  end
end
