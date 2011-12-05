require 'mkmf'

uname = `uname -s`.chomp.downcase

find_header("ares_config.h", File.expand_path("../config_#{uname}", __FILE__))
$defs.push "-DHAVE_CONFIG_H"
create_makefile('cares/cares')
