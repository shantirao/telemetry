/*
usage: extract \data\20100901 note,wfsc
*/
var folder = system.arguments.shift() || '.'

var channels = (system.arguments.shift() || '').split(',')

var fileNumber = 1

function padLeft(i) {return ('00000000'+i).substr(-8);}

var header = new Record

var out = new Table
out.addColumn('id')
out.addColumn('channel')
out.addColumn('time')
out.addColumn('text')
var row = 0

while (1)
{
	let fileName = folder + '/' + padLeft(fileNumber) + '.dat'
	if (!system.exists(fileName)) break
	
	fileNumber++
	
	let dat = new Stream(fileName)
	let value
	
	while (!dat.eof)
	{		
		dat.readMIME(header)
		value = dat.read(parseInt(header.get('length')))
		let ch = header.get('channel')
		if (channels.length == 0 || channels.indexOf(ch) != -1)
		{
			row++
		 	out.set(row,'id',header.get('id'))
		 	out.set(row,'channel',header.get('channel'))
		 	out.set(row,'time',header.get('time'))
		 	
		 	if (value[0] != '(' || value[1] != '{') //not a JS object
		 		out.set(row,'text',unescape(value))
		 	else
		 	{
		 		value = eval(value)
		 		for (let [n,v] in value)
		 		{
		 			if (n) n = ch +'.'+n 
		 			if (!out.column(n)) out.addColumn(n)
		 			out.set(row,n,v.toString().replace(/\n\n/,''))
		 		}
		 	}
			//write(header.get('id'),'\t',header.get('channel'),'\t',header.get('time'),'\t',value)
		}
		dat.readln()		
		dat.readln()
		system.gc()		
	}
	dat.close()
}

out.save(folder+'_extract.tsv','\t')