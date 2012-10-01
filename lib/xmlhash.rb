require 'xmlhash/xmlhash'

module Xmlhash
  VERSION = '1.3.0'

  class XMLHash < Hash
    
    # Return an array of elements or []. It requires a plain string as argument
    # 
    # This makes it easy to write code that assumes an array. 
    # If there is just a single child in the XML, it will be wrapped
    # in a single-elemnt array and if there are no children, an empty
    # array is returned.
    # 
    # You can also pass a block to iterate over all childrens.
    def elements(name)
      unless name.kind_of? String
        raise ArgumentError, "expected string"
      end
      sub = self[name]
      return [] unless sub
      unless sub.kind_of? Array
        if block_given?
          yield sub
          return
        else
          return [sub]
        end
      end
      return sub unless block_given?
      sub.each do |n|
        yield n
      end
    end

    # Return the element by the given name or an empty hash
    # 
    # This makes it easy to write code that assumes a child to be present.
    # obj["a"]["b"] will give you a "[] not defined for nil".
    # obj.get("a")["b"] will give you nil
    def get(name)
      sub = self[name]
      return sub if sub
      return XMLHash.new
    end
      
    # Return the value of the name or nil if nothing is there
    # 
    def value(name)
      sub = self[name.to_s]
      return nil unless sub
      return '' if sub.empty? # avoid {}
      return sub
    end
    
    def inspect
      "X(#{super})"
    end
  end
end 
