#include "rslib.h"
#pragma hdrstop
#include <stdio.h>

#ifndef NO_FILE_STREAM

#define BUFSIZE 65535

FileStream::FileStream() : Stream(NotOpen)
{
File = 0;
#ifdef XP_WIN
IsText = false;
#endif
}

FileStream::FileStream (const char * filename,TOpenMode mode, TType _Type):
 Stream(_Type)
{
 int OpenMode = mode;
 Init (filename, _Type, OpenMode&OMText,!(OpenMode&OMUnbuffered)) ;
 if (error) throw xdb(error);
}

void FileStream::Init (const char * filename,TType _Type,
                                bool textmode,bool buffered)

{
EndFileMarker=0;
FileName = filename;
Type = _Type;

#ifdef XP_WIN
 File = INVALID_HANDLE_VALUE;
 Replace(FileName,'/','\\');
#endif

 filename=FileName;
 canClose = false;

#ifdef XP_WIN
ReadPosition = 0;
Buffer = 0;
BufMode = BufNone;

if (*FileName == 0 || FileName == "stdout" ||
     FileName == "stdin" || FileName == "stderr") //stdio
{
  if ((Type == WriteOnly && *FileName == 0) || FileName == "stdout")
  {
   Type = AppendOnly;
   File = GetStdHandle(STD_OUTPUT_HANDLE);
   FileName = "stdout";
   buffered = false;
  }
  else
  if ((Type == ReadOnly && *FileName == 0) || FileName == "stdin")
  {
   Type = ReadOnly;
   File = GetStdHandle(STD_INPUT_HANDLE);
   FileName = "stdin";
   buffered = false;
  }
  else
  if (FileName == "stderr")
  {
   Type = NotOpen;
   File = GetStdHandle(STD_ERROR_HANDLE);
   FileName = "stderr";
   buffered = false;
  }

  if (File == 0)
   {
    error = new xdb("File open failed","name",FileName);
    Type = NotOpen;
    return;
   }
  canClose = false;
//  FileName=0;
}
else
{
DWORD DesiredAccess,ShareMode,Creation;
 switch (Type)
  {case NotOpen:
   case ReadOnly: DesiredAccess = GENERIC_READ;
                  ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
                  Creation = OPEN_EXISTING;
                  break;
   case Create:
   case ReadWrite: DesiredAccess = GENERIC_READ | GENERIC_WRITE;
                   ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
                   Creation = (Type == ReadWrite) ? OPEN_ALWAYS : CREATE_ALWAYS;
                   break; //create if the file doesn't exist.
   case AppendOnly:
   case WriteOnly: DesiredAccess = GENERIC_WRITE;
                   ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
                   Creation = (Type == AppendOnly) ? OPEN_ALWAYS : CREATE_ALWAYS;
  }

    SECURITY_ATTRIBUTES sec;
    sec.nLength = sizeof(sec);
    sec.lpSecurityDescriptor = NULL;
    sec.bInheritHandle = true;

    OSVERSIONINFO versioninfo = {sizeof(OSVERSIONINFO),0,0,0,0};
    GetVersionEx(&versioninfo);

    if (versioninfo.dwPlatformId == VER_PLATFORM_WIN32_NT) //unicode
     File = CreateFileW(WStr(filename),DesiredAccess,ShareMode,&sec,
                  Creation,FILE_FLAG_SEQUENTIAL_SCAN,0);

    if (File == INVALID_HANDLE_VALUE)
     File = CreateFileA(filename,DesiredAccess,ShareMode,&sec,
                  Creation,FILE_FLAG_SEQUENTIAL_SCAN,0);

  if (File == INVALID_HANDLE_VALUE)
   {
    const char * mode;
    switch(Type)
    {
     case ReadOnly: mode="r"; break;
     case ReadWrite: mode="rw";break;
     case AppendOnly: mode="a";break;
     case WriteOnly: mode="w";break;
     default: mode="unknown";
    }
    error = new xdb("File open failed","name",filename,"mode",mode);
    Type = NotOpen;
    return;
   }

  canClose = true;

  if (Type == AppendOnly) SetFilePointer(File,0,0,FILE_END);
  //prepare for appending
  }
IsText=textmode;
//if (Type == WriteOnly || Type == ReadWrite)

if (buffered)
 Buffer = new MemoryStream(BUFSIZE);
else
 Buffer = 0;

#else

 if ( !strcasecmp(filename,"stdin"))
 {
    File=stdin;
    Type = ReadOnly;
 }
 else if ( !strcasecmp(filename,"stdout"))
 {
    File=stdout;
    Type = WriteOnly;
 }
 else if ( !strcasecmp(filename,"stderr"))
 {
    File=stderr;
    Type = WriteOnly;
 }
 else
 {
  char flags[4];
  strcpy(flags,"r");
  switch (Type)
  {
   case IOCreate:
       strcpy(flags,"w+"); //if (FileExists(filename) && FileAttributes(filename) & 2)
       //else { Type = ReadOnly; }
       break; //create if the file doesn't exist.
   case ReadWrite:
       strcpy(flags,"r+"); //if (FileExists(filename) && FileAttributes(filename) & 2)
       //else { Type = ReadOnly; }
       break; //create if the file doesn't exist.
   case WriteOnly: strcpy(flags,"w"); break;
   case AppendOnly:  strcpy(flags,"a");
  }

  if (textmode) strcat(flags,"t"); else strcat(flags,"b");
// printf("fopen %s %s\n",filename,flags);
  File = fopen(filename,flags);
     if (File == NULL)
        {
         error = new xdb("File open failed","name",filename,"mode",flags);
         perror("FileStream");
         Type = NotOpen;
         return;
        }
  canClose = true;
 }
#endif
};

