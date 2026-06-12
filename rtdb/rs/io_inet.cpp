#include "rslib.h"
#pragma hdrstop

#ifndef NO_INET_STREAM

#ifdef XP_WIN
#if !defined(_WINSOCK2API_) && !defined(_WINSOCKAPI_)
#include <winsock2.h>
#ifndef IPPROTO_TCP
#define IPPROTO_TCP 0
#endif
#endif
#endif

#ifdef XP_UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#define INVALID_SOCKET 0xFFFFFFFF
#define SOCKET_ERROR -1
//#define AF_INET PF_INET
#define SOCKADDR struct sockaddr
#define HOSTENT hostent
#endif

#ifndef XP_UNIX
#define socklen_t int
#endif

bool InternetServer::Startup()
{
 s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); /* make Internet socket */
 if (s == INVALID_SOCKET) return false;

 memset(&myaddr,0,sizeof(myaddr)); /* host addr */
 myaddr.sin_family = AF_INET;       /* Internet address family */
 myaddr.sin_addr.s_addr = htonl(INADDR_ANY);     /* incomming addr */
 myaddr.sin_port = htons(port);     /* port */

 //1 memset(&myaddr.sin_addr.s_addr,0, sizeof(IN_ADDR)); /* host addr */
 //1 memset(myaddr.sin_zero,0,sizeof(myaddr.sin_zero));

 if (bind(s,(struct sockaddr*)&myaddr,sizeof(myaddr)) == SOCKET_ERROR)
   return false;

 int i = sizeof(myaddr);
if (getsockname(s,(sockaddr*)&myaddr,(socklen_t*)&i) != SOCKET_ERROR)
{
      port = ntohs(myaddr.sin_port);
     hostent* addr;
     addr = gethostbyaddr ((char*)(void*)&myaddr.sin_addr,sizeof(INADDR_ANY),AF_INET);
     if (addr)
     {
        hostinfo.Resize(strlen(addr->h_name) + 10);
        sprintf(hostinfo,"%s:%d",addr->h_name,port);
     }
 }
 if (listen(s,SOMAXCONN) == SOCKET_ERROR)
   return false; /* the destructor calls closesocket and shutdown */

 return true;
}

bool InternetServer::AnyoneWaiting()
{
  fd_set incoming;
  FD_ZERO(&incoming);
  FD_SET(s,&incoming);

  timeval t;
  t.tv_sec=0;
  t.tv_usec = 0;

  if (select(s+1,&incoming,0,0,&t) ==SOCKET_ERROR) return false;

  return FD_ISSET(s,&incoming) != 0;
}


InternetServer::InternetServer(int p)
{
 port = p;
 s = INVALID_SOCKET;
#ifdef XP_WIN
 WSADATA d;
 if (WSAStartup(0x0101, &d) != 0) /* init winsock lib. want v1.1 */
 {
  error = new xdb("Cannot start winsock","file",__FILE__,"line",__LINE__);
  return;
 }
#endif
 if (!Startup())
 {
  error = new xdb("Cannot listen on that port");
  return;
 }
}

InternetServer::~InternetServer()
 {

 if (s != INVALID_SOCKET) /* shutdown and close the socket if needed */
   { shutdown(s, 2);   /* shutdown both channels (read+write) */
#ifdef XP_WIN
    closesocket(s);
#endif
#ifdef XP_UNIX
    close(s);
#endif
   }
#ifdef XP_WIN
   WSACleanup();        /* free winsock lib */
#endif
 }

InternetStream* InternetServer::Accept()
{
 unsigned int t;
 sockaddr_in clientaddr; /* now, get the host name out */
 int len = sizeof(clientaddr);
 t = accept( s,(SOCKADDR*)&clientaddr,(socklen_t*)&len);

 if (t == INVALID_SOCKET)
   return 0;

 int hostaddr = clientaddr.sin_addr.s_addr;
 InternetStream* r;
 try{
 r = new InternetStream(t,hostaddr);
 if (r->error) return 0;
  } catch(...) {return 0;}
 return r;
}

