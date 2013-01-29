MRuby::Gem::Specification.new('mruby-curl') do |spec|
  spec.license = 'MIT'
  spec.authors = 'mattn'
 
  spec.linker.libraries << 'curl'
end