#ifdef XP_WIN
void FileStream::FlushCache(EBufMode Mode)
{
//if (Mode == BufMode) return; /* don't bother */
if (!Buffer) return;

//clear the buffers if necessary
if (BufMode == BufRead)
 {
   int32 offset = Buffer->size() - Buffer->pos();
   ReadPosition = SetFilePointer(File,-offset,0,FILE_CURRENT);
 }
else if (BufMode == BufWrite)
 {
  DWORD written=0;
  WriteFile(File,(char*)*Buffer,Buffer->pos(),&written,0);
 }

 Buffer->Clear(BUFSIZE); // rewind the buffer to start over
 BufMode = Mode; // store the buffer size
}
#endif

FileStream::~FileStream()
{
#ifdef XP_WIN
 if (Buffer)
 {
    FlushCache(BufNone);
    delete Buffer;
 }

 if (canClose && File)
  {
   CloseHandle(File);
  }

#else
 if (canClose && File) // && File != stdin && File != stdout && File != stderr)
   fclose(File);
#endif
}

int32 FileStream::seek(int32 offset)
{
if (Type == NotOpen) return 0;
if (EndFileMarker) offset = min(offset,EndFileMarker);
#ifdef XP_WIN
FlushCache(BufNone);
if (offset == -1) return SetFilePointer(File,0,0,FILE_END);
return SetFilePointer(File,offset,0,FILE_BEGIN);
#else
if (offset == -1)
   return fseek(File,0,SEEK_END) == 0 ? 0 : -1;
return fseek(File,offset,SEEK_SET) == 0 ? offset : -1;
#endif
}

int32 FileStream::size()
{
if (Type == NotOpen) return 0;
int32 Size;
#ifdef XP_WIN
FlushCache(BufNone);
Size = GetFileSize(File,0);
#else
struct stat st;
int reV=stat(FileName,&st);
if(reV==-1)
{
 int32 orig = pos();
 Size = fseek(File,0,SEEK_END);
 fseek(File,orig,SEEK_SET);
}
Size=st.st_size;
//Size = fseek(File,0,SEEK_END);
//fseek(orig);
#endif
return EndFileMarker ? min(EndFileMarker,Size) : Size;
}

int32 FileStream::pos()
{
if (Type == NotOpen) return 0;
#ifdef XP_WIN
 if (ReadPosition != 0 && Buffer != NULL)
   return ReadPosition - Buffer->Maxsize + Buffer->Position;

 FlushCache(BufNone);
 return SetFilePointer(File,0,0,FILE_CURRENT);
#else
 return ftell(File);
#endif
}

