#!/usr/bin/env ruby

while (index = ARGV.index('-I'))
  _,path = ARGV.slice!(index, 2)
  $: << path
end

$header = IO.read("src/oj/oj.h")

$out_file = ARGV[0]
$version = '?'

# get the version
i = $header.index('OJ_VERSION')
first = $header.index('"', i + 10)
last = $header.index('"', first + 1)
$version = $header[first + 1...last]

class Enum
  attr_accessor :name
  attr_accessor :synopsis
  attr_accessor :values

  def initialize(s)
    @synopsis = s
    n = s.index('}')
    @name = s[n + 1..-2].strip
    @values = []
    i = s.index('{') + 1
    s[i...n].split("\n").each { |line|
      line.strip!
      next if line.empty?
      v, _ = line.strip.split('=')
      v.strip!
      @values << v unless v.empty?
    }
  end
end

class Param
  attr_accessor :name
  attr_accessor :type

  def initialize(name, type)
    @name = name
    @type = type
  end
end

class Type
  attr_accessor :name
  attr_accessor :synopsis
  attr_accessor :fields

  def initialize(name, s)
    @name = name.gsub('*', '')
    @synopsis = s
    @fields = []
    s[0..-2].split("\n").each { |line|
      next unless line.include?(';')
      next if line.include?('{') || line.include?('}')
      line = line.split(';')[0]
      line.strip!
      parts = line.split("\t")
      @fields << Param.new(parts[-1], parts[0])
    }
  end
end

class Global
  attr_accessor :name
  attr_accessor :synopsis

  def initialize(s)
    @name = s.strip[0..-2].split("\t")[-1]
    @synopsis = s.gsub(/\t+/, "\t")
  end
end

class Func
  attr_accessor :name
  attr_accessor :synopsis
  attr_accessor :params

  def initialize(s)
    @synopsis = s
    # TBD
  end
end

$enums = []
$types = []
$globals = []
$funcs = []

def type_end(s, i)
  cnt = 1
  i = s.index('{', i) # get things started
  while true
    i = s.index(/[{}]/, i + 1)
    if '{' == s[i]
      cnt += 1
    else
      cnt -= 1
    end
    break if 0 == cnt
  end
  i
end

def gather_enums
  i = 0
  while (!(i = $header.index('typedef enum', i + 1)).nil?)
    semi = $header.index(';', i)
    $enums << Enum.new($header[i..semi])
    i = semi
  end
  $enums.sort_by! { |e| e.name }
end

def gather_types
  i = 0
  while (!(i = $header.index('typedef union', i + 1)).nil?)
    close = type_end($header, i)
    semi = $header.index(';', close)
    $types << Type.new($header[close + 1...semi].strip, $header[i..semi])
    i = semi
  end
  i = 0
  while (!(i = $header.index('typedef struct', i + 1)).nil?)
    close = type_end($header, i)
    semi = $header.index(';', close)
    $types << Type.new($header[close + 1...semi].strip, $header[i..semi])
    i = semi
  end
  $types.sort_by! { |e| e.name }
end

def gather_globals
  i = 0
  $header.split("\n").each { |line|
    next unless line.include?('extern') && line.include?(';')
    next if line.include?('(')
    line.strip!
    $globals << Global.new(line)
  }
end

def gather_funcs
end

gather_enums
gather_types
gather_globals
gather_funcs

$out = %|<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>
  <head>
    <title>OjC Client API</title>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
    <link rel="stylesheet" type="text/css" media="screen,print" href="ojc-api.css" />
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body onload="displayDesc(event,'Introduction')">
    <div class="header">
      <a href="https://github.com/ohler55/ojc" class="normal"><img src="ojc.svg" width="160"></a><span class="api">v#{$version} API Documentation</span>
    </div>
    <div class="page">
      <div class="index">
        <button class="item level1" onclick="displayDesc(event,'Introduction')">Introduction</button>
        <button class="item level1" onclick="displayDesc(event,'Guides')">Guides</button>
|

$out += %|        <span class="cat">Enums</span>
|
$enums.each { |e|
  $out += %|        <button class="item level2" onclick="displayDesc(event,'#{e.name}')">#{e.name}</button>
|
}

$out += %|        <span class="cat">Types</span>
|
$types.each { |t|
  $out += %|        <button class="item level2" onclick="displayDesc(event,'#{t.name}')">#{t.name}</button>
|
}

$out += %|        <span class="cat">Globals</span>
|
$globals.each { |g|
  $out += %|        <button class="item level2" onclick="displayDesc(event,'#{g.name}')">#{g.name}</button>
|
}

$out += %|        <span class="cat">Functions</span>
|
# TBD functions

$out += %|      </div>

      <div class="desc-pane">
        <div id="Introduction" class="desc">
          <div class="title">Introduction</div>
          <p class="desc-text">
	    TBD
          </p>
        </div>

        <div id="Guides" class="desc">
          <div class="title">Guides</div>
          <h3>Simple JSON Parsing Example</h3>
	  TBD examples
        </div>
|

# details

$enums.each { |e|
  $out += %|
        <div id="#{e.name}" class="desc">
          <div class="title">#{e.name}</div>
          <div class="synopsis">#{e.synopsis}</div>
          <p class="desc-text">
	  TBD
          </p>
          <table class="params">
|
  e.values.each { |v|
    $out += %|            <tr><td><span class="param">#{v}</span></td><td>TBD</td></tr>
|
  }
  $out += %|          </table>
        </div>
|
}

$types.each { |t|
  $out += %|
        <div id="#{t.name}" class="desc">
          <div class="title">#{t.name}</div>
          <div class="synopsis">#{t.synopsis}</div>
          <p class="desc-text">
	  TBD
          </p>
          <table class="params">
|
  t.fields.each { |f|
    $out += %|            <tr><td><span class="param">#{f.type}</span></td><td><span class="param">#{f.name}</span></td><td>TBD</td></tr>
|
  }
  $out += %|          </table>
        </div>
|
}

$globals.each { |g|
  $out += %|
        <div id="#{g.name}" class="desc">
          <div class="title">#{g.name}</div>
          <div class="synopsis">#{g.synopsis}</div>
          <p class="desc-text">
	  TBD
          </p>
        </div>
|
}

# TBD functions

$out += %|
      </div>
    </div>
    <script>
function displayDesc(e,d){
    var all = document.getElementsByClassName('desc');
    for (var i = all.length - 1; 0 <= i; i--) {
        all[i].style.display = 'none';
    }
    all = document.getElementsByClassName('item');
    for (var i = all.length - 1; 0 <= i; i--) {
        all[i].className = all[i].className.replace(' selected', '');
    }
    document.getElementById(d).style.display = 'block';
    e.currentTarget.className += ' selected';
}
    </script>
  </body>
</html>
|

File.open($out_file, "w") { |f|
  f.puts $out
}
