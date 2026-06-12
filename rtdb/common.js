/* $(), addEvent(), insertAfter, getElementsByClass, getCookie, setCookie, 
   deleteCookie are from http://www.dustindiaz.com/top-ten-javascript/

   addEvent() and EventCache are under the CC-LGPL license.

   The rest are too obvious to be protected.
*/

function loadXMLDoc(url,responder,data)
{
    var req  

    function handler()
    {
      if (req && req.readyState == 4 && req.status == 200 && responder != null)
               {responder(req.responseText)}
    }
   
    if (window.XMLHttpRequest)
        req = new XMLHttpRequest();
    else if (window.ActiveXObject)
        req = new ActiveXObject("Microsoft.XMLHTTP");

    req.onreadystatechange = handler
    if (data)
    {
     req.open("POST", url, true);
     req.send(data);
    }
    else
    {
     req.open("GET", url, true);
     req.send(null);
    }
}

function $() {
  var elements = new Array();
  for (var i = 0; i < arguments.length; i++) {
    var element = arguments[i];
    if (typeof element == 'string')
      element = document.getElementById(element);
    if (arguments.length == 1)
      return element;
    elements.push(element);
  }
  return elements;
}

function hide(obj)
{
 var el = document.getElementById(obj);
 el.style.display = 'none';
}

function toggle(obj,caller, color, display) {
  var el = document.getElementById(obj);
  if ( el.style.display != 'none' ) {
    el.style.display = 'none';
    if (caller) caller.style.color=''
  }
  else {
    el.style.display = display || '';
    if (caller) caller.style.color = color
  }
}

function addEvent( obj, type, fn ) {
  if (obj.addEventListener) {
    obj.addEventListener( type, fn, false );
    EventCache.add(obj, type, fn);
  }
  else if (obj.attachEvent) {
    obj["e"+type+fn] = fn;
    obj[type+fn] = function() { obj["e"+type+fn]( window.event ); }
    obj.attachEvent( "on"+type, obj[type+fn] );
    EventCache.add(obj, type, fn);
  }
  else {
    obj["on"+type] = obj["e"+type+fn];
  }
}

var EventCache = function(){
  var listEvents = [];
  return {
    listEvents : listEvents,
    add : function(node, sEventName, fHandler){
      listEvents.push(arguments);
    },
    flush : function(){
      var i, item;
      for(i = listEvents.length - 1; i >= 0; i = i - 1){
        item = listEvents[i];
        if(item[0].removeEventListener){
          item[0].removeEventListener(item[1], item[2], item[3]);
        };
        if(item[1].substring(0, 2) != "on"){
          item[1] = "on" + item[1];
        };
        if(item[0].detachEvent){
          item[0].detachEvent(item[1], item[2]);
        };
        item[0][item[1]] = null;
      };
    }
  };
}();

addEvent(window,'unload',EventCache.flush);

function addLoadEvent(func) {addEvent(window,'load',EventCache.flush);}

function getElementsByClass(searchClass,node,tag) {
  var classElements = new Array();
  if ( node == null )
    node = document;
  if ( tag == null )
    tag = '*';
  var els = node.getElementsByTagName(tag);
  var elsLen = els.length;
  var pattern = new RegExp('(^|\\\\s)'+searchClass+'(\\\\s|$)');
  for (i = 0, j = 0; i < elsLen; i++) {
    if ( pattern.test(els[i].className) ) {
      classElements[j] = els[i];
      j++;
    }
  }
  return classElements;
}

function insertAfter(parent, node, referenceNode) {
  parent.insertBefore(node, referenceNode.nextSibling);
}

if (!Array.prototype.indexOf)
{
  Array.prototype.indexOf = function(elt /*, from*/)
  {
    var len = this.length;

    var from = Number(arguments[1]) || 0;
    from = (from < 0) ? Math.ceil(from) : Math.floor(from);
    if (from < 0)
      from += len;

    for (; from < len; from++)
    {
      if (from in this &&
          this[from] === elt)
        return from;
    }
    return -1;
  };
}

if (!Array.prototype.firstThat)
{
  Array.prototype.firstThat = function(f ,g)
  {
    var len = this.length;
    for (var i=0; i < this.length; i++)
      if (f(this[i],g)) return this[i]
    return null;
  }
}

if (!Array.prototype.unique)
{
    Array.prototype.unique = function () 
    {
        var r = new Array();
        for(var i = 0, n = this.length; i < n; i++)
            if (this.lastIndexOf(this[i]) == i) 
               r.push(this[i]);
        return r;
    }
}

function resizeLastNode(node,margin)
{
    var windowHeight;

    if (typeof window.innerWidth != 'undefined')
        windowHeight = window.innerHeight;
    else if (typeof document.documentElement != 'undefined'
            && typeof document.documentElement.clientWidth != 'undefined'
            && document.documentElement.clientWidth != 0) // IE6 in standards compliant mode (i.e. with a valid doctype as the first line in the document)
        windowHeight = document.documentElement.clientHeight;
    else // older versions of IE
        windowHeight = document.getElementsByTagName('body')[0].clientHeight;

    node.style.height = (windowHeight - node.offsetTop  - (margin||0))+ "px";
}

function getCookie( name ) {
  var start = document.cookie.indexOf( name + "=" );
  var len = start + name.length + 1;
  if ( ( !start ) && ( name != document.cookie.substring( 0, name.length ) ) ) 
  {
    return null;
  }
  if ( start == -1 ) return null;
  var end = document.cookie.indexOf( ';', len );
  if ( end == -1 ) end = document.cookie.length;
  return unescape( document.cookie.substring( len, end ) );
}

function setCookie( name, value, expires, path, domain, secure ) {
  var today = new Date();
  today.setTime( today.getTime() );
  if ( expires ) {
    expires = expires * 1000 * 60 * 60 * 24;
  }
  var expires_date = new Date( today.getTime() + (expires) );
  document.cookie = name+'='+escape( value ) +
    ( ( expires ) ? ';expires='+expires_date.toGMTString() : '' ) + //expires.toGMTString()
    ( ( path ) ? ';path=' + path : '' ) +
    ( ( domain ) ? ';domain=' + domain : '' ) +
    ( ( secure ) ? ';secure' : '' );
}

function deleteCookie( name, path, domain ) {
  if ( getCookie( name ) ) document.cookie = name + '=' +
      ( ( path ) ? ';path=' + path : '') +
      ( ( domain ) ? ';domain=' + domain : '' ) +
      ';expires=Thu, 01-Jan-1970 00:00:01 GMT';
}

function windowClose()
{
 window.close()
 if (!window.closed)
 {
    if (netscape)
    {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        prefserv = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
        prefserv.setBoolPref("dom.allow_scripts_to_close_windows",true)
        window.close()
        prefserv.setBoolPref("dom.allow_scripts_to_close_windows",false)
    }
    
 }
}