bool FileStream::eof()
{
if (Type == NotOpen) return 0;
#ifdef XP_WIN
 if (BufMode == BufRead)
   if (Buffer->Position < Buffer->Maxsize) return false;

 return pos() >= size(); //not a good function to call.
#else
 if (EndFileMarker) return pos() >= size();
 return feof(File);
#endif
}

int32 FileStream::goforward(int32 delta)
{
if (Type == NotOpen) return 0;
#ifdef XP_WIN
FlushCache(BufNone);
if (EndFileMarker) {return seek(pos() + delta);}
return SetFilePointer(File,delta,0,FILE_CURRENT);
#else
if (EndFileMarker) {return seek(pos() + delta);}
return fseek(File,delta,SEEK_CUR);
#endif
}//seeks forward

int32 FileStream::putback(int32 delta)
{
if (Type == NotOpen) return 0;
#ifdef XP_WIN
FlushCache(BufNone);
if (EndFileMarker) {return seek(pos() - delta);}
return SetFilePointer(File,-delta,0,FILE_CURRENT);
#else
if (EndFileMarker) {return seek(pos() - delta);}
return fseek(File,-delta,SEEK_CUR);
#endif
}

#ifdef XP_WIN
int memspn(const char * src, int maxcopy)
 {
  for (int i = 0 ; i<maxcopy; i++)
   if (src[i] == '\r' || src[i] == '\n') return i;
  return maxcopy;
 }

void bufwritetext(MemoryStream& buf, const char * src,int maxcopy)
 {
  while (maxcopy>0)
  {
   int i = memspn(src,maxcopy);
   if (i)
    {
     buf.write(src,i);
     src += i;
     maxcopy -= i;
    }
   else if (*src == '\n')
    { buf.write("\r\n",2); src++; maxcopy--; }
   else //some other unwanted control character
    { src++; maxcopy--; }
 }
}

int bufreadtext(char * dest,MemoryStream& buf,int maxcopy,int MaxSize)
 {
  int count = 0;
  //int startpos = buf.pos();
  while (count < maxcopy)
  {
   int pos = buf.pos();
   char * src = buf + pos;
   int maxlen = min (maxcopy - count, MaxSize - pos);
   if (!maxlen) break;
   int i = memspn((char*)src,maxlen);
   if (i)
    {
     i = buf.read(dest+count,i);
     if (!i) break;
     count += i;
    }
   else if (*src == '\n')
    { dest[count] = '\n'; count++; buf.goforward(1); }
   else //some unwanted control character
    { buf.goforward(1); }
  }
  return count;
 }

DWORD writetext(HANDLE File,const char * src, int maxcopy)
 {
   DWORD written=0;
   DWORD thiswrite;
   while (maxcopy>0)
   {
    int i = memspn(src,maxcopy);
    if (i)
    {
     WriteFile(File,src,i,&thiswrite,0);
     written+=thiswrite;
     src += i;
     maxcopy -= i;
    }
    else if (*src == '\n')
    {
     WriteFile(File,"\r\n",2,&thiswrite,0);
     written+=thiswrite;
     src ++;
     maxcopy --;
    }
    else
    {
     src++;
     maxcopy--;
     thiswrite=1;
    }
    if (!thiswrite) break;
   }
   return written;
 }
#endif

