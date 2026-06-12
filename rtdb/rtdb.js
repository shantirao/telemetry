
var currentSnapshot = {}
var snapshotFlashTime = 4
var currentHighlight = null
var currentInspection = {}
var startDateTime;
var dateTimeOffset;

function setStartTime(x)
{
	startDateTime = new Date(x.substr(0,4),x.substr(4,2),x.substr(6,2),x.substr(8,2),x.substr(10,2),x.substr(12,2));
	$('startTime').innerHTML = startDateTime.toUTCString()
	startDateTime = startDateTime.getTime();
	var t = new Date(startDateTime);
	t.setHours(0);
	t.setMinutes(0);
	t.setSeconds(0);
	t.setMilliseconds(0);
	dateTimeOffset = startDateTime - t.getTime();
	$('debug').innerHTML = printElapsed(0)
}

function inspect(dom)
{
	if (dom.previousSibling.previousSibling.style.display=='none')
	{
	 dom.previousSibling.previousSibling.style.display=''
	 dom.previousSibling.style.display='none'
	 dom.innerHTML = '...'
	}
	else
	{
	 dom.previousSibling.previousSibling.style.display='none'
	 dom.previousSibling.style.display=''
	  dom.innerHTML = '[x]'
	}
}

function pad(x,n)
{
	x = x.toString();
	while (x.length < n) x = '0'+x;
	return x;
}

function printElapsed(t) //[d] hh:mm:ss.xxx
{
	var t = dateTimeOffset + Number(t);
	var d = Math.trunc(t/86400000);

	var h = t % 86400000;
	h = Math.trunc(h/3600000);
	var m = t % 3600000;
	m = Math.trunc(m/60000);
	var s = t % 60000;
	s = s.toString();
	return (d ? d.toString() + ' ' : '')+ pad(h,2) + ':' + pad(m,2) + ':' + s.substr(0,2) + '.' + pad(s.substr(2,3),3);
}

function printLong(text,length)
{
 var l = length || 160
 if (text.length > l)
  return '<span>'+text.substr(0,l) + '</span><span style="display:none">'+ text+'</span><span onClick="inspect(this)">...</span></tt></td>'
 else
  return text
}

function printSnapshot(objs)
{
 var r =['<tr style="background:#a0a0a0"><td width=15%>Channel<td width=5%>Time<td width=5%>ID<td>Event<td>Value</tr>']
 for (var i in objs)
 {
  var n = objs[i].channel
  var e = objs[i].event
  r.push(objs[i]._flash ? '<tr style="background:#cfc">' : '<tr>')
  r.push('<td nowrap><span onClick="history.title = \'' + n + '\';status(\'wait\'); historyChannel = \''+n+'\';loadXMLDoc(\'/select?' + escape(n) + '\',history)">' + n + '</span></td>')
  r.push('<td>' + printElapsed(objs[i].time) + '</td><td>' + objs[i].id + '</td>')
  r.push('<td nowrap><span onClick="loadXMLDoc(\'/event?' + escape(e) + '\',showEvent)">' + e + '</span></td>')
  r.push('<td><tt>' )
  r.push(printLong(objs[i].value) + '</tt></td>')
 }
 return r.join('');
}

function updateSnapshot(objs, snapshot, flashTime)
{
 if (!flashTime) flashTime= 1
 var any = false
 var updates = false
 //var maxId = 0
 for (var i=0; i< objs.length; i++)
 {
  var o = objs[i]
  var n = o.channel
  if (!n) continue
  var old = snapshot[n]
  if (!old || old.id != o.id)
  {
    snapshot[n] = o
    o._flash = flashTime
    //if (o.id > maxId) maxId = o.id
    any = true
    updates = true
  }
  else if (old._flash)
  {
    old._flash --
    any = true
  }
 }
 return any //needs redrawing?
}

function status(msg)
{
 $('status').innerHTML = msg
}

function snapshot(text)
{
 var objs = readTSV(text)

  if (updateSnapshot(objs, currentSnapshot, snapshotFlashTime))
    $('snapshot').innerHTML = printSnapshot(currentSnapshot)

  status('')
}

function fetch(text)
{
//$('debug').innerHTML = '<pre>' + text + '</pre>'
  updateSnapshot(readTSV(text), currentInspection)
 $('data').innerHTML =  printSnapshot(currentInspection)
 status('')
 resizeLastNode($('history'),16)
}

function select(obj)
{
 var old = currentHighlight
 if (currentHighlight) currentHighlight.style.background = ''
 currentHighlight = obj
 obj.style.background = '#ddf'
 return old
}

var historyAsTable = false
var historyChannel = ''

function showEvent(text)
{
	$('debug').innerHTML = text
	 var objs = readTSV(text)
    $('event').innerHTML = printSnapshot(objs)

  status('')
}

function history(text) //data id channel length event time\n value
{
 var lines
 if (historyAsTable)
  {
 try{
    header = ['id']
     lines = text.replace(/data (\d+) (\d+) (\d+) (\d+) (\d+).*\n(.*)/gm,
       function(t, id, channel, length, event, time, data)
       {
        var s = eval(data.substr(0,length))
        var l = ['<tr><td onClick="if (select(this) !== this) {status(\'wait\'); loadXMLDoc(\'/snapshot?'+id+'\',fetch);$(\'when\').innerHTML=\''+id+'\'}">'+id+'</td>']
        var i=0;
        //return '<tr><td>' + data + '</td></tr>'
        for (var n in s)
        {
         if (header.length <= i+1) header[i+1] =
         '<span onClick="window.open(\'plot.html?' + historyChannel +'.'+n+'\',\'plot'+historyChannel+'.'+n+'\')">' +n + '</span>'

         i++
         l.push( '<td>' + s[n] + '</td>')
        }
        l.push('</tr>\n')
        return l.join('')
       }
     )
    lines = '<table><thead><tr><td>' + header.join('</td><td>') + "</td></tr></thead><tbody>" + lines + "</tbody>"
   } catch(err) {historyAsTable = false}
 }

 if (!historyAsTable)
     lines = text.replace(/data (\d+) (\d+) (\d+) (\d+) (\d+).*\n(.*)/gm,
	     function(t, id, channel, length, event, time, data)
        {
        return '<span onClick="if (select(this) !== this) {status(\'wait\'); loadXMLDoc(\'/snapshot?' + id + '\',fetch);$(\'when\').innerHTML=\''+id+'\'}">'+id+' ['+time+']: '+data.substr(0,length)+'</span><br>\n'
        })

 $('history').innerHTML = lines
 $('title').innerHTML = history.title
 status('')
 resizeLastNode($("history"),16)
}

function periodic()
{
 $('status2').innerHTML ='snapshot'
 loadXMLDoc('/snapshot',snapshot)
 //window.setTimeout(periodic, 100)
}

function readQuery(text)
{
 // data id channel length sequence time CRC
 var r = [] //{id: id, value: value}

}

function readTSV(text)
{
 var r = []
 var l = text.split('\n')
 var fields = l.shift()
 if (!fields) return null
 fields = fields.split('\t')
 if (!fields.length) return null
 while (l.length)
 {
  var values = l.shift().split('\t')
  if (values.length == fields.length)
  {
    var o = {}
    for (var i=0; i< fields.length; i++)
      o[fields[i]] = values[i]
    r.push(o)
  }
 }
 return r
}

//loadXMLDoc('/snapshot',snapshot)
window.setInterval("loadXMLDoc('/snapshot',snapshot); if (historyChannel && historyAsTable) loadXMLDoc(\'/select?' + escape(historyChannel) + '\',history)", 750)
