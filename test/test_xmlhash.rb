require "test/unit"
require "xmlhash"

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
    ret = Xmlhash.parse(xml)
    assert_equal ret, { "hello" => { "who" => "world" } }
  end
end
