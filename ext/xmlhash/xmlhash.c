#include <ruby.h>

static VALUE parse_xml_hash(VALUE mod)
{
  return rb_str_new2("hello world");
}

void Init_xmlhash()
{
  VALUE mXmlhash = rb_define_module("Xmlhash");
  rb_define_singleton_method(mXmlhash, "parse", parse_xml_hash, 1);
}
