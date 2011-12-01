require 'mkmf'
require 'fileutils'

to_copy = Dir[File.expand_path("../c-ares/*.{c,h}", __FILE__)]
to_exclude = Dir[File.expand_path("../c-ares/{acountry.c,adig.c,ahost.c,config-dos.h}", __FILE__)]
FileUtils.cp_r(to_copy - to_exclude, File.expand_path("../", __FILE__))

Dir.chdir(File.expand_path("../", __FILE__)) do
  system("patch -p1 <ares.diff")
  system("patch -p1 <ares_setup.diff")
end

uname = `uname -s`.chomp.downcase
find_header("ares_config.h", File.expand_path("../config_#{uname}", __FILE__))
$defs.push "-DHAVE_CONFIG_H"
create_makefile('cares/cares')
