/*
usage: transcript [-html] \data\20100901 key_channels ... > file.txt
*/
load('utils.js')
if (system.arguments.length == 0)
{
 writeln('usage: transcript [-html] \\data\\20100901 key_channels ... > file.txt')
}

var html = system.arguments[0] == '-html';
if (html) system.arguments.shift();

var folder = system.arguments.shift() || '.';

var channels = system.arguments;

//function padLeft(i) {return ('00000000'+i).substr(-8);}

var highlight = ['note']
var header = new Record
var tab = html ? '<td>':'\t';
	
if (html) writeln('<table nowrap style="border-collapse:collapse" border=1>')

for (var fileNumber = 1; ; fileNumber++)
{
	let fileName = folder + '/' + padLeft(fileNumber,8) + '.dat'
	if (!system.exists(fileName)) break
	
	let dat = new Stream(fileName)
	var value
	var ch
	var s
	var isKey;
	
	while (!dat.eof)
	{		
		dat.readMIME(header)
		value = dat.read(parseInt(header.get('length')))		
		ch = header.get('channel')
		
		isKey = channels.indexOf(ch) != -1;
		if (html) 
		{
		 if (isKey) write('<tr bgcolor=lightcyan><td>')
		 else if (highlight.indexOf(ch) != -1)write('<tr bgcolor=lighthellow><td>')
		 else write('<tr><td>')
		}
		write(header.get('id'),tab,header.get('time'),tab,ch,tab)
		
		if (1)
		{
		}
		else if (value[0] == '(' && value[1] == '{')
		{
		 value = eval(value);
		 s = []
		 for (let [n,v] in value)
		 {
		  if (v.constructor.name == 'Array' && v.length > 6)
		  {
		   s.push(n+': '+stats(v.map(function(x) (Number(x)>65535?65535:x))).toString())
		  }		   
		  else if (v.constructor.name == 'String'||v.constructor.name == 'Number'||v.constructor.name == 'Boolean')
		  {
		   s.push(n+': '+v)
		  }
		  else
		  {
		   s.push(n+': '+v.toSource())
		  }
		 }
		 value = s.join(html?'<br>':isKey ? '\n\t\t\t':'\n\t\t\t\t')
		}
		//else if (value.length > 255) value = value.substr(0,255)
		
		
		if (isKey)
		 writeln(value,tab)
		else
		 writeln(tab,value)
		if (html) write('</tr>')
				
		dat.readln()		
		dat.readln()
		system.gc()		
	}
	dat.close()
}


if (html) writeln('</table>')