require 'mkmf'

if find_library('cares', 'ares_init') and have_header('ares.h')
  create_makefile('cares')
end
