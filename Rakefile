# -*- ruby -*-

require 'rubygems'
require 'hoe'
require 'rake/extensiontask'

Hoe.spec 'xmlhash' do
  developer('Stephan Kulow', 'coolo@suse.com')
  self.licenses = ["MIT"]
  self.readme_file = 'README.txt'
  self.spec_extras = { :extensions => ["ext/xmlhash/extconf.rb"] }
  self.extra_dev_deps << ['rake-compiler', '>= 0']
  Rake::ExtensionTask.new('xmlhash', spec) do |ext|
    ext.lib_dir = File.join('lib', 'xmlhash')
  end
end

Rake::Task[:test].prerequisites << :compile

# vim: syntax=ruby
