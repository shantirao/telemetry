s = new Stream('tcp://127.0.0.1:8040')

s.writeln('snapshot ',Number(system.arguments.shift()))

var id, length, channel

while (s.canWrite && !system.kbhit())
{
 let h = s.readLine()

 if (!h) continue
 if (h == 'ok') break
 writeln(h)


 let m = h.match(/^info (\d+) (\d+) (\d+)/) //id, channel, length
 if (m)
 {
  [,id, channel, length] = m
  s.writeln("fetch "+id);
  continue
 }


 m = h.match(/^data (\d+) (\d+) (\d+)/) //id, channel, length
 if (m)
 {
  [,id, channel, length] = m
  writeln(channel,'.',id,': ',s.read(Number(length)))
  continue
 }

 if (!s.canRead) system.sleep(100)
}

//writeln("Done. Press Enter.")
//system.readln()