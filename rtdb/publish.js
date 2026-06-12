s = new Stream('tcp://127.0.0.1:8060')

var n = "0123456789"
for (var i=0; i< 1024; i++)
{
 s.writeln('insert numbers 32')
 for (var j=0; j<32; j++)
 {
  s.write(n[i%10])
 }

}
s.writeln('close')
s.close()