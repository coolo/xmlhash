# encoding: UTF-8

require "minitest/autorun"
require "xmlhash"
require 'json'
require 'thread'

Minitest::Test.make_my_diffs_pretty!

Xml = <<~eos
  <request id="93651">
    <action type="submit">
      <source project="server:dns" package="pdns" rev="65"/>
      <target project="openSUSE:Factory" package="pdns"/>
    </action>
    <state name="revoked" who="coolo" when="2011-12-19T13:20:50">
      <comment/>
    </state>
    <review state="accepted" by_group="legal-auto" who="licensedigger" when="2011-11-25T15:09:55">
      <comment>Big comment</comment>
    </review>
    <review state="new" by_group="factory-auto"/>
    <history name="review" who="coolo" when="2011-11-25T15:02:53"/>
    <history name="declined" who="coolo" when="2011-11-25T16:17:30">
      <comment>please make sure to wait before these depencencies are in openSUSE:Factory: libopendbx-devel, libopendbx1, libopendbxplus1, opendbx-backend-pgsql</comment>
    </history>
    <description>update and factory fix (forwarded request 86230 from -miska-)</description>
  </request>
eos

Output = { "history" =>
              [{ "name" => "review", "when" => "2011-11-25T15:02:53", "who" => "coolo" },
               { "comment" => "please make sure to wait before these depencencies are in openSUSE:Factory: libopendbx-devel, libopendbx1, libopendbxplus1, opendbx-backend-pgsql",
                 "name" => "declined", "when" => "2011-11-25T16:17:30", "who" => "coolo" }],
           "review" =>
              [
                { "comment" => "Big comment",
                  "by_group" => "legal-auto",
                  "when" => "2011-11-25T15:09:55",
                  "who" => "licensedigger",
                  "state" => "accepted" },
                { "by_group" => "factory-auto",
                  "state" => "new" }
              ], "action" => { "type" => "submit", "target" => { "project" => "openSUSE:Factory", "package" => "pdns" }, "source" => { "rev" => "65", "project" => "server:dns", "package" => "pdns" } }, "id" => "93651", "description" => "update and factory fix (forwarded request 86230 from -miska-)", "state" => { "comment" => {}, "name" => "revoked", "when" => "2011-12-19T13:20:50", "who" => "coolo" } }

class TestXmlhash < Minitest::Test
  def test_xml
    1000.times { |i|
      ret = Xmlhash.parse(Xml)
      GC.start
      assert_equal ret, Output
    }

    10000.times {
      ret = Xmlhash.parse(Xml)
      assert_equal ret, Output
    }
  end

  def test_threading
    counter = Array.new(10, 100)
    threads = []
    10.times do |t|
      threads << Thread.new do
        while counter[t] > 0 do
          ret = Xmlhash.parse(Xml)
          counter[t] -= 1
          assert_equal ret, Output
        end
      end
    end
    threads.each { |thr| thr.join }
  end

  def test_entry
    xml = <<~eos
      <?xml version='1.0' encoding='UTF-8'?>
      <directory count="4">
         <entry name="Apache"/>
         <entry name="Apache:APR_Pool_Debug"/>
         <entry name="Apache:MirrorBrain"/>
         <entry name="Apache:Modules"/>
      </directory>
    eos

    rubyoutput = { "count" => "4",
                   "entry" =>
                      [{ "name" => "Apache" },
                       { "name" => "Apache:APR_Pool_Debug" },
                       { "name" => "Apache:MirrorBrain" },
                       { "name" => "Apache:Modules" }] }

    ret = Xmlhash.parse(xml)
    assert_equal ret, rubyoutput

    assert_equal ret.elements("entry").first.value("name"), "Apache"
  end

  def test_encoding
    xml = "<?xml version='1.0' encoding='UTF-8'?><name>Adrian Schröter</name>"

    ret = Xmlhash.parse(xml)
    assert_equal ret, "Adrian Schröter"

    xml = "<?xml version='1.0' encoding='UTF-8'?><name value='Adrian Schröter'/>"
    ret = Xmlhash.parse(xml)
    assert_equal ret, { "value" => "Adrian Schröter" }

    assert_equal ret.get("value"), "Adrian Schröter"
  end

  def test_cdata
    xml = <<~eos
      <sourcediff key="7ebf6606bf56a9f952dda73f0d861738">
         <new name="myfile" md5="299d8fe34c516b078c3d367e3fb460b9" size="12"/>
          <diff lines="1">DummyContent</diff>
      </sourcediff>
    eos

    ret = Xmlhash.parse(xml)
    assert_equal ret['diff'], { "lines" => "1", "_content" => "DummyContent" }
  end

  def test_empty
    xml = "<request><files/></request>"
    ret = Xmlhash.parse(xml)
    assert_equal ret.elements('files'), []
  end

  def test_garbage
    # unfortunately it's rather challening testing nothing is printed to stderr
    ret = Xmlhash.parse("asdasdaskdladka")
    assert_nil ret
  end

  def test_entities
    ret = Xmlhash.parse("<ents><text>&lt;</text><text>&gt;</text></ents>")
    assert_equal ret, {"text"=>["<", ">"]}
  end

  def test_utf8
    xml = '<package name="libconfig" project="home:coolo">
  <title>libconfig &#8211; C/C++ Configuration File Library</title>
  <description>Libconfig is a simple library for processing structured configuration files, like this one: test.cfg. This file format is more compact and more readable than XML. And unlike XML, it is type-aware, so it is not necessary to do string parsing in application code.

Libconfig is very compact &#8212; just 38K for the stripped C shared library (less than one-fourth the size of the expat XML parser library) and 66K for the stripped C++ shared library. This makes it well-suited for memory-constrained systems like handheld devices.

The library includes bindings for both the C and C++ languages. It works on POSIX-compliant UNIX systems (GNU/Linux, Mac OS X, Solaris, FreeBSD) and Windows (2000, XP and later).</description>
  </package>'
    xh = Xmlhash.parse(xml)
    assert_equal "UTF-8", xh['title'].encoding.to_s

    # now try with different input encoding
    xml.encode!('US-ASCII')
    xh = Xmlhash.parse(xml)
    assert_equal "UTF-8", xh['title'].encoding.to_s

    xml = '<?xml version="1.0" encoding="ISO-8859-1"?>
    <package><title>Äöß</title></package>'
    xml.encode!('ISO-8859-1')
    xh = Xmlhash.parse(xml)
    assert_equal "ISO-8859-1", xh['title'].encoding.to_s

    xml = '<?xml version="1.0" encoding="ISO-8859-1"?>
    <package><title>&#228;&#211;&#254;</title></package>'
    xml.encode!('US-ASCII')
    xh = Xmlhash.parse(xml)
    assert_equal "UTF-8", xh['title'].encoding.to_s
  end
end
