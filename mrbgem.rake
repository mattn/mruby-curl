MRuby::Gem::Specification.new('mruby-curl') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'

  spec.linker.libraries << 'curl'

  spec.add_dependency 'mruby-http'
end
