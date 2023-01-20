require 'mkmf'
require 'pkg-config'
unless find_library('xml2', 'xmlAddID')
  abort "xml2 is missing.  please install libxml2"
end
$CFLAGS << ' ' + PackageConfig.new('libxml-2.0').cflags
$CFLAGS += " -Werror -Wall "
create_makefile('xmlhash/xmlhash')
