require "test/unit"
require "xmlhash"

class TestXmlhash < Test::Unit::TestCase
  def test_xml
    ret = Xmlhash.parse("<hello who='world'/>" )
    assert_equal ret, { "hello" => { "who" => "world" } }
  end
end
