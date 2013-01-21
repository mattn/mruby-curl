MRuby::Gem::Specification.new('mruby-curl') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'
 
  if ENV['OS'] == 'Windows_NT'
    spec.linker.libraries << 'curldll'
  else
    spec.linker.libraries << 'curl'
  end
end
