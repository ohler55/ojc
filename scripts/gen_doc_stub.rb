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

$enums = []

def gather_enums
  i = 0
  while (!(i = $header.index('typedef enum', i + 1)).nil?)
    semi = $header.index(';', i)
    $enums << Enum.new($header[i..semi])
    i = semi
  end
  $enums.sort_by! { |e| e.name }
end

gather_enums

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

# TBD types
# TBD globals
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

# TBD types
# TBD globals
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
