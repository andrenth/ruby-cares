require 'socket'
require 'cares'

domain = 'www.rubyforge.org'
addr = '205.234.109.18'
port = 25

cares = Cares.new

cares.gethostbyname(domain, Socket::AF_INET) do |name, aliases, family, *addrs|
  puts "[-] Cares#gethostbyname:"
  puts "  #{domain}:"
  puts "    canonical name: #{name}"
  puts "    aliases: #{aliases.join(', ')}"
  puts "    addresses: #{addrs.join(', ')}"
  puts
end

cares.gethostbyaddr(addr, Socket::AF_INET) do |name, *rest|
  puts "[-] Cares#gethostbyaddr:"
  puts "  #{addr}:"
  puts "    name: #{name}"
  puts
end

cares.getnameinfo(:addr => addr, :port => port) do |name, service|
  puts "[-] Cares#getnameinfo:"
  puts "  IP #{addr} is #{name}"
  puts "  Port #{port} is #{service}"
end

cares.select_loop
