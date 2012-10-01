# encoding: UTF-8

require "test/unit"
require "xmlhash"
require 'json'

class TestXmlhash < Test::Unit::TestCase
  def test_xml
    xml = <<eos
<request id="93651">
  <action type="submit">
    <source project="server:dns" package="pdns" rev="65"/>
    <target project="openSUSE:Factory" package="pdns"/>
  </action>
  <state name="revoked" who="coolo" when="2011-12-19T13:20:50">
    <comment/>
  </state>
  <review state="accepted" by_group="legal-auto" who="licensedigger" when="2011-11-25T15:09:55">
    <comment>{"approve": "preliminary, version number changed"} &lt;!-- {
  "dest": {
    "ldb": {
      "review": "done", 
      "rpm_license": "GPLv2+", 
      "status": "production", 
      "version": "3.0.rc1"
    }, 
    "license": "GPLv2+", 
    "version": "2.9.22"
  }, 
  "hint": [
    "src('3.0') and dest('2.9.22') version numbers differ"
  ], 
  "plugin": "0.35", 
  "src": {
    "auto-co": "/api.opensuse.org/server:dns/pdns%3.0%r65", 
    "license": "GPLv2+", 
    "rev": "65", 
    "version": "3.0"
  }
} --&gt;</comment>
  </review>
  <review state="new" by_group="factory-auto"/>
  <history name="review" who="coolo" when="2011-11-25T15:02:53"/>
  <history name="declined" who="coolo" when="2011-11-25T16:17:30">
    <comment>please make sure to wait before these depencencies are in openSUSE:Factory: libopendbx-devel, libopendbx1, libopendbxplus1, opendbx-backend-pgsql</comment>
  </history>
  <description>update and factory fix (forwarded request 86230 from -miska-)</description>
</request>
eos

    rubyoutput =  {"history"=> 
      [ {"name"=>"review", "when"=>"2011-11-25T15:02:53", "who"=>"coolo"}, 
        {"comment"=>"please make sure to wait before these depencencies are in openSUSE:Factory: libopendbx-devel, libopendbx1, libopendbxplus1, opendbx-backend-pgsql", 
          "name"=>"declined", "when"=>"2011-11-25T16:17:30", "who"=>"coolo"}
      ], 
      "review"=> 
      [
       {"comment"=>"{\"approve\": \"preliminary, version number changed\"} <!-- {\n  \"dest\": {\n    \"ldb\": {\n      \"review\": \"done\", \n      \"rpm_license\": \"GPLv2+\", \n      \"status\": \"production\", \n      \"version\": \"3.0.rc1\"\n    }, \n    \"license\": \"GPLv2+\", \n    \"version\": \"2.9.22\"\n  }, \n  \"hint\": [\n    \"src('3.0') and dest('2.9.22') version numbers differ\"\n  ], \n  \"plugin\": \"0.35\", \n  \"src\": {\n    \"auto-co\": \"/api.opensuse.org/server:dns/pdns%3.0%r65\", \n    \"license\": \"GPLv2+\", \n    \"rev\": \"65\", \n    \"version\": \"3.0\"\n  }\n} -->", "by_group"=>"legal-auto", "when"=>"2011-11-25T15:09:55", "who"=>"licensedigger", "state"=>"accepted"}, {"by_group"=>"factory-auto", "state"=>"new"}], "action"=>{"type"=>"submit", "target"=>{"project"=>"openSUSE:Factory", "package"=>"pdns"}, "source"=>{"rev"=>"65", "project"=>"server:dns", "package"=>"pdns"}}, "id"=>"93651", "description"=>"update and factory fix (forwarded request 86230 from -miska-)", "state"=>{"comment"=>{}, "name"=>"revoked", "when"=>"2011-12-19T13:20:50", "who"=>"coolo"}}

    1000.times {
      ret = Xmlhash.parse(xml)
      GC.start
      assert_equal ret, rubyoutput
    }
    10000.times {
      ret = Xmlhash.parse(xml)
      assert_equal ret, rubyoutput
     }

  end

  def test_entry
      xml = <<eos
<?xml version='1.0' encoding='UTF-8'?>
<directory count="4">
   <entry name="Apache"/>
   <entry name="Apache:APR_Pool_Debug"/>
   <entry name="Apache:MirrorBrain"/>
   <entry name="Apache:Modules"/>
</directory>
eos

    rubyoutput = {"count" => "4",
      "entry"=>
      [{"name"=>"Apache"},
       {"name"=>"Apache:APR_Pool_Debug"},
       {"name"=>"Apache:MirrorBrain"},
       {"name"=>"Apache:Modules"}]}
    
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
     assert_equal ret, {"value"=>"Adrian Schröter"}

     assert_equal ret.get("value"), "Adrian Schröter"
  end

end
