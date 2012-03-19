#include <assert.h>
#include <ruby.h>
#include <st.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>

struct XmlhashParserData 
{
  VALUE m_current;
  VALUE m_stack;
  VALUE m_cstring;
  VALUE m_result;
};

struct XmlhashParserData *init_XmlhashParserData()
{
  struct XmlhashParserData *newone = (struct XmlhashParserData *)malloc(sizeof(struct XmlhashParserData));
  memset(newone, 0, sizeof(struct XmlhashParserData));
  newone->m_current = Qnil;
  newone->m_stack = rb_ary_new();
  return newone;
}

void xml_hash_start_element(struct XmlhashParserData *self, const xmlChar *name)
{
  // needed for further attributes
  self->m_current = rb_hash_new();
  VALUE pair = rb_ary_new();
  rb_ary_push(pair, rb_str_new2((const char*)name));
  rb_ary_push(pair, self->m_current);
  rb_ary_push(self->m_stack, pair);
  self->m_cstring = rb_ary_new();
}

void xml_hash_end_element(struct XmlhashParserData *self, const xmlChar *name)
{
  VALUE pair = rb_ary_pop(self->m_stack);
  VALUE cname = rb_ary_entry(pair, 0);
  VALUE chash = rb_ary_entry(pair, 1);
  assert(!strcmp((const char*)name, RSTRING_PTR(cname)));

  if (rb_obj_is_kind_of(chash, rb_cHash) && RHASH_SIZE(chash) == 0) {
    // now check if the cstring array contains non-empty string
    VALUE string = rb_ary_join(self->m_cstring, Qnil);
    const char *string_ptr = RSTRING_PTR(string);
    long string_len = RSTRING_LEN(string);
    while (string_len > 0 && (string_ptr[0] == ' ' || string_ptr[0] == '\t' || string_ptr[0] == '\n')) {
      string_ptr++;
      string_len--;
    }
    while (string_len > 0 && (string_ptr[string_len-1] == ' ' || string_ptr[string_len-1] == '\t' || string_ptr[string_len-1] == '\n')) {
      string_len--;
    }
    // avoid overwriting empty hash with empty string
    if (string_len > 0)
      chash = string;
  }
  if (RARRAY_LEN(self->m_stack) == 0) {
    self->m_result = chash;
    return;
  }

  pair = rb_ary_entry(self->m_stack, RARRAY_LEN(self->m_stack)-1);
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

void xml_hash_add_attribute(struct XmlhashParserData *self, const xmlChar *name, const xmlChar *value)
{
  assert(self->m_current != Qnil);
  rb_hash_aset(self->m_current, rb_str_new2((const char*)name), rb_str_new2((const char*)value));
}

void xml_hash_add_text(struct XmlhashParserData *self, const xmlChar *text)
{
  rb_ary_push(self->m_cstring, rb_str_new2((const char*)text));
}

void processAttribute(struct XmlhashParserData *state, xmlTextReaderPtr reader) 
{
  const xmlChar *name = xmlTextReaderConstName(reader);
  assert(xmlTextReaderNodeType(reader) == XML_READER_TYPE_ATTRIBUTE);
  xml_hash_add_attribute(state, name, xmlTextReaderConstValue(reader));
}

void processNode(struct XmlhashParserData *state, xmlTextReaderPtr reader) 
{
  const xmlChar *name;
  const xmlChar *value;
  int nodetype;

  name = xmlTextReaderConstName(reader);
  value = xmlTextReaderConstValue(reader);

  nodetype = xmlTextReaderNodeType(reader);
  
  if (nodetype == XML_READER_TYPE_END_ELEMENT) {
    xml_hash_end_element(state, name);
    assert(value == NULL);
    return;
  }

  if (nodetype == XML_READER_TYPE_ELEMENT) {
    xml_hash_start_element(state, name);
    assert(value == NULL);

    if (xmlTextReaderMoveToFirstAttribute(reader) == 1)
      {
	processAttribute(state, reader);
	while (xmlTextReaderMoveToNextAttribute(reader) == 1)
	  processAttribute(state, reader);

	xmlTextReaderMoveToElement(reader);
      }

    if (xmlTextReaderIsEmptyElement(reader) == 1) {
      xml_hash_end_element(state, name);
    }
    return;
  }

  if (nodetype == XML_READER_TYPE_TEXT || 
      nodetype == XML_READER_TYPE_WHITESPACE || 
      nodetype == XML_READER_TYPE_SIGNIFICANT_WHITESPACE)
    {
      xml_hash_add_text(state, value);
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
  VALUE result;

  Check_Type(rb_xml, T_STRING);
 
  data = (char*)calloc(RSTRING_LEN(rb_xml), sizeof(char));
  xdata = xmlCharStrndup(StringValuePtr(rb_xml), RSTRING_LEN(rb_xml));
  (void)xdata;
  memcpy(data, StringValuePtr(rb_xml), RSTRING_LEN(rb_xml));

  reader = xmlReaderForMemory(data, RSTRING_LEN(rb_xml), 
			      NULL, NULL, XML_PARSE_NOENT);
  struct XmlhashParserData *state = init_XmlhashParserData();
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

  result = state->m_result;
  free(state);
  return result;
}

void Init_xmlhash()
{
  LIBXML_TEST_VERSION
  VALUE mXmlhash = rb_define_module("Xmlhash");
  rb_define_singleton_method(mXmlhash, "parse", &parse_xml_hash, 1);
}
