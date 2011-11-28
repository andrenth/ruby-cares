require "bundler/gem_tasks"
require 'rake/extensiontask'

Rake::ExtensionTask.new('cares') do |ext|
  ext.lib_dir = "lib/cares"
  ext.ext_dir = "ext"
end

desc "load ruby-cares in a pry or irb session (alias `rake c`)"
task :console do
  if system("which pry")
    repl = "pry"
  else
    repl = "irb"
  end

  sh "#{repl} -Ilib -rubygems -rcares"
end

task :c => :console
