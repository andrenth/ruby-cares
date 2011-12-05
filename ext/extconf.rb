require 'mkmf'
require 'fileutils'

to_exclude = Dir["c-ares/{acountry.c,adig.c,ahost.c,config-dos.h}"]
FileUtils.cp_r(Dir["c-ares/*.{c,h}"] - to_exclude, File.expand_path("../", __FILE__))

system("patch -p1 <ares.diff")
system("patch -p1 <ares_setup.diff")
uname = `uname -s`.chomp.downcase

find_header("ares_config.h", File.expand_path("../config_#{uname}", __FILE__))
$defs.push "-DHAVE_CONFIG_H"
create_makefile('cares/cares')