InternetStream::InternetStream(unsigned int socket, int host) : Stream()
{
    port = 0;
   s = socket;

#ifdef XP_WIN
   WSADATA d;
   if (WSAStartup(0x0101, &d) != 0) /* init winsock lib. want v1.1 */
   {
      error = new xdb("Cannot start winsock","file",__FILE__,"line",__LINE__);
      Type=NotOpen;
      return;
   }
#endif

   hostaddr = host;

   hostent* addr;

   addr = gethostbyaddr ((char*)(void*)&hostaddr,sizeof(INADDR_ANY),AF_INET);

   if (addr)
    hostinfo = addr->h_name;

  Type = ReadWrite;
}

/* proxy server from
IE:
HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings\ProxyServer

NN:


*/


InternetStream::InternetStream(const char* url,TNameValueList* headers) : Stream()
{
   hostaddr = 0;
   s = INVALID_SOCKET;
#ifdef XP_WIN
   WSADATA d;
   if (WSAStartup(0x0101, &d) != 0)     /* init winsock lib. want v1.1 */
   {
      WSACleanup();
      error = new xdb("Cannot start winsock","file",__FILE__,"line",__LINE__);
      Type = NotOpen;
      return;
   }
#endif

 TStr Error;
 TStr service, user, password, host, filename, query;
 URLSplit(url, service, user, password, host, filename, query);
// hostinfo = url; //  service://user:password@host:port/filename/query

 if (!*host) host = "localhost";

 if (!*filename) filename = "/";

 if (!init(host,80,&Error)) return; //throw xdb(Error,"host",host,"port",80);

 bool IsProxy = strncasecmp(filename,"http://",7) == 0;

 if (*query) filename = TStr(filename,"?",query); // http://proxy.com/http://server.com/page?query

 size_t l = strcspn(filename,"%+ \t\r\n");
 if (filename[l])
  {
   query=filename;
   filename = 0;
   URLEncodeURL(filename, query);
  }


 const char* c = filename;
 while (*c == '/') c++;

 MemoryStream Request;

 Request << "GET ";

 if (!IsProxy)
  Request << "/";

 Request << c << " HTTP/1.1\r\n"
         << "Host: " << host << "\r\n"
         << "User-Agent: Raosoft/1.0 (jsdb; en-us; " __DATE__ ")\r\n";

 if (headers)
    Request.WriteMIME(*headers);
 else
    Request << "\r\n";

 write((char*)Request,Request.size());
}

#ifdef UNUSED
int InternetStream::SkipHeaders()
{
 TParameterList temp;
 return GetHeaders(temp);
}

int InternetStream::GetHeaders(TNameValueList &n) //returns the status
{
 TStr status;
 readline(status);
 char * c = strchr(status,' ');
 if (!c) return 0;
 int ret = atoi(c+1);
 ReadMIME(n);
 return ret;
}
#endif

InternetStream::InternetStream() : Stream()
{
  hostaddr = 0;
  port= 0;
   s = INVALID_SOCKET;
#ifdef XP_WIN
   WSADATA d;
   if (WSAStartup(0x0101, &d) != 0)     /* init winsock lib. want v1.1 */
   {
      WSACleanup();
      error = new xdb("Cannot start winsock","file",__FILE__,"line",__LINE__);
      Type = NotOpen;
      return;
   }
#endif
}

InternetStream::InternetStream(const char host[],int port) : Stream()
{
   hostaddr = 0;
   s = INVALID_SOCKET;
#ifdef XP_WIN
   WSADATA d;
   if (WSAStartup(0x0101, &d) != 0)     /* init winsock lib. want v1.1 */
   {
      WSACleanup();
      error = new  xdb("Cannot start winsock","file",__FILE__,"line",__LINE__);
      Type = NotOpen;
      return;
   }
#endif
 TStr Error;
 if (!init(host,port,&Error)) return;
}

/* connect to the specified host at the specified port. the host address */
/* can be in the form of "192.168.0.1" or "phoenix.ttdev.com". isnum     */
/* should be set to true if the numeric form is used.                    */
/* return: true  --- ok                                                  */
/*         false --- error                                               */

#ifdef XP_WIN
 int InternetStream::GetLastError()   { return WSAGetLastError();}
#else
 int InternetStream::GetLastError()   { return 0;}
#endif

