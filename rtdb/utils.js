function padLeft(num,size)
{
 let t = num.toString()
 while (t.length < size) t = '0' + t
 return t
}
function call(cmd)
{
 let s = new Stream('exec://' +cmd)
 let ret = s.readFile()
 s.close()
 system.gc()
 return ret
}

function Polynomial(coefficients)
{
 this.poly = coefficients;
}

Polynomial.prototype.at = function(x)
{
   var z = 1;
   var y = 0
   for (var a in this.poly)  
   {
    y += this.poly[a] * z
    z *= x
   }  
   return y;
}

function plotSeries(data,out,points,curves)
{
 var s = stats(data);
 var p = new Image(600,400)
 var c = p.color(0,0,255)
 var c2 = p.color(255,0,0)
 var last = null;
 var lastx = 0;
 //writeln(s.toSource())
 /*
 if (curves)
 for (var c in curves)
 {
  var poly = new Polynomial(curves[c])
  for (var i =0; i< data.length; i++)
  {
   var y = poly.at(data[i][0])
   if (y < s.max && y > s.min)
   {
    var x = Math.round(i * p.width / data.length)
    var d = Math.round((y - s.min) * p.height / s.range)
    p.setp(x,Math.round(p.height - d),c)
   }
  }
  
 }*/
 for (var i=0; i<data.length; i++)// (!src.eof)
  {
   if (data[i] == null) continue;
   var x = Math.round(i * p.width / data.length)
   var d = Math.round((data[i] - s.min) * p.height / s.range)
   
   p.setp(x,Math.round(p.height - d),c)
   
   if (i){
     var a = Math.round(p.height - last)
     var b = Math.round(p.height - d)
     if (points)
       p.setp(x,Math.round(p.height - d),c)
     else
       p.line(lastx, a , x,b, c)     
   }
   lastx = x
   last = d
  }
 
   if (typeof out == 'string') 
   {
    out = new Stream(out,'wb')
    p.write(out)
    out.close()
   }
   else
   {
    p.write(out)
  }
}

/** 
data in blue, curves in red, points in black
labels = [ {x: 4, y: 7, text: "hello", color: [122,33,44] } ]
*/
function plotData(data,out,curves,points,labels,colors) 
{
  var width = data.length
  var i=0
  var domain, range
  var series = 1;
  var domain = [data[0][0], data[0][0]];
  var range = [data[0][1], data[0][1]];
  var margin = 20

  for (var i in data)
  {
    if (data[i][0] < domain[0]) domain[0] = data[i][0]
    if (data[i][0] > domain[1]) domain[1] = data[i][0]
  
    for (var s =1; s < data[i].length; s++)
    {
     if (data[i][s] < range[0]) range[0] = data[i][s]
     if (data[i][s] > range[1]) range[1] = data[i][s]
     if (s > series) series = s;
    }
  } 
 
  var plotScale = 1;
  while (((plotScale+1) * width) <= 800) plotScale++;
 
  if (width > 2048) width = 2048
  else width = width * plotScale
  
  var height = Math.round(width * 3/4);
  var p = new Image(width,height)
  var color = p.color(0,0,255)
  var c1 = [color];
  if (colors) for (var i in colors) c1[i] = p.color(colors[i][0],colors[i][1],colors[i][2]);
  var c2 = p.color(255,0,0)
  var l = p.color(0,0,0)
  var c3 = p.color(0,0,0)
  var grid = p.color(200,200,200)

  width -= margin
  var height = p.height - margin
  var scaleX = (domain[1] == domain[0])? width : (width) / (domain[1] - domain[0]) 
  var scaleY = (range[1] == range[0]) ? height : (height) / (range[1] - range[0])
  //writeln(domain)
  //writeln(range)
  
  var divX
  var divY
  
  if (domain[1] == domain[0]) divX = 1;
  else divX = Math.pow(10,Math.floor(Math.log(domain[1] - domain[0])/Math.log(10))) /5
  
  if (range[1] == range[0]) divY = 1;
  else divY = Math.pow(10,Math.floor(Math.log(range[1] - range[0])/Math.log(10))) /5
  
  var startX = Math.floor(domain[0])
  var startY = Math.floor(range[0])

  i = 0

//writeln(startX,' ',startY,' ',divX,' ',divY)
  function toX(ax) {return Math.round(margin + (ax - domain[0]) * scaleX);}
  function toY(ay) {return Math.round(height - (ay - range[0]) * scaleY);}
  
  if (labels)
  {
    for (var i in labels)
    {
      labels[i].x = toX(labels[i].x)
      labels[i].y = toY(labels[i].y)
    }
  }
  else labels = []
  
  p.line(margin,0,margin,height,l)
  p.line(margin,height,p.width,height,l)

  var show= true;
  while (true)
  {
   var x = startX + (i * divX)
   i++;
   if (x < domain[0]) continue
   if (x > domain[1]) break
   var sx = toX(x)
   
   //if (sx <= 20) continue
   if (i % 2)
   {
    p.line(sx,height + 5,sx, height , grid)    
   }
   else if (i% 10)
   {
    p.line(sx,0,sx,height+5,grid)
    show = true
   }
   else
   {
    p.line(sx,0,sx,height,grid)
    p.line(sx,height+10,sx,height+margin,l)
    show = true
   }
   
   if (show)
     labels.push({x: sx-5, y: height + 10, text: x.toFixed(2).replace(/\.*0+$/,'')})
   show = false
  }
  
  i=0;
  show = true
  while (true)
  {
   var y = startY + (i * divY)
   i++;
   if (y < range[0]) continue
   if (y > range[1]) break
   var sy = toY(y)
   
   //if (sy <= 20) continue
   if (i % 2)
   {
    p.line(margin - 5, sy, margin , sy , grid)    
   }
   else if (i% 10)
   {
    p.line(margin - 5, sy, p.width , sy,grid)
    show = true
   }
   else
   {
    p.line(margin,sy,p.width,sy,grid)
    p.line(0,sy,margin,sy,l)
    show = true
   }
   
   if (show)
    labels.push({x: 2, y: sy-2, text: y.toFixed(2).replace(/\.*0+$/,'')})
   show = false

  }
    
  if (curves)
  {    
    for (var c in curves)
    {
      var last = null;
      var poly = curves[c]
      for (var i = 0; i < width; i++)
      {
       var x = (i / scaleX) + domain[0]
       var y = toY(poly.at(x))
       
       if (y < height && y > 0)
       {
         if (last == null)
         {
          p.setp( margin+ i, y,c2)
          last = [ i, y]
         }
         else
         {
           p.line(margin+last[0],last[1],margin+ i, y, c2)
           p.setp(margin+  i , y,c2)
           last = [i, y]
         }
       }
       else last = null
      }
    }
  }
  
  if (points)
  for (var p in points)
  {   
   p.setp(toX(points[p][0]), toY(points[p][1]),c3)
  }

  var last, i, x, y, c
  for (var s=0; s < series; s++)
  {
   if (s < c1.length) 
    c = c1[s];
   else 
    c = color
   
   last = null
   for (i in data)// (!src.eof)
   {  
    x = toX(data[i][0])
    y = toY(data[i][s + 1])
    if (last == null || x < last[0])
    {
      p.setp(x, y, c)
    }
    else
    {
      p.line(last[0], last[1] , x, y, c)
      
    }
    last = [x,y]
   }
  }
  
  for (var i in labels)
  {
   var color = l;
   if (labels[i].color)
   {
    color = p.color(labels[i].color[0],labels[i].color[1],labels[i].color[2])
   }
   p.print(labels[i].x, labels[i].y, color, labels[i].text)
   
  }
   
  if (typeof out == 'string') 
  {
   out = new Stream(out,'wb')
   p.write(out)
   out.close()
  }
  else
  {
   p.write(out)
  }
}

