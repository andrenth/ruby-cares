require 'socket'
require 'cares'

c = Cares.new

c.gethostbyname('www.google.com', Socket::AF_INET) do |name, aliases, f, *addrs|
  puts 'www.google.com:'
  puts "\tcanonical name:\t#{name}"
  puts "\taliases:\t#{aliases.join(', ')}"
  puts "\taddresses:\t#{addrs.join(', ')}"
end

c.gethostbyaddr('200.20.10.17', Socket::AF_INET) do |name, aliases, *rest|
  puts "200.20.10.17:"
  puts "\tname: #{name}"
end

c.select_loop