bool InternetStream::init(const char *phost, int port, TStr* errmsg)
{
   if (s != INVALID_SOCKET) /* shutdown and close the socket if needed */
   { shutdown(s, 2);   /* shutdown both channels (read+write) */
#ifdef XP_WIN
     closesocket(s);
#endif
#ifdef XP_UNIX
     close(s);
#endif
     s = INVALID_SOCKET;
   }

   int opt;
   /* host addr as a 32bit unsigned int */
   uint32 *p;
   HOSTENT *he;
   sockaddr_in svraddr;
   TStr host(phost);

   char * cp = strchr(host,':');
   if (cp)
    {
     int newport = atoi(cp+1);
     if (newport) port = newport;
     *cp=0;
    }

   hostinfo.Resize(strlen(host) + 10);
   sprintf(hostinfo,"%s:%d",(char*)host,port);
   //hostinfo = TStr(host,':',port);

   if (isdigit(host[0]))        /* something like 192.168.0.1 */
   {
      hostaddr = inet_addr(host);      /* convert to 32bit unsigned int */
   }
   else                         /* something like phoenix.ttdev.com */
   {
      he = gethostbyname(host); /* convert to 32bit unsigned int */
      if (!he) {return 0;}
      p = (uint32*)he->h_addr_list[0];
      hostaddr = *p;
   }

#ifdef XP_WIN // synchronous sockets are not defined for win16

#ifndef SO_OPENTYPE
#define SO_OPENTYPE 0x7008
#endif
#ifndef SO_SYNCHRONOUS_NONALERT
#define SO_SYNCHRONOUS_NONALERT 0x20
#endif

   int err;
   int len=sizeof(opt);
   err = getsockopt( /* Get the original setting of SO_OPENTYPE. */
        INVALID_SOCKET,
        SOL_SOCKET,
        SO_OPENTYPE,
        (char *)&opt,
        &len);

   opt = SO_SYNCHRONOUS_NONALERT;
   int originalOpt = opt;

   /* make sure we're gonna create a synchronous socket */
   err = setsockopt(INVALID_SOCKET,
                  SOL_SOCKET,
                  SO_OPENTYPE,
                  (char*)&opt,
                  sizeof(opt));

   if (err != NO_ERROR) return false;
#endif

   s = socket(AF_INET, SOCK_STREAM, 0); /* make Internet socket */

#ifdef XP_WIN
   opt = originalOpt; /* reset the option to default for async sockets */

   err=setsockopt(INVALID_SOCKET, //NT and 95 specific
              SOL_SOCKET,
              SO_OPENTYPE,
              (char*)&opt,
              sizeof(opt));

   if (err != NO_ERROR) return false;
#endif

   if (s == INVALID_SOCKET) return false;  /* did we create the sync socket? */

   svraddr.sin_family = AF_INET;       /* Internet address family */
   svraddr.sin_port = htons(port);     /* port */
   memcpy(&svraddr.sin_addr.s_addr, &hostaddr, sizeof(INADDR_ANY)); /* host addr */
   memset(svraddr.sin_zero,0,sizeof(svraddr.sin_zero));

   if (connect(s, (SOCKADDR*)&svraddr, sizeof(svraddr)) != 0)
   {if (errmsg) {errmsg->Resize(128); sprintf(*errmsg,"%d",GetLastError()); }
    return false;
   }
 Type = ReadWrite;
 return true;
}

void InternetStream::NoDelay()
{
   int opt = 1;

//#ifdef XP_WIN
/* no transmission delay */
   setsockopt(s,
                  IPPROTO_TCP,
                  TCP_NODELAY,
                  (char*)&opt,
                  sizeof(opt));
//#endif
}


InternetStream::~InternetStream()
{ if (s != INVALID_SOCKET) /* shutdown and close the socket if needed */
   {
#ifdef XP_WIN
	OVERLAPPED o ;
	memset(&o,0,sizeof(o));
#ifndef WSAID_DISCONNECTEX
#define WSAID_DISCONNECTEX \
    {0x7fda2e11,0x8630,0x436f,{0xa0, 0x31, 0xf5, 0x36, 0xa6, 0xee, 0xc1, 0x57}}
#endif
	BOOL(PASCAL *DisconnectEx)(  SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved);
	DWORD d;
	GUID g = WSAID_DISCONNECTEX;

	if (WSAIoctl(s,SIO_GET_EXTENSION_FUNCTION_POINTER,&g,sizeof(g),&DisconnectEx,sizeof(DisconnectEx),&d,0,0))
	  DisconnectEx(s, &o, 0, 0);
	else
	  shutdown(s, 2);
	closesocket(s);
#else
	 shutdown(s, 2);   /* shutdown both channels (read+write) */
#endif
#ifdef XP_UNIX
     close(s);
#endif
   }
#ifdef XP_WIN
   WSACleanup();        /* free winsock lib */
#endif
}

