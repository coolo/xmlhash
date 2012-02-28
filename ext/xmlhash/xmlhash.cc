#include <cassert>
#include <ruby.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>

class XmlhashParserData 
{
public:
  XmlhashParserData();
  void start_element(const xmlChar *);
  void end_element(const xmlChar *);
  void add_attribute(const xmlChar *, const xmlChar *);
  void add_text(const xmlChar *text);
  
  VALUE result() { return m_result; }
private:
  VALUE m_current;
  VALUE m_stack;
  VALUE m_cstring;
  VALUE m_result;
};

XmlhashParserData::XmlhashParserData()
{
  m_current = Qnil;
  m_stack = rb_ary_new();
}

void XmlhashParserData::start_element(const xmlChar *name)
{
  // needed for further attributes
  m_current = rb_hash_new();
  VALUE pair = rb_ary_new();
  rb_ary_push(pair, rb_str_new2((const char*)name));
  rb_ary_push(pair, m_current);
  rb_ary_push(m_stack, pair);
  m_cstring = rb_ary_new();
}

void XmlhashParserData::end_element(const xmlChar *name)
{
  VALUE pair = rb_ary_pop(m_stack);
  VALUE cname = rb_ary_entry(pair, 0);
  VALUE chash = rb_ary_entry(pair, 1);
  assert(!strcmp((const char*)name, RSTRING(cname)->ptr));

  if (rb_obj_is_kind_of(chash, rb_cHash)) {
    printf("UND NUN %s\n", name);
  }
    //  if chash == {}
    //    chash = @cstring.join
    //    # empty string is nonsense
    //    if chash.strip.empty?
    // 	 chash = {}
    //    end
    //  end
  
  if (RARRAY_LEN(m_stack) == 0) {
    m_result = chash;
    return;
  }

  pair = rb_ary_entry(m_stack, RARRAY_LEN(m_stack)-1);
  //VALUE pname = rb_ary_entry(pair, 0);
  VALUE phash = rb_ary_entry(pair, 1);

  VALUE obj = rb_hash_aref(phash, cname);
  if (obj != Qnil) {
    if (rb_obj_is_kind_of(obj, rb_cArray)) {
      rb_ary_push(obj, chash);
    } else {
      VALUE nobj = rb_ary_new();
      rb_ary_push(nobj, obj);
      rb_ary_push(nobj, chash);
      rb_hash_aset(phash, cname, nobj);
    }
  } else {
    // implement force_array
    rb_hash_aset(phash, cname, chash);
  }
}

void XmlhashParserData::add_attribute(const xmlChar *name, const xmlChar *value)
{
  assert(m_current != Qnil);
  rb_hash_aset(m_current, rb_str_new2((const char*)name), rb_str_new2((const char*)value));
}

void XmlhashParserData::add_text(const xmlChar *text)
{
  rb_ary_push(m_cstring, rb_str_new2((const char*)text));
}

void processAttribute(XmlhashParserData &state, xmlTextReaderPtr reader) 
{
  const xmlChar *name = xmlTextReaderConstName(reader);
  assert(xmlTextReaderNodeType(reader) == XML_READER_TYPE_ATTRIBUTE);
  state.add_attribute(name, xmlTextReaderConstValue(reader));
}

void processNode(XmlhashParserData &state, xmlTextReaderPtr reader) 
{
  const xmlChar *name;
  const xmlChar *value;
  int nodetype;

  name = xmlTextReaderConstName(reader);
  value = xmlTextReaderConstValue(reader);

  nodetype = xmlTextReaderNodeType(reader);
  
  if (nodetype == XML_READER_TYPE_END_ELEMENT) {
    state.end_element(name);
    assert(value == NULL);
    return;
  }

  if (nodetype == XML_READER_TYPE_ELEMENT) {
    state.start_element(name);
    assert(value == NULL);

    if (xmlTextReaderMoveToFirstAttribute(reader) == 1)
      {
	processAttribute(state, reader);
	while (xmlTextReaderMoveToNextAttribute(reader) == 1)
	  processAttribute(state, reader);

	xmlTextReaderMoveToElement(reader);
      }

    if (xmlTextReaderIsEmptyElement(reader) == 1) {
      state.end_element(name);
    }
    return;
  }

  // Enum xmlReaderTypes {
//     XML_READER_TYPE_NONE = 0
//     XML_READER_TYPE_ELEMENT = 1
//     XML_READER_TYPE_ATTRIBUTE = 2
//     XML_READER_TYPE_TEXT = 3
//     XML_READER_TYPE_CDATA = 4
//     XML_READER_TYPE_ENTITY_REFERENCE = 5
//     XML_READER_TYPE_ENTITY = 6
//     XML_READER_TYPE_PROCESSING_INSTRUCTION = 7
//     XML_READER_TYPE_COMMENT = 8
//     XML_READER_TYPE_DOCUMENT = 9
//     XML_READER_TYPE_DOCUMENT_TYPE = 10
//     XML_READER_TYPE_DOCUMENT_FRAGMENT = 11
//     XML_READER_TYPE_NOTATION = 12
//     XML_READER_TYPE_WHITESPACE = 13
//     XML_READER_TYPE_SIGNIFICANT_WHITESPACE = 14
//     XML_READER_TYPE_END_ELEMENT = 15
//     XML_READER_TYPE_END_ENTITY = 16
//     XML_READER_TYPE_XML_DECLARATION = 17
// }

  if (nodetype == XML_READER_TYPE_TEXT || nodetype == XML_READER_TYPE_WHITESPACE || nodetype == XML_READER_TYPE_SIGNIFICANT_WHITESPACE)
    {
      state.add_text(value);
      return;
    }

  printf("%d %s\n",
	 nodetype,
	 name
	 );

}

static VALUE parse_xml_hash(VALUE self, VALUE rb_xml)
{
  xmlChar * xdata;
  char *data;
  xmlTextReaderPtr reader;
  int ret;

  Check_Type(rb_xml, T_STRING);
 
  data = (char*)calloc(RSTRING_LEN(rb_xml), sizeof(char));
  xdata = xmlCharStrndup(StringValuePtr(rb_xml), RSTRING_LEN(rb_xml));
  (void)xdata;
  memcpy(data, StringValuePtr(rb_xml), RSTRING_LEN(rb_xml));

  reader = xmlReaderForMemory(data, RSTRING_LEN(rb_xml), 
			      NULL, NULL, XML_PARSE_NOENT);
  XmlhashParserData state;
  if (reader != NULL) {
    ret = xmlTextReaderRead(reader);
    while (ret == 1) {
      processNode(state, reader);
      ret = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
    if (ret != 0) {
      printf("%s : failed to parse\n", data);
    }
  }

  return state.result();
}

typedef VALUE (RubyMethod)(...);

extern "C" void Init_xmlhash()
{
  LIBXML_TEST_VERSION
  VALUE mXmlhash = rb_define_module("Xmlhash");
  rb_define_singleton_method(mXmlhash, "parse", (RubyMethod*)&parse_xml_hash, 1);
}
