#include "rslib.h"
#pragma hdrstop

#ifndef NO_MEMORY_STREAM

MemoryStream::MemoryStream()
 : Stream(ReadWrite), Position(0), Mem(1024)
 {
  Maxsize = 0;
 }

MemoryStream::MemoryStream(int32 size)
: Stream(ReadWrite), Position(0), Mem(size)
 {
  Maxsize = 0;
 }

MemoryStream::~MemoryStream()
 {
 }

void MemoryStream::Resize(int32 size)
 {
  Mem.Resize(size);
  Maxsize=size;
  Position = min(Position,Maxsize);
 }

void MemoryStream::Clear(int32 size)
 {
  if (Mem.size > 16384) Mem.Resize(size);
  Position = 0;
  Maxsize = 0;
 }

int32 MemoryStream::seek(int32 offset)
 {
  Position = min((size_t)offset,Mem.size);
  return Position;
 }

int32 MemoryStream::goforward(int32 delta)
 {
  return seek(Position+delta);
 }

int32 MemoryStream::putback(int32 delta)
 {
  return seek(Position-delta);
 }

int32 MemoryStream::size()
 {
    return Maxsize;
 }

bool MemoryStream::eof()
 {
  return Maxsize <= Position;
 }

int32 MemoryStream::pos()
 {
  return Position;
 }

int MemoryStream::read(char * dest,int maxcopy)
  {
       if ((maxcopy<=0)||(!dest)) return 0;

       int32 s = Maxsize - Position ;

       if (s > maxcopy) s = maxcopy;
       // s is now the number of bytes to read

       if ( s <= 0 ) return 0;

       char * c =(char*) Mem;
       if (!c) return 0;

       c += Position;

       memcpy(dest,(char*)c,s);
       Position += s;
       return s;
      };

int MemoryStream::write(const char * src,int maxcopy)
      {
       if ((maxcopy<=0)||(!src)) return 0;

       int32 s = Mem.size - Position ;
       if (maxcopy > s) //expand the buffer
         {
          int32 newsize = maxcopy + Position + 1024;
          Mem.Resize(newsize); //always get 1k extra
          s = Mem.size - Position;
          if (maxcopy > s) return 0;
         }

       if ( s <= 0 ) return 0;//something is wrong

       char * c = (char*)Mem;
       if (!c) return 0;

       c = c + Position;

       memcpy((char*)(c),src,maxcopy);
       Position += maxcopy;

       Maxsize = max(Position,Maxsize); //buffer terminator
       return maxcopy;
      };

//------------------------------------------------------------

ByteStream::ByteStream(char * _Buf,int32 _Size)
:Stream(ReadWrite)
{
 Maxsize = _Size ? _Size : strlen(_Buf);
 Buf=_Buf;
 Position=0;
}

ByteStream::ByteStream(const char * _Buf,int32 _Size)
:Stream(ReadOnly)
{
 Maxsize = _Size ? _Size : strlen(_Buf);
 Buf=(char*)_Buf;
 Position=0;
}

ByteStream::ByteStream(const uint16 * _Buf,int32 _Size)
:Stream(ReadOnly)
{
 Maxsize = _Size ? _Size : sizeof(uint16)*ucslen(_Buf);
 Buf = (char*)_Buf;
 Position = 0;
}

ByteStream::ByteStream(uint16 * _Buf,int32 _Size)
:Stream(ReadWrite)
{
 Maxsize = _Size ? _Size : sizeof(uint16)*ucslen(_Buf);
 Buf = (char*)_Buf;
 Position = 0;
}

ByteStream::~ByteStream()
{
}

bool ByteStream::eof() {return Maxsize <= Position;}

int32 ByteStream::seek(int32 offset)
{
 if (offset < 0) offset = Maxsize;
 return Position = min(offset,Maxsize);
}

int32 ByteStream::size()
{return Maxsize;}

int32 ByteStream::pos()
{return Position;}

int32 ByteStream::goforward(int32 delta)
{ return seek(delta+Position); }

int32 ByteStream::putback(int32 delta)
{ return seek(Position-delta); }

int ByteStream::read(char * dest,int maxcopy)
{
       if ((maxcopy<=0)||(!dest)) return 0;

       int32 s = Maxsize - Position ;

       if (s > maxcopy) s = maxcopy;
       // s is now the number of bytes to read

       if ( s <= 0 ) return 0;

       char * c = Buf;
       if (!c) return 0;

       c += Position;

       memcpy(dest,(char*)c,s);
       Position += s;
       return s;
}

int ByteStream::write(const char * src,int maxcopy)
{
   if (!(Type & IOWrite)) return 0;

       if ((maxcopy<=0)||(!src)) return 0;

       int32 s = Maxsize - Position ;
       if (maxcopy > s) maxcopy = s;

       char * c = Buf;
       if (!c) return 0;

       c += Position;

       memcpy((char*)c,src,maxcopy);
       Position += maxcopy;
       return maxcopy;
}

#endif
