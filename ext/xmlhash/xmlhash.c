#include <assert.h>

/* libxml headers first - see https://github.com/coolo/xmlhash/issues/1 */
#include <libxml/parser.h>
#include <libxml/xmlreader.h>

#include <ruby.h>
#ifdef HAVE_RUBY_ST_H
# include <ruby/st.h>
#else
# include <st.h>
#endif

/* API_VERSION_CODE is only defined in those we want */
#ifdef HAVE_RUBY_ENCODING_H
# include <ruby/encoding.h>
#endif

static VALUE m_current = Qnil;
static VALUE m_stack = Qnil;
static VALUE m_cstring = Qnil;
static VALUE m_result = Qnil;
static VALUE m_xmlClass = Qnil;
#ifdef HAVE_RUBY_ENCODING_H
static rb_encoding *m_current_encoding = NULL;
#endif

void init_XmlhashParserData()
{
  m_current = Qnil;
  rb_ary_clear(m_stack);
  rb_ary_clear(m_cstring);
}

void xml_hash_start_element(const xmlChar *name)
{
  VALUE pair;
  /* needed for further attributes */
  m_current = rb_class_new_instance(0, 0, m_xmlClass);
  pair = rb_ary_new();
  rb_ary_push(pair, rb_str_new2((const char*)name));
  rb_ary_push(pair, m_current);
  rb_ary_push(m_stack, pair);
  rb_ary_clear(m_cstring);
}

void xml_hash_end_element(const xmlChar *name)
{
  VALUE pair, cname, chash, phash, obj;

  assert(m_stack != Qnil);
  pair = rb_ary_pop(m_stack);
  assert(pair != Qnil);
  cname = rb_ary_entry(pair, 0);
  chash = rb_ary_entry(pair, 1);
  assert(!strcmp((const char*)name, RSTRING_PTR(cname)));

  if (rb_obj_is_kind_of(chash, rb_cHash) && RHASH_SIZE(chash) == 0) {
    VALUE string;
    const char *string_ptr;
    long string_len;

    /* now check if the cstring array contains non-empty string */
    string = rb_ary_join(m_cstring, Qnil);
    string_ptr = RSTRING_PTR(string);
    string_len = RSTRING_LEN(string);
    while (string_len > 0 && (string_ptr[0] == ' ' || string_ptr[0] == '\t' || string_ptr[0] == '\n')) {
      string_ptr++;
      string_len--;
    }
    while (string_len > 0 && (string_ptr[string_len-1] == ' ' || string_ptr[string_len-1] == '\t' || string_ptr[string_len-1] == '\n')) {
      string_len--;
    }
    /* avoid overwriting empty hash with empty string */
    if (string_len > 0)
      chash = string;
  }
  if (RARRAY_LEN(m_stack) == 0) {
    m_result = chash;
    return;
  }

  pair = rb_ary_entry(m_stack, RARRAY_LEN(m_stack)-1);
  phash = rb_ary_entry(pair, 1);

  obj = rb_hash_aref(phash, cname);
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
    /* implement force_array */
    rb_hash_aset(phash, cname, chash);
  }
}

void xml_hash_add_attribute(const xmlChar *name, const xmlChar *value)
{
#ifdef HAVE_RUBY_ENCODING_H
  VALUE v_name, v_value;
#endif

  assert(m_current != Qnil);
#ifdef HAVE_RUBY_ENCODING_H
  v_name = rb_external_str_new_with_enc((const char*)name, xmlStrlen(name), m_current_encoding);
  v_value = rb_external_str_new_with_enc((const char*)value, xmlStrlen(value), m_current_encoding);
  rb_hash_aset(m_current, v_name, v_value);
#else
  rb_hash_aset(m_current, rb_str_new2((const char*)name), rb_str_new2((const char*)value));
#endif
}

void xml_hash_add_text(const xmlChar *text)
{
#ifdef HAVE_RUBY_ENCODING_H
  VALUE str;
  str = rb_external_str_new_with_enc((const char*)text, xmlStrlen(text), m_current_encoding);
  rb_ary_push(m_cstring, str);
#else
  rb_ary_push(m_cstring, rb_str_new2((const char*)text));
#endif
}

void processAttribute(xmlTextReaderPtr reader) 
{
  const xmlChar *name = xmlTextReaderConstName(reader);
  assert(xmlTextReaderNodeType(reader) == XML_READER_TYPE_ATTRIBUTE);
  xml_hash_add_attribute(name, xmlTextReaderConstValue(reader));
}

void processNode(xmlTextReaderPtr reader) 
{
  const xmlChar *name;
  const xmlChar *value;
  int nodetype;

  name = xmlTextReaderConstName(reader);
  value = xmlTextReaderConstValue(reader);

  nodetype = xmlTextReaderNodeType(reader);
  
  if (nodetype == XML_READER_TYPE_END_ELEMENT) {
    xml_hash_end_element(name);
    assert(value == NULL);
    return;
  }

  if (nodetype == XML_READER_TYPE_ELEMENT) {
    xml_hash_start_element(name);
    assert(value == NULL);

    if (xmlTextReaderMoveToFirstAttribute(reader) == 1)
      {
	processAttribute(reader);
	while (xmlTextReaderMoveToNextAttribute(reader) == 1)
	  processAttribute(reader);

	xmlTextReaderMoveToElement(reader);
      }

    if (xmlTextReaderIsEmptyElement(reader) == 1) {
      xml_hash_end_element(name);
    }
    return;
  }

  if (nodetype == XML_READER_TYPE_TEXT || 
      nodetype == XML_READER_TYPE_WHITESPACE || 
      nodetype == XML_READER_TYPE_SIGNIFICANT_WHITESPACE)
    {
      xml_hash_add_text(value);
      return;
    }

  printf("%d %s\n",
	 nodetype,
	 name
	 );

}

static VALUE parse_xml_hash(VALUE self, VALUE rb_xml)
{
  char *data;
  xmlTextReaderPtr reader;
  int ret;

  Check_Type(rb_xml, T_STRING);
#ifdef HAVE_RUBY_ENCODING_H
  m_current_encoding = rb_enc_get(rb_xml);
#endif

  data = (char*)calloc(RSTRING_LEN(rb_xml), sizeof(char));
  memcpy(data, StringValuePtr(rb_xml), RSTRING_LEN(rb_xml));

  reader = xmlReaderForMemory(data, RSTRING_LEN(rb_xml), 
			      NULL, NULL, XML_PARSE_NOENT);
  init_XmlhashParserData();
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

  free(data);
#ifdef HAVE_RUBY_ENCODING_H
  m_current_encoding = 0;
#endif
  return m_result;
}

void Init_xmlhash()
{
  VALUE mXmlhash;

  LIBXML_TEST_VERSION
  mXmlhash = rb_define_module("Xmlhash");
  m_xmlClass = rb_define_class_under(mXmlhash, "XMLHash", rb_cHash);
  rb_define_singleton_method(mXmlhash, "parse", &parse_xml_hash, 1);
  m_stack = rb_ary_new();
  rb_global_variable(&m_stack);
  m_cstring = rb_ary_new();
  rb_global_variable(&m_cstring);
  rb_global_variable(&m_result);
  rb_global_variable(&m_current);
}
