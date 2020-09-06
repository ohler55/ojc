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

class Guide
  attr_accessor :key
  attr_accessor :title

  def initialize(key, title)
    @key = key
    @title = title
  end
end

$guides = [
  Guide.new('validate', 'Validate JSON String'),
  Guide.new('parse-string', 'Parse JSON String'),
  Guide.new('pp-string', 'Push-Pop Parse String'),
  Guide.new('callback-string', 'Callback Parse String'),
  Guide.new('caller-string', 'Caller Parse String'),
  Guide.new('pp-file', 'Push-Pop Parse File'),
  Guide.new('callback-file', 'Callback Parse File'),
  Guide.new('caller-file', 'Caller Parse File'),
  Guide.new('build-write', 'Build and Write'),
]

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
      type = parts[0]
      n = parts[-1]
      while '*' == n[0] do
	type += '*'
	n = n[1..-1]
      end
      @fields << Param.new(n, type)
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
  attr_accessor :type
  attr_accessor :params

  def initialize(s)
    @synopsis = s
    tab = s.index("\t", 7)
    @type = s[7..tab].strip
    open = s.index("(", tab)
    @name = s[tab...open].strip
    @params = []
    s[open + 1..-3].split(',').each { |arg|
      sep = arg.rindex(/[ \t]+/)
      t = arg[0..sep].strip
      n = arg[sep..-1].strip
      while '*' == n[0] do
	t += '*'
	n = n[1..-1]
      end
      @params << Param.new(n, t)
    }
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
  $header.split("\n").each { |line|
    next unless line.include?('typedef') && line.include?(';') && line.include?('(')
    line.strip!
    star = line.index('(*')
    close = line.index(')', star + 2)
    $types << Type.new(line[star + 2...close], line)
  }
  $types.sort_by! { |e| e.name }
end

def gather_globals
  $header.split("\n").each { |line|
    next unless line.include?('extern') && line.include?(';')
    next if line.include?('(')
    line.strip!
    $globals << Global.new(line)
  }
  $globals.sort_by! { |g| g.name }
end

def gather_funcs
  i = 0
  while (!(i = $header.index('extern', i + 1)).nil?)
    open = $header.index('(', i)
    nl = $header.index("\n", i)
    next if open.nil? || nl < open
    semi = $header.index(';', i)
    $funcs << Func.new($header[i..semi])
    i = semi
  end
  $funcs.sort_by! { |f| f.name }
end

gather_enums
gather_types
gather_globals
gather_funcs

$out = %|<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>
  <head>
    <title>OjC API</title>
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
|

$out += %|        <span class="cat">Guides</span>
|
$guides.each { |g|
  $out += %|        <button class="item level2" onclick="displayDesc(event,'#{g.key}')">#{g.title}</button>
|
}

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
$funcs.each { |f|
  $out += %|        <button class="item level2" onclick="displayDesc(event,'#{f.name}')">#{f.name}()</button>
|
}

$out += %|      </div>

      <div class="desc-pane">
        <div id="Introduction" class="desc">
          <div class="title">Introduction</div>
          <p class="desc-text">
	    TBD
          </p>
        </div>
|

# details

$guides.each { |g|
  $out += %|
        <div id="#{g.key}" class="desc">
          <div class="title">#{g.title}</div>
          <p class="desc-text">
	  TBD
          </p>
          <div class="synopsis">
TBD code
          </div>
        </div>
|
}

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
    $out += %|            <tr><td><span class="enum-value">#{v}</span></td><td>TBD</td></tr>
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
    $out += %|            <tr><td><span class="param-type">#{f.type}</span></td><td><span class="param-name">#{f.name}</span></td><td>TBD</td></tr>
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

$funcs.each { |f|
  $out += %|
        <div id="#{f.name}" class="desc">
          <div class="title">#{f.name}()</div>
          <div class="synopsis">#{f.synopsis}</div>
          <p class="desc-text">
	  TBD
          </p>
          <table class="params">
|
  f.params.each { |p|
    $out += %|            <tr><td><span class="param-type">#{p.type}</span></td><td><span class="param-name">#{p.name}</span> TBD.</td><tr>
|
  }
  $out += %|          </table>
        </div>
|
}

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
