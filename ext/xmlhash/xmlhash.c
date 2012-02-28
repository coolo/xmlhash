#include <ruby.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>

void processNode(xmlTextReaderPtr reader) {
  fprintf(stderr, "processNode\n");
  xmlChar *name, *value;
  int nodetype;

  name = xmlTextReaderName(reader);
  if (name == NULL)
    name = xmlStrdup(BAD_CAST "--");
  value = xmlTextReaderConstValue(reader);

  nodetype = xmlTextReaderNodeType(reader);
  
  printf("%d %d %s %d %d",
	 xmlTextReaderDepth(reader),
	 nodetype,
	 name,
	 xmlTextReaderIsEmptyElement(reader),
	 xmlTextReaderHasAttributes(reader)
	 );

  if (nodetype == XML_READER_TYPE_ATTRIBUTE)
    {
      printf("%s = %s\n", name, value);
    }

  xmlFree(name);
  if (value == NULL)
    printf("\n");
  else {
    printf(" %s\n", value);
  }

  xmlTextReaderMoveToFirstAttribute(reader);
  processAttribute(reader);
}

static VALUE parse_xml_hash(VALUE self, VALUE rb_xml)
{
  xmlChar * xdata;
  char *data;
  xmlTextReaderPtr reader;
  int ret;

  Check_Type(rb_xml, T_STRING);
 
  data = calloc(RSTRING_LEN(rb_xml), sizeof(char));
  xdata = xmlCharStrndup(StringValuePtr(rb_xml), RSTRING_LEN(rb_xml));
  (void)xdata;
  memcpy(data, StringValuePtr(rb_xml), RSTRING_LEN(rb_xml));

  reader = xmlReaderForMemory(data, RSTRING_LEN(rb_xml), NULL, NULL, XML_PARSE_NOENT);
  if (reader != NULL) {
    ret = xmlTextReaderRead(reader);
    while (ret == 1) {
      processNode(reader);
      ret = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
    if (ret != 0) {
      printf("%s : failed to parse\n", data);
    }
  }

  return rb_str_new2("wow");
}

void Init_xmlhash()
{
  LIBXML_TEST_VERSION
  VALUE mXmlhash = rb_define_module("Xmlhash");
  rb_define_singleton_method(mXmlhash, "parse", parse_xml_hash, 1);
}
