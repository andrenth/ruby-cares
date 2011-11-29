# -*- encoding: utf-8 -*-
$:.push File.expand_path("../lib", __FILE__)
require 'cares/version'

spec = Gem::Specification.new do |s|
  s.name              = 'ruby-cares'
  s.version           = Cares::VERSION
  s.summary           = 'A Ruby extension for c-ares'
  s.has_rdoc          = false
  s.authors           = ['Andre Nathan', 'David Albert']
  s.email             = 'andre@digirati.com.br'
  s.rubyforge_project = 'cares'
  s.homepage          = 'http://cares.rubyforge.org'
  s.description = <<-EOF
    Ruby/Cares is a C extension for the c-ares library, Providing an
    asynchronous DNS resolver to be used with ruby scripts.
  EOF

  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- {test,spec,features}/*`.split("\n")
  s.executables   = `git ls-files -- bin/*`.split("\n").map{ |f| File.basename(f) }
  s.require_paths = ["lib"]
  s.extensions    = ["ext/c-ares/extconf.rb"]

  s.add_development_dependency "rake-compiler"
end
