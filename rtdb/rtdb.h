#ifndef _PICTURE_SERVER_H
#define _PICTURE_SERVER_H

#ifdef XP_UNIX
#include <pthread.h>
#include <unistd.h>
#endif

#ifdef XP_UNIX
#define InitializeCriticalSection(x) pthread_mutex_init(x,NULL)
#define CRITICAL_SECTION pthread_mutex_t
#define EnterCriticalSection pthread_mutex_lock
#define TryEnterCriticalSection	pthread_mutex_trylock
#define LeaveCriticalSection	pthread_mutex_unlock
#define DeleteCriticalSection	pthread_mutex_destroy
#define Sleep(x) usleep(1000*x)
#endif

//#define STATE_LENGTH 16

/** \mainpage

The real-time database server maintains a wide table with many columns (channels) and a single row. Each transaction modifies one cell in that table, and is identified by a sequential ID#, starting with 1. A history of the system can be reconstructed by assembling the transactions in chronological order. You can also create a virtual table showing the history of a column with the select command.

Transactions are stored in memory in reverse-chronological order. If you want to query the database, you usually
askfor everything that's happened since your last query.

Publishers connect on socket 160, subscribers on 140.

Command line

<tt>Usage: rtdb [-help] [-stop] [-nogui] [-publish port] [-subscribe port] [directory]</tt>
<br><tt>-help</tt> prints this message
<br><tt>-stop</tt> shuts down the running database on this host
<br><tt>-subscribe #</tt> overrides the subscriber port number (usually 140)
<br><tt>-publish #</tt> overrides the publisher port number (usually 160)
<br><tt>-nogui #</tt> doesn't load the monitor GUI

Publisher commands

<ul>
  <li>Adds a channel
  <br><tt>>channel name
  <br>ok {number}
  </tt>

  <li>Get the current time (for synchronization).
  <br><tt>>time [channel]
  </tt>
  <br>Reply
  <br><tt>ok {ms_since_start} {current_UTC}</tt>
  <br>If a channel name is included, a timestamp will be entered into the database in that channel.

  <li>Update the database.
  <br><tt>>insert channel [length] [opaque] [time] [CRC]
  <br>{data}
  </tt>
  <br>Reply
  <br><tt>ok {id} </tt>
  <br>If CRC is omitted, it will be calculated using adler32, with a seed of -1.
  <br>If time is omitted, it will be ms since the database opened

  <li>Shut down the database
  <br><tt>>quit
  </tt>
</ul>

Subscriber commands

<ul>
  <li>Look up a channel name
  <br><tt>>channel name
  <br>ok number
 <br> error 'name' not found
  </tt>


  <li>Subscribes to changes on that channel. Will create a channel if one of that name doesn't already exist. When that channel changes, the server will send out-of-band "data" messages.
  <br><tt>>listen [channel name or number]
  </tt>

 <li> Switches verbose mode on or off. Verbose=1 sends "data" messages to "listen" requests, verbose=0 sends "info" messages instead.
  <br><tt>>verbose [1|0]
  </tt>

  <li>Stop listening to a channel (or blank for all)
  <br><tt>>stop [channel name or number]
  <br>ok
  </tt>

  <li>Return the last entry from each channel on or before age
  <br><tt>>snapshot [age]
  <br>data id channel length event time crc
  <br>[bytes]
  <br>...
  <br>ok
  </tt>

  <li>Replays the next change to each channel caused by the 'age' event.
  <br><tt>>replay [age]
  <br>data id channel length event time crc
  <br>[bytes]
  <br>...
  <br>ok
  </tt>

  <li>Describes all updates to a channel after event id
  <br><tt>>digest [channel name or number] [id]
  <br>info id channel length event time crc
  <br>...
  <br>ok
  </tt>

  <li>Returns all updates to a channel after event id
  <br><tt>>select [channel name or number] [id]
  <br>data id channel length event time crc
  <br>{data}
  <br>...
  <br>ok
  </tt>

  <li>What channels are available?
  <br><tt>>channels
  <br>channel_1_name channel_2_name channel_3_name ...
  </tt>

  <li>Describe a data update record
  <br><tt>>info id
  <br>info id channel length event time crc
  </tt><br>or<tt>
  <br>error 'id' not found
  </tt>

  <li>Fetch a data update record
  <br><tt>>fetch id
  <br>data id channel length event time crc
  <br>{data}
  </tt><br>or<tt>
  <br>error 'id' not found
  </tt>

  <li>Close the connection gracefully
  <br><tt>>close
  </tt>

  <li>Shut down the database
  <br><tt>>quit
  </tt>

</ul>
*/

