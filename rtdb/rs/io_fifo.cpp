#include "rslib.h"
#include "io_fifo.h"
#pragma hdrstop

/*
   by Jason M Sachs  7/11/2008
 */

#ifndef NO_MEMORY_STREAM

FIFOStream::FIFOStream()
 : Stream(ReadWrite), _inpos(0), _outpos(0), maxsize(1024)
 {
 }

FIFOStream::FIFOStream(int32 size)
: Stream(ReadWrite), _inpos(0), _outpos(0), maxsize(size)
 {
	 if (maxsize < 0)
		 maxsize = 0;
 }

FIFOStream::~FIFOStream()
 {
 }

void FIFOStream::Resize(int32 size)
 {
	maxsize=size;
 }


int32 FIFOStream::seek(int32 offset)
 {
  // undefined operation
  return _outpos;
 }

int32 FIFOStream::goforward(int32 delta)
 {
	 pull(NULL,delta);
	 return _outpos;
 }

int32 FIFOStream::putback(int32 delta)
 {
	 // not possible
	return _outpos;
 }

int32 FIFOStream::size()
 {
    return maxsize;
 }

bool FIFOStream::eof()
 {
	 // hmm. EOF has different meanings for reading out & writing in.
	return false;
 }

int32 FIFOStream::pos()
 {
  return _outpos; // does this make sense?
 }


#endif