function average(list)
{
 var s = 0.0;
 for (var i in list) s += list[i]
 return s / list.length
}

function stats(list,mask)
{
 if (list.constructor.name != "Numbers") 
   if (typeof list.data == 'object' && list.data.length > 0) list = list.data;
 var s = 0.0;
 var s2 = 0.0
 var mx = list[0]
 var mn = list[0]
 var mnp = 0;
 var mxp = 0;
 if (typeof mx != "number") mx = mx[1]
 if (typeof mn != "number") mn = mn[1]
 
 if (list.constructor.name == "Numbers")
 for (var i=0; i<list.length; i++) 
 {
  var x = list.get(i)
  if (mask && !(mask.get ? mask.get(i) : mask[i])) continue
 // writeln(x.toSource())
  if (typeof x == "undefined") continue
  if (typeof x != "number") x = x[1]
  if (typeof x != "number") continue
  if (i % 10000 == 0) {system.gc()}
  s += x
  s2 += x * x
  if (mx < x || typeof mx == "undefined") {mxp = i; mx = x;}
  if (mn > x || typeof mn == "undefined") {mnp = i; mn = x;}
 }
 else
 for (var i=0; i<list.length; i++) 
 {
  var x = list[i]
  if (mask && !mask[i]) continue
 // writeln(x.toSource())
  if (typeof x == "undefined") continue
  if (typeof x != "number") x = x[1]
  if (typeof x != "number") continue
  if (i % 10000 == 0) {system.gc();}
  s += x
  s2 += x * x
  if (mx < x || typeof mx == "undefined") {mxp = i; mx = x;}
  if (mn > x || typeof mn == "undefined") {mnp = i; mn = x;}
 }
 var l = list.length;
 return {stddev: Math.sqrt((s2 - ((s*s)/(l)))/l), 
         average: s / l, 
         sum:s, 
         minp: mnp, 
         maxp: mxp, 
         max: mx, 
         min: mn, 
         range: mx - mn,
         size:l,
         toString: function() {var s = ''; for (var i in this) if (typeof this[i] == 'number') s += i + '=' + this[i] + ' '; return s;}
         }
}