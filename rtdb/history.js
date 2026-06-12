var dbName = system.arguments.shift() || '*'
var markerField = system.arguments.shift()
var marker = markerField ? '-marker ' + markerField : '';

var src = new Stream('exec://rtdb.exe -fields note '+marker+' -summary \\data\\' + dbName)

var lines = src.readFile().split('\n')
src.close();

function objtable(t)
{
 let s1 = ['<tr>']
 let s2 = ['<tr>']
 for (let [n,v] in t)
 {
  s1.push('<td><b>' + n + '</b></td>')
  v = v.toString()
  s2.push('<td>' + (v.length > 80 ? v.substr(0,80) + '...' : v) + '</td>')
 }
 return '<table border=0>' + s1.join('') + s2.join('') + '</table>'
}

var out = new Stream('history.html','wt')
header = "<style>\n" +
"body,html {font: 10pt Arial,Helvetica,sans-serif;}\n" +
"h1{font: 16pt Arial,Helvetica,sans-serif; font-weight: bold;  }\n" +
"table {border-collapse: collapse;}\n" +
"td {font: 10pt Arial,Helvetica,sans-serif; border:1px solid #c0c0c0;}\n" +
"td table {background: #ffffe0; margin:0px;}\n" +
"td td {font: 10pt Arial,Helvetica,sans-serif; border:0.5px dotted #c0c0c0; padding-left:2pt;padding-right:2pt;}\n" +
"</style>\n" 

function printLines(list)
{
 while (list.length)
 {
  l = list.shift()
  if (!l || l == '\r') continue
  l=l.split('\t') 
  
  
  if (markerField && l[2] == markerField)
   out.write('<tr bgcolor=#ffdddd>')
  else
   out.write('<tr>')
  for (var i =0; i<l.length; i++) 
  {
   out.write('<td>')
   if (l[i][0] == '(' && l[i][1] == '{')
   {
    out.write(objtable(eval(l[i])))
   }
   else
   out.write(l[i])
   
   out.write('</td>')
  }
  out.writeln()
 }
}

out.writeln(header)

lines.pop()

var current=[]
while (lines.length)
{
 var l = lines.shift()
 if (l.indexOf("Database:") == 0)
 {
  if (current.length)
  {
    current.unshift(current.pop())
	printLines(current)
	out.writeln('</table>')
  }
  out.writeln('<h1>',l.substr(10),'</h1>')
  out.writeln('<P><table width=100%>')  
  current = []
 }
 else 
 current.unshift(l)
}
if (current.length)
{
    current.unshift(current.pop())
 printLines(current)
 out.writeln('</table>')
}

out.close()
system.execute('history.html')