///Header for each transaction. id is sequential, "event" is the client's event number (optional)
struct Header
{
	uint32 id; // sequential id number assigned to the transaction
	uint32 channel;
	uint32 length; //
	uint32 event;
	uint32 time;
	uint32 CRC;
};

///An entry in the status table
class Datum
{
	public:
	Datum *next, *prev;
	Header header;
	char* data;
	size_t length;

	Datum(Header& h) {memcpy(&header,&h,sizeof(Header)); data = new char[h.length+1]; next=prev=0; length=0; }
	~Datum() {delete[] data;}
	bool Fill(Stream& s);
};


/**
Publishers connect on port 160.


*/
class Publisher
{
	public:
	MemoryStream command;
	TPointer<InternetStream> stream;
	Publisher(InternetStream* s): stream(s) {}
	bool read()
	{
		char c[2];
		c[1]=0;
		if(stream) while (stream->canread())
		{
			if (!stream->read(c,1))
				return false;
			if (c[0] == '\r')
				continue;
			if (c[0] == '\n')
				return (command.size()>0);
			else
				command << c;
		}
		return false;
	}
	void reset()
	{
		command.Clear();
	}
};

/**
Subscribers connect on port 140.


*/

class Subscriber
{
	public:
	unsigned long lastId;
	bool verbose;
	TIndexList channels;
	MemoryStream command;
	TPointer<InternetStream> stream;
	Subscriber(InternetStream* s): stream(s) {verbose=true; lastId = 0;}

	bool read()
	{
		char c[2];
		c[1]=0;
		while (stream->canread())
		{
			if (!stream->read(c,1))
				return false;
			if (c[0] == '\r')
				continue;
			if (c[0] == '\n')
				return (command.size()>0);
			else
				command << c;
		}
		return false;
	}
	void reset()
	{
		command.Clear();
	}
};

///Database class
class Database
{
	public:
	Datum* items;
	TStr directory;
	unsigned long nextId;
	unsigned long nextEvent; //if client wants an autoincrementer
	CRITICAL_SECTION lock;
	TStringList channels;
	bool canWrite;

	Database(const char* dir, bool writeable = true);

	Datum* Data(unsigned id = 0, unsigned channel = 0);

	Datum* Snapshot(unsigned channel, unsigned id = 0);

	Datum* Replay(unsigned channel, unsigned id = 0);

	bool Insert(Header &header, Stream& src, bool save = true, bool calcCRC = false);

	bool Insert(unsigned channel, const char* text, unsigned event, unsigned time);

	bool Command(const char* cmd, Subscriber& sub);

	bool Command(const char* cmd, Publisher& publish);

	unsigned Query(Stream& out, unsigned channel, unsigned after, bool verbose);

	unsigned QueryText(Stream& out, unsigned channel, unsigned after, unsigned event);

//	bool Fetch(unsigned id);

	size_t AddChannel(const char* name, bool save = true);

	const char* Name(unsigned ch);

	size_t Channel(const char* name, bool canAdd = false); //valid channel numbers start at 1

	void writeHeader(Datum*r, Stream&s);

	bool readHeader(Header& h, Stream&s);

	void Summary(Stream& out, TStringList& detail, unsigned before = 0, bool all = false, unsigned eventChannel = 0);

	~Database();

	protected:
	void Insert(Datum* last, bool save, bool calcCRC);
};


#endif
