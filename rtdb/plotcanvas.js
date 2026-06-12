/**
data in blue, curves in red, points in black
Usage:

var canvas = document.getElementById("canvas");
var data = [[0, 0], [1, 1], [2, 4], [3, 9], [4, 16], [5, 25], [6, 36], [7, 49], [8, 64], [9, 81]]
resizePlot(canvas,data)
plotData(canvas, data,
{ curves:[{at:function(x) {return .9*x*x}}],
  points:[[4,7]],
  labels: [ {x: 4, y: 16, text: "hello", align:"left", baseline:"middle", color: [122,33,44] } ],
  colors: [[0,127,127]],
  pointColor:"purple",
 })

*/

function plotData(canvas,data,options)
{
    var width = canvas.width
    var height = canvas.height
    var i=0
    var domain, range
    var series = 1;
    var domain = [data[0][0], data[0][0]];
    var range = [data[0][1], data[0][1]];

    options = options || {}
    var timeseries = options['timeSeries'] || false
    var xInterval = timeseries ? 6 : 5;
    var yInterval = 5
    var curves = options['curves'] || null
    var points = options['points'] || null
    var labels = options['labels'] || []
    var colors = options['colors'] || null
    var margin = options['margin'] || 20
    var lines = options['lines'] != null ? options['lines'] : true
    var dots = options['dots'] || false
    var topMargin = 4    

    var axis = rgb(0,0,0)
    var grid = rgb(200,200,200)
    var color = rgb(0,0,255)
    var c1 = [color];
    if (colors) for (var i in colors) c1[i] = rgb(colors[i]);
    var c2 = rgb(255,0,0)
    var c3 = options['pointColor'] || rgb(255,127,0) //point color

    var p = canvas.getContext("2d")

  for (var i=0; i<data.length; i++)
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
//$('debug').innerHTML = data.toSource()

  width -= margin
  height -= margin
  height -= topMargin
  var divX
  var divY
  
  if (domain[1] == domain[0]) 
  {
    domain[1] = domain[0] + 1
    divX = .5;  
  }
  else 
  {
    divX = Math.pow(10,Math.floor(Math.log(domain[1] - domain[0])/Math.log(10))) / xInterval
    while ((domain[1] - domain[0]) / divX < 20)  divX = divX / 2  
  }

  if (range[1] == range[0]) 
  {
    range[0] = range[0] - 1
    range[1] = range[1] + 1
    divY = .1;
  }
  else
  {
    divY = Math.pow(10,Math.floor(Math.log(range[1] - range[0])/Math.log(10))) / yInterval
    while ((range[1] - range[0]) / divY < 20)  divY = divY / 2
  }

  var scaleX = (domain[1] == domain[0])? width : (width) / (domain[1] - domain[0])
  var scaleY = (range[1] == range[0]) ? height : (height) / (range[1] - range[0])
    
  var startX = Math.floor(domain[0])
  var startY = Math.floor(range[0])

    function rgb(r,g,b) { return typeof r == "string" ? r : typeof g != "undefined" ? "rgb("+r+","+g+","+b+")" : "rgb("+r+")"}
    function toX(ax) {return Math.round(margin + (ax - domain[0]) * scaleX);}
    function toY(ay) {return Math.round(height +topMargin - (ay - range[0]) * scaleY);}
    function fromX(sx) {return (sx-margin) / scaleX + domain[0]}
    function fromY(sy) {return (height + topMargin - sy) / scaleY + range[0]}
    
    function line(x1,y1,x2,y2,c)
    {
        p.beginPath()
        p.strokeStyle = c
        p.moveTo(x1,y1)
        p.lineTo(x2,y2)
        p.stroke()
    }

    function setp(x,y,c)
    {
        p.beginPath()
        p.fillStyle = c
        p.arc(x,y,Number(dots),0,Math.PI*2,false)
        p.fill()
    }

  if (labels)
  {
    for (var i =0; i<labels.length; i++)
    {
      labels[i].cx = toX(labels[i].x)
      labels[i].cy = toY(labels[i].y)
    }
  }

// draw the axis lines
  line(margin,topMargin,margin,height+topMargin,axis)
  line(margin,height+topMargin,canvas.width,height+topMargin,axis)

  function xText(x)
  {
   if (timeseries) return Math.floor(x) + ':' + (Math.floor((x%1)*100)/100).toFixed(2).substr(2)
   x.toFixed(2).replace(/\.*0+$/,'')
  }
//x axis
  for (i=0; ;i++)
  {
   var show = false
   var x = startX + (i * divX)
   if (x < domain[0]) continue
   if (x > domain[1]) break
   var sx = toX(x)

   if (i % (2*xInterval) == 0)
   {
    if (i) line(sx,+topMargin,sx,height+topMargin,grid)
    line(sx,height+topMargin,sx,height+topMargin+6,axis)
    show = true
   }
   else if (i % 2 )
   {
    line(sx,height + 5,sx, height+topMargin , grid)    
   }
   else
   {
    line(sx,topMargin,sx,height+topMargin+5,grid)
   }

   if (show || x == 0)
   {
     labels.push({cx: Math.min(sx,width-6), cy: height+topMargin+8, align:"center", baseline:"top", color:axis, 
        text: xText(x) })
     line(sx,height,sx,height+topMargin+6,axis)
   }
  }

//y axis
  for (i=0; ;i++)
  {
   var show = false
   var y = startY + (i * divY)
   if (y < range[0]) continue
   if (y > range[1]) break
   var sy = toY(y)

   if (i% (2*yInterval) == 0 || y==0)
   {
    if (i) line(margin,sy,canvas.width,sy,grid)
    line(margin-5,sy,margin,sy,axis)
    show = true
   }
   else if (i % 2)
   {
    line(margin - 5, sy, margin , sy , grid)
   }
   else
   {
    line(margin - 5, sy, canvas.width , sy,grid)
   }

    if (show || y == 0)
    {
        labels.push({cx: margin-6, cy: Math.max(sy+2,6), align:"right", baseline:"middle", color:axis,text: y.toFixed(2).replace(/\.*0+$/,'')})
        line(margin-5,sy,margin,sy,axis)
    }
  }
  
  p.globalAlpha = 0.5
  

  if (curves)
  {
    for (var poly in curves)
    {
      p.beginPath()
      p.strokeStyle = c2

      var last = false;
      for (var i = 0; i < width; i++)
      {
       var x = (i / scaleX) + domain[0]
       var y = toY(poly.at(x))

       if (y < height && y > 0)
       {
         if (!last)
         {
           p.moveTo(margin+ i, y)
           last=true
         }
         else
           p.lineTo(margin+ i, y)
       }
       else
       {
        p.stroke()
        p.beginPath()
        last = false
       }
      }
      p.stroke()
    }
  }


  if (points)
  for (var i in points)
  {
    setp(toX(points[i][0]), toY(points[i][1]),c3)
  }

  var last, i, x, y
  for (var s=0; s < series; s++)
  {
    last = null
    //if (s < c1.length)
        p.fillStyle = p.strokeStyle = c1[s%c1.length];
    //else
    //  p.fillStyle = p.strokeStyle = color
   if (lines)
   {
    p.beginPath()
    
    for (i=0;i<data.length;i++)// (!src.eof)
    {
      x = toX(data[i][0])
      y = toY(data[i][s + 1])
      if (last == null)
      {
        p.moveTo(x, y)
      }
      else
      {
        p.lineTo(x, y)
      }
      last = [x,y]
    }   
    p.stroke()
   }
   
   if (dots)
    {
      for (i=0;i<data.length;i++)// (!src.eof)
      {
        x = toX(data[i][0])
        y = toY(data[i][s + 1])
        if (x == NaN || y == NaN) continue
        p.beginPath()
        p.arc(x,y,3,0,Math.PI*2,false)
        p.closePath()
        p.fill()
      }
    }
  }

  var defaultFont = p.font
  for (var i=0; i<labels.length; i++)
  {
   var color = axis;
   if (labels[i].color)
   {
    if( typeof labels[i].color=="string")
     p.fillStyle=labels[i].color;
    else
     p.fillStyle = rgb(labels[i].color[0],labels[i].color[1],labels[i].color[2])
   }
   if (labels[i].font) p.font = labels[i].font
   if (labels[i].baseline) p.textBaseline=labels[i].baseline
   else p.textBaseline = "alphabetic"
   if (labels[i].align) p.textAlign=labels[i].align
   else p.textAlign = "left"
   //$('debug').innerHTML = typeof labels[i]// labels[i].toSource()
   p.fillText(labels[i].text,labels[i].cx, labels[i].cy)
   if (labels[i].font) p.font = defaultFont
  }
  
  return function(sx, sy) {return [fromX(sx), fromY(sy)]}
}

function resizePlot(canvas,data,ratio)
{
  ratio = ratio || 3/4
  var plotScale = 1;
  while (((plotScale+1) * canvas.width) <= 800) plotScale++;

  canvas.width = canvas.width * plotScale

  canvas.height = Math.round(canvas.width * 3/4);
}