#if 0
InternetStream::~InternetStream()
{ if (s != INVALID_SOCKET) /* shutdown and close the socket if needed */
   { shutdown(s, 2);   /* shutdown both channels (read+write) */
#ifdef XP_WIN
     closesocket(s);
#endif
#ifdef XP_UNIX
     close(s);
#endif
   }
#ifdef XP_WIN
   WSACleanup();        /* free winsock lib */
#endif
}
#endif

int InternetStream::write(const char *b, int n)
{/* send a block of bytes. return # of bytes actually sent */
 if (Type == NotOpen) return 0;
 int ret = ::send(s, (const char*)b, n, 0);
 if (ret == SOCKET_ERROR)
   Type = NotOpen;
 return ret;
}

bool InternetStream::canread()
{
 if (Type == NotOpen) return false;

  fd_set incoming;
  FD_ZERO(&incoming);
  FD_SET(s,&incoming);

  timeval t;
  t.tv_sec=0;
  t.tv_usec = 0;

  if (select(s+1,&incoming,0,0,&t) ==SOCKET_ERROR) return false;

  return FD_ISSET(s,&incoming);
}

bool InternetStream::eof()
{
 if (canread() || canwrite()) return false;
 return (Type == NotOpen);
/* if (Type == NotOpen) return true;

  fd_set incoming;
  FD_ZERO(&incoming);
  FD_SET(s,&incoming);

  timeval t;
  t.tv_sec=0;
  t.tv_usec = 0;

  if (select(s+1,0,0,&incoming,&t) ==SOCKET_ERROR) return true;

  return FD_ISSET(s,&outgoing);
*/
}


bool InternetStream::canwrite()
{
 if (Type == NotOpen) return false;

  fd_set outgoing;
  FD_ZERO(&outgoing);
  FD_SET(s,&outgoing);

  timeval t;
  t.tv_sec=0;
  t.tv_usec = 0;

  if (select(s+1,0,&outgoing,0,&t) ==SOCKET_ERROR) return false;

  return FD_ISSET(s,&outgoing);
}


int InternetStream::read(char *b, int n)
{/* receive a block of bytes. return # of bytes actually received */
 if (Type == NotOpen) return 0;
 int ret =0;
/*   if (start)
   {
    if (start->eof())
    {
     start = 0;
    }
    else
    {
     ret = start->read(b,n);
     b += ret;
     n -= ret;
     if (!n) return ret;
    }
   }
 */
   ret += ::recv(s, (char*)b, n, 0);
   if (ret < 0) {Type = NotOpen; return 0;}
   return ret;
}

/* send a line of text. return true or false */
bool  InternetStream::sendln(const char s[])
{
 if (Type == NotOpen) return false;

   int l;
   l = strlen(s);            /* see if it ends with \n */
   if (l > 0 && s[l-1] == '\n') l--;  /* we don't want that */
   if (write(s, l) != l) return false;     /* send the string without trailing \n */
   if (write("\r\n", 2) != 2) return false;/* send \r\n now */
   return true;
}

/* receive a line of text. \r\n signals the end but */
/* it is not stored into s. return true or false    */
bool  InternetStream::recvln(char s[], int maxlen)
{
 if (Type == NotOpen) return false;
 bool ret = false;
   int i=0;
   char ch;
   s[0]=0;
   while (i < maxlen-1)
   {
      if (read(&ch,1)!=1) return false; /* receive a char */
      if (ch == '\n') {ret = true; break;}
      if (ch == '\r')     /* \r\n signals the end */
      { read(&ch,1); ret=true; break; }
      s[i] = ch;          /* save the char and proceed */
      i++;
   }
   s[i] = 0;              /* NULL terminate the string */
   return ret;
}

#endif
