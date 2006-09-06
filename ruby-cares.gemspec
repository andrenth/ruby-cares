require 'rubygems'

spec = Gem::Specification.new do |s|
  s.name              = 'ruby-cares'
  s.version           = '0.1.0'
  s.platform          = Gem::Platform::LINUX_586
  s.summary           = 'A Ruby extension for c-ares'
  s.requirements      = %q{c-ares}
  s.files             = Dir.glob('*/**')
  s.extensions        = %q{ext/extconf.rb}
  s.autorequire       = 'cares'
  s.has_rdoc          = false
  s.author            = 'Andre Nathan'
  s.email             = 'andre@digirati.com.br'
  s.rubyforge_project = 'cares'
  s.homepage          = 'http://cares.rubyforge.org'
  s.description = <<-EOF
    Ruby/Cares is a C extension for the c-ares library, Providing an
    asynchronous DNS resolver to be used with ruby scripts.
  EOF
end

if __FILE__ == $0
  Gem.manage_gems
  Gem::Builder.new(spec).build
end