int FileStream::read(char * dest,int maxcopy)
{
if (Type == NotOpen) return 0;
if (EndFileMarker)
 {
  maxcopy = (int)min((int32)maxcopy,EndFileMarker-pos());
 }

if (maxcopy <= 0) return 0;

#ifdef XP_WIN
//DWORD start = pos();
//LockFile(File,start,0,maxcopy,0);
if (BufMode != BufRead) FlushCache(BufRead);
DWORD bytesread=0;

// first, read bytes from the buffer
if (Buffer)
 {
  int x = Buffer->size() - Buffer->pos();
  x = min(x,maxcopy); // does the buffer have anything in it?
  if (x > 0)
   {
    if (IsText)
     x = bufreadtext(dest,*Buffer,x,Buffer->Maxsize);
    else
     x = Buffer->read(dest,x);

    if (maxcopy == x) return x; // done!

    maxcopy -= x;
    dest += x;
    bytesread += x;
   }
    // else the buffer is empty, so reset the buffer.
 }

if (Buffer)
  {
   Buffer->Maxsize = 0;
   Buffer->Position = 0;
  }

if (Buffer && maxcopy < BUFSIZE)
 { // mark the buffer as empty
  while (maxcopy > 0)
  {
   DWORD thisread=0;
   ReadFile(File,Buffer->Mem.buf,Buffer->Mem.size,&thisread,0);
   ReadPosition += thisread;
   Buffer->Maxsize = thisread;
   int x = min((int)thisread,(int)maxcopy);
   if (!x) break; // EOF

   if (IsText)
     x = bufreadtext(dest,*Buffer,x,Buffer->Maxsize);
   else
     x = Buffer->read(dest,x);

   bytesread += x;
   maxcopy -= x;
   dest += x;
  }
 }
else
 { // unbuffered read
   ReadFile(File,dest,maxcopy,&bytesread,0);
   if (IsText)
   {
    maxcopy = RemoveChar(dest,'\r',bytesread);
    dest += bytesread-maxcopy;

    DWORD thisread;
    while (maxcopy && ReadFile(File,dest,maxcopy,&thisread,0))
     {
      if (!thisread) break;
      maxcopy = RemoveChar(dest,'\r',thisread);
      dest += bytesread-maxcopy;
     }
   }
 }

//UnlockFile(File,start,0,maxcopy,0);
return bytesread;
#else
return maxcopy ? ::fread(dest,1,maxcopy,File) : 0;
#endif
};

int FileStream::write(const char * src,int maxcopy)
{
if (Type == NotOpen) return 0;
if (EndFileMarker) {maxcopy = (int)min((int32)maxcopy,EndFileMarker-pos());}

if (!maxcopy) return 0;

#ifdef XP_WIN
if (BufMode != BufWrite) FlushCache(BufWrite);

DWORD written = 0;

if (Type == AppendOnly) SetFilePointer(File,0,0,FILE_END);

if (Buffer) //buffer nearly full: write the buffer, then clear it
 {
  int bufsize = Buffer->pos();
  if (maxcopy + bufsize >= BUFSIZE)
   {
    WriteFile(File,(char*)*Buffer,bufsize,&written,0);
    Buffer->seek(0);
   }
 }

if (Buffer)
 {
  if (IsText) //always buffer text writes if possible
   {
    bufwritetext(*Buffer,src,maxcopy);
    return maxcopy;
   }
  else if (maxcopy < BUFSIZE)
   {
    Buffer->write(src,maxcopy);
    return maxcopy;
   }
  else
   {
    WriteFile(File,src,maxcopy,&written,0);
    return written;
   }
 }
else
 {
  if (IsText)
  {
   return writetext(File,src,maxcopy);
  }
  else
  {
   WriteFile(File,src,maxcopy,&written,0);
   return written;
  }
 }
#else
return ::fwrite(src,1,maxcopy,File);
#endif
};

//---------------------------------------------------------------------------


//--------------------------------------------------------------------

#ifndef XP_WIN
bool CopyFile(const char * source, const char * dest, bool DontErase)
{
 if (DontErase) if (FileExists(dest)) return false;

 FileStream D(dest,Stream::OMBinary,
                   Stream::WriteOnly);

 FileStream S(source,Stream::OMBinary,
                     Stream::ReadOnly);

 if (D.error || S.error) return false;
 D.Append(S);
 D.SetEndOfFile(D.pos());
}
#endif


bool AppendFile(const char * source, const char * dest, bool DontErase)
{
try {
 if (DontErase && FileExists(dest)) return false;
 FileStream D(dest,Stream::OMBinary,Stream::AppendOnly);

 FileStream S(source,Stream::OMBinary,Stream::ReadOnly);

 if (D.error || S.error) return false;
 D.Append(S);
 } catch(...) {return false;}
 return true;
}
#endif
