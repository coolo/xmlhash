require 'mkmf'
unless find_library('xml2', 'xmlAddID')
  abort "xml2 is missing.  please install libxml2"
end
create_makefile('xmlhash/xmlhash')
