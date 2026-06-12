#include "rslib.h"
#include "rtdb.h"
#include <time.h>
//TParameterList GlobalOptions;

#define DEBUG 0

long startTime = GetTickCount();
char startDateTime[32];

char* writeTime(int t)
{
	static char s[32];
	//if (t < 1000000)
	//	sprintf(s,"%06d",t);
	//else
		sprintf(s,"%d",t);
	return s;
}

void Database::writeHeader(Datum*r, Stream&s)
{
	s << "id: " << r->header.id << "\n";
	s << "channel: " << Name(r->header.channel) << "\n";
	s << "event: " << r->header.event << "\n";
	s << "length: " << r->header.length << "\n";
	s << "time: " << writeTime(r->header.time) << "\n";
	s << "CRC: " << r->header.CRC << "\n";
	s << "\n";
}

bool Database::readHeader(Header& h, Stream&s)
{
	static TParameterList m;
	if (m.Count() && m.Count() != 6) m.Clear();
	if (!s.ReadMIME(m)) return false;
	h.id = m.GetInt("id");
	h.length = m.GetInt("length");
	h.channel = Channel(m.Get("channel"));
//printf("%d %s %d\n",h.id ,m.Get("channel"),h.channel);
//if (!h.id) printf("pos %s %d\n",s.filename(),s.pos());
	h.event = m.GetInt("event");
	h.time = atoi(m.Get("time"));
	h.CRC = m.GetInt("CRC");
	return true;
}

bool Datum::Fill(Stream& s)
{
	int timeout = 0;
	if (!header.length)
	{
		delete[] data;
		data = new char[256];
		data[255]=0;
		length = s.readline(data,255);
		if (length && data[length-1] == '\r') data[--length]=0;
		header.length = length;
//if (DEBUG) printf("filled %s\n",data);

		return true;
	}

	length = 0;
	while (timeout < 100 && length < header.length)
	{
		int delta = s.read(data + length, header.length - length);
		length += delta;
		data[length] = 0;
		if (length == header.length) return true;
		if (!delta)
		{
			Sleep(0);
			timeout++;
		}
	}
	return false;
}

Database::Database(const char* dir, bool writeable) : directory(dir), items(0)
{
/*
Files are 0000001.dat, 00000002.dat, etc.
*/
	canWrite = true;
	InitializeCriticalSection(&lock);
	if (writeable) MakeDirectoryExist(directory);
	AddBackslash(directory);

	//TStringList files;
	//GetWildCardFileNames(TStr(directory,"*.dat"), files);

	MemoryStream m;
	Header h;
	nextId = 1;
	nextEvent = 1;
	unsigned max = 0;

	TStr index(directory,"channels.txt");
	if (FileExists(index))
	{
		FileStream f(index);
		char line[256];
		while (!f.eof())
		{
			f.readline(line,256);
			Replace(line,'\r',0);
		    AddChannel(line,false);
	  	}
	}
	else if (writeable)
	if (!FileExists(TStr(directory,"00000001.dat"))) AddChannel("note");

	for (size_t i=1; ; i++)
	{
		m.Clear();
		char temp[64];
		sprintf(temp,"%08d.dat",i);
		TStr filename(directory,temp);
if (DEBUG) printf("Reading %s\n",(char*)filename);
		if (!FileExists(filename)) break;
		FileStream f(filename);
		while (!f.eof())
		{
			if (!readHeader(h,f))  //tolerate blank lines
			 continue;
			 //if (!readHeader(h,f))
			 // break;
			Insert(h, f, false);
			if (h.id > max) max = h.id;
		}
	}
	if (max >= nextId) nextId = max + 1;
	canWrite = writeable;
}

Database::~Database()
{
	EnterCriticalSection(&lock);
	while (items)
	{
		Datum * n = items->next;
		delete items;
		items = n;
	}
	LeaveCriticalSection(&lock);
	DeleteCriticalSection(&lock);
}

bool Database::Insert(Header &header, Stream& src, bool save, bool calcCRC)
{
	if (!canWrite) return false;
	if (!header.channel)
	 header.channel = 1;

	Datum * last = new Datum(header);
	if (!last->Fill(src))
	{
		delete last;
		return false;
	}

	Insert(last, save, calcCRC);
	header.id = last->header.id;
	return last->length == header.length;
}

bool Database::Insert(unsigned channel, const char* text, unsigned event, unsigned time)
{
	if (!canWrite) return false;
	Header header;
	header.id = 0;
	header.event = event;
	header.length = text ? strlen(text) : 0;
	header.time = time;
	header.channel = channel;
	header.CRC = crc32(-1,text,header.length);

	if (!header.channel)
	 header.channel = 1;

	Datum * last = new Datum(header);
	memcpy(last->data,text,header.length);
	last->length = header.length;

	Insert(last, true, true);
	return true;
}

void Database::Insert(Datum* last, bool save, bool calcCRC)
{
	if (!canWrite) return;
	if (items) items->prev = last;
	last->next = items;

	if (calcCRC)
		last->header.CRC = crc32(-1,last->data,last->length);

	EnterCriticalSection(&lock);
	unsigned id = last->header.id = nextId++;
	items = last;
	unsigned event = last->header.event;
	if (event > nextEvent) nextEvent = event + 1;
	LeaveCriticalSection(&lock);

	unsigned long file = (id / 65536) + 1;

	if (save)
	{
		char x[32];
		sprintf(x,"%08d.dat",file);
		FileStream out(TStr(directory,x),FileStream::OMBinary,FileStream::AppendOnly);
		writeHeader(last,out);
		out.write(last->data,last->length);
		out.write("\n\n",2); //extra newlines don't hurt anything.
	}
}

bool Has(TIndexList& l, unsigned x)
{
	for (size_t i=0; i<l.count; i++)
	{
		if (l[i] == x) return true;
	}
	return false;
}

void write(Datum*r, Stream& stream, bool data = false)
{
	char s[80];
	const char* label = data ? "data" : "info";
	sprintf(s,"%s %d %d %d %d %06d %d\n",
			label,
			r->header.id,
			r->header.channel,
			r->header.length,
			r->header.event,
			r->header.time,
			r->header.CRC);
	stream.writestr(s);
	if (data)
	{
		stream.write(r->data, r->length);
		stream.writestr("\n");
	}
}

unsigned Database::QueryText(Stream& out, unsigned channel, unsigned after, unsigned event)
{
	Datum * r = Data();
	unsigned last = r ? r->header.id : 0;
	while (r)
	{
		if (after && r->header.id < after) break;
	  if (event && event != r->header.event) continue;
		if ((channel == 0 || r->header.channel == channel))
		{
			out << r->header.id << "\t" << writeTime(r->header.time) << "\t"<< r->header.event << "\t"<< Name(r->header.channel) << "\t" ;
			out.write (r->data,r->length);
			out.write("\n",1);

		}
		r = r->next;
	}
	return last;
}

/**
Returns all records before a given id number. The events table is in reverse order by id number.
Returns 1 + the id of the last record that was queried. Repeated calls will retrieve data as they are added,
though not in a consistent order.

 s = 0
 while (1)
 {
	 s = Query(out, channel, s, verbose)
	 sleep(...)
 }
*/
unsigned Database::Query(Stream& out, unsigned channel, unsigned after, bool verbose)
{
	Datum * r = Data(); //locks the database and returns a snapshot
	unsigned last = r ? r->header.id : 0;
	while (r)
	{
		if (after && r->header.id < after) break;
		if ((channel == 0 || r->header.channel == channel))
		{
			write(r, out, verbose);
		}
		r = r->next;
	}
	return last;
}

///Return the datum for this channel at or prior to the specified id, so you can build a snapshot based on a specific event.
Datum* Database::Snapshot(unsigned channel, unsigned id)
{
	if (!channel) return 0;
	Datum* r = Data();
	while (r)
	{
		if (r->header.channel == channel && (r->header.id <= id || id == 0))
		{
			return r;
		}
		r = r->next;
	}
	return 0;
}

///Return the datum for this channel at or after the specified id.
Datum* Database::Replay(unsigned channel, unsigned id)
{
	Datum* r, *d=0;
	r = Data();
	if (!channel) return 0;
	while (r)
	{
		if (r->header.channel == channel)
		{
			if (r->header.id < id)
				return d;
			d = r;
		}
		r = r->next;
	}
	return d;
}


Datum* Database::Data(unsigned id, unsigned channel)
{
	Datum* r;
//	if (!id)
//		return items;
	EnterCriticalSection(&lock);
	r = items;
	LeaveCriticalSection(&lock);
	if (!id && !channel) return r;
	while (r)
	{
		if ((!id || r->header.id == id) && (!channel || r->header.channel == channel))
		{
			return r;
		}
		r = r->next;
	}
	return 0;
}

const char* Database::Name(unsigned channel)
{
	const char* n;
	if (!channel) return 0;
	EnterCriticalSection(&lock);
		n = channels[channel-1];
	LeaveCriticalSection(&lock);
	return n;
}
size_t Database::AddChannel(const char* name,bool save)
{
	if (!canWrite) return false;
	size_t num;
	EnterCriticalSection(&lock);
	if (channels.Find(name) != NOT_FOUND)
		return false;

	num = channels.Add(name);
	LeaveCriticalSection(&lock);
if (DEBUG) printf("channel %d %s\n",num+1,name);
	if (save)
	{
		TStr index(directory,"channels.txt");
		FileStream f(index,Stream::OMBinary,Stream::IOAppend);
		f.writestr(name,"\n");
	}
	return num + 1;
}

size_t Database::Channel(const char* name, bool canAdd)
{
	size_t num;
	if (!*name) return 0;

	//if (*name >= '0' && *name <= '9')
	//  return atoi(name);

	EnterCriticalSection(&lock);
	num = channels.Find(name); //channel numbers start with 1
	LeaveCriticalSection(&lock);
	if (num == NOT_FOUND && canAdd)
	  return AddChannel(name);
	return num == NOT_FOUND ? 0 : num + 1;
}

/**
Print the snapshot plus detail from certain channels

If marker is set, a snapshot is printed each time the marker channel is encountered

If before is set, the history starts at that point

If all is set, the entire database is dumped

If detail is set, those fields are always printed
*/
void Database::Summary(Stream& out, TStringList& detail, unsigned before, bool all, unsigned marker)
{
	size_t * ch;
	size_t count = channels.Count();
	size_t channelCount = count;
	Datum * r;
	EnterCriticalSection(&lock);
	r = items;
	LeaveCriticalSection(&lock);

	ch = new size_t[count]; //0 = print, 1 = skip, 2 = print all
	memset(ch,0,sizeof(size_t)*count);

	size_t i,j;

	for (i=0; i< detail.Count(); i++)
	{
		j = Channel(detail[i],false);
		if (!j) continue;
		j--;
		ch[j] = 2;
	}
	out << "id\ttime\tchannel\tvalue\n";

	while (r && count)
	{
		if (!all && marker && r->header.channel == marker)
		{
		 for (j=0; j<channelCount; j++)
		   if (ch[j] == 1) ch[j] = 0; //reset snapshot marker
		}

		if (before == 0 || r->header.id <= before)
		{
			j = r->header.channel - 1;
			if (all || ch[j] != 1)
			{

				if (!all && ch[j] != 2) {if (!marker) count--; ch[j] = 1;}
				out << r->header.id << "\t" << writeTime(r->header.time) << "\t" << Name(r->header.channel) << "\t"  ;
				out.write (r->data,r->length);
				out << "\n";
			}
		}
		r = r->next;
	}
	delete[] ch;
}

bool Database::Command(const char* cmd, Publisher& publish)
{
	TStr command(cmd);
//if (DEBUG) printf("command: %s\n",(char*)command);
	char* params = strchr(command,' ');
	if (params) *params++ = 0;

	if (command == "channel") //channel name
	{
		size_t num = Channel(params,true);
		*publish.stream << "ok " << num << "\n";
	}
	else if (command == "event") //event [timestamp_channel]     writes a timestamp with the ms since started and returns the current time
	{
		char now[32];
		GetDateTime(now, 0, 5, true);
		char ms[64];
		unsigned t = (GetTickCount() - startTime);
		sprintf(ms,"%d",t);
		unsigned long evt = nextEvent++;
		*publish.stream << "ok " << evt << " " << ms << " " << now << "\n";  //event_number ms_counter clock_time
		if (params) //have a channel name or number to mark the event with
		{
			strcat(ms," ");
			strcat(ms,now);
			Insert(Channel(params,true),ms,evt,t);
		}
	}
	else if (command == "time") //gets the current time without logging to the database
	{
		char now[32];
		GetDateTime(now, 0, 5, true);
		char ms[64];
		unsigned t = (GetTickCount() - startTime);
		sprintf(ms,"%d",t);
		*publish.stream << "ok " << ms << " " << now << "\n";  //event_number ms_counter clock_time
	}
	else if (command == "start") //gets the start time without logging to the database
	{
		*publish.stream << "ok " << startDateTime << "\n";  //event_number ms_counter clock_time
	}
	else if (command == "insert") //insert channel length event time CRC\n[data]
	{
		Header h;
		int hasCRC = 1;

		if (params)
		{
			//channel is a text string or a number
			char * a = strchr(params,' ');
			if (a) *a++ = 0;
			h.channel = Channel(params,true);
			params = a;

			//length can be zero for \n terminated up to 255 chars
			if (params)
			{
				h.length = atoi(params);
				params = strchr(params,' ');
			} else h.length = 0;

			//event
			if (params)
			{
				h.event = atoi(++params);
				params = strchr(params,' ');
			} else h.event = 0;


			//time
			if (params)
			{
				h.time = atoi(++params);
				params = strchr(params,' ');
			}
			else
			{
				//char now[32];
				//GetDateTime(0, now, 0, false);
				//h.time = atoi(now); //current time in HHMMSS
				h.time = GetTickCount() - startTime; //ms since database started
			}

			//CRC
			if (params)
			{
				h.CRC = atoi(++params);
			} else hasCRC = h.CRC = 0;
		}

		if (Insert(h, *publish. stream, !hasCRC))
			*publish.stream << "ok " << h.id << "\n";
		else
			*publish.stream << "error\n";

	}
	return true;
}

bool Database::Command(const char* cmd, Subscriber& sub)
{
	TStr command(cmd);
	char* a = strchr(command,' '); //GET /filename HTTP/1.1
	if (a) *a++ = 0;

	char* b = 0;
	if (a) b = strchr(a,' ');
	if (b) *b = 0;

//if (DEBUG) printf("command: %s, file: %s\n",(char*)command,a);

	if (!strcasecmp(command,"GET")) //http
	{
//Result sets look like tab-delimited tables. MIME type: text/tab-separated-values
//channel\tid\ttime\tvalue,
		FileStream console("stdout",Stream::OMText,Stream::WriteOnly);
		console << command << " " << (a?a:"-") << " " << (b?b:"-") << "\n";

		char h[128];
		time_t rawtime;
		time ( &rawtime );
		strftime ( h, sizeof(h), "%a, %d %b %Y %H:%M:%S GMT", gmtime ( &rawtime ) );

		*sub.stream << "HTTP/1.1 200 OK\r\n";
		*sub.stream << "Date: " << h << "\r\n";
		*sub.stream << "Expires: 0\r\n";
		*sub.stream << "Server: RTDB/1\r\n";
		*sub.stream << "Connection: close\r\n";

		const char* file = "rtdb.html";
		const char* type = "text/html; charset=utf-8";
		TPointer<Stream> send(new MemoryStream);

		if (a && *a == '/') a++;
		TStr pageName(a);
		char* query = strchr(pageName,'?');
		if (query) *query++ = 0;

		if (!a || !*a)
		{
sendfile:
			send = new FileStream(file,Stream::OMBinary,Stream::ReadOnly);
		}
		else if (FileExists(pageName))
		{
			file = pageName;
			if (stristr(file,".js")) type = "text/javascript";
			goto sendfile;
		}
		else if (!strncmp(a,"snapshot",8))
		{
			type = "text/tab-separated-values";
			unsigned id = query ? atoi(query) : 0;
			*send << "id\tchannel\ttime\tevent\tvalue\n";
			for (size_t i=0; i< channels.Count(); i++)
			{
				Datum* r = Snapshot(i+1,id);
				if (r)
				{
					*send << r->header.id << "\t" << Name(r->header.channel) << "\t" << writeTime(r->header.time) << "\t" << r->header.event << "\t";
					send->write (r->data,r->length);
					*send << "\n";
				}
			}
		}
		else if (!strncmp(a,"replay",6))
		{
			type = "text/tab-separated-values";
			unsigned id = query ? atoi(query) : 0;

			*send << "id\ttime\tevent\tchannel\tvalue\n";
			for (size_t i=0; i< channels.Count(); i++)
			{
				Datum* r = Replay(i+1,id);
				if (r)
				{
					*send << r->header.id << "\t" << writeTime(r->header.time) << "\t"<< r->header.event << "\t"<< Name(r->header.channel) << "\t" ;
					send->write (r->data,r->length);
					*send << "\n";
				}
			}
		}
		else if (!strncmp(a,"nextid",6)) //fetch the current id
		{
			type = "text/plain";
			*send << nextId;
		}
		else if (!strncmp(a,"about",5))
		{
			type = "text/plain";
			*send << directory;
		}
		else if (!strncmp(a,"nextevent",9)) //fetch the current event
		{
			type = "text/plain";
			*send << nextEvent;
		}
		else if (!strncmp(a,"digest",6))
		{
			type = "text/plain";
			unsigned channel = query ? Channel(query) : 0;
			//if (DEBUG) printf("a: %s, %d, channel: %d\n",a+7,a[6] == '?',channel);

			Query(*send, channel, 0, false);
		}
		else if (!strncmp(a,"history",7))
		{
			type = "text/tab-separated-values";
			char* seq = strchr(query,',');
			if (seq) *seq++ = 0;
			unsigned channel = query ? Channel(query) : 0;

			*send << "id\ttime\tevent\tchannel\tvalue\n";
			QueryText(*send, channel, seq?atoi(seq):0,0);
		}
		else if (!strncmp(a,"event",5))
		{
			type = "text/tab-separated-values";
			char* seq = strchr(query,',');
			if (seq) *seq++ = 0;
			unsigned channel = query ? Channel(query) : 0;

			*send << "id\ttime\tevent\tchannel\tvalue\n";
			QueryText(*send, channel, 0, seq?atoi(seq):0);
		}
		else if (!strncmp(a,"time",4))
		{
			type = "text/plain";
			char now[32];
			GetDateTime(now, 0, 5, true);
			char ms[64];
			unsigned t = (GetTickCount() - startTime);
			sprintf(ms,"%d",t);

			*send << ms << " " << now << "\n";
		}
		else if (!strncmp(a,"start",5))
		{
			type = "text/plain";

			*send << startDateTime << "\n";
		}
		else if (!strncmp(a,"select",6))
		{
			type = "text/plain";
			char* seq = strchr(query,',');
			if (seq) *seq++ = 0;
			unsigned channel = query ? Channel(query) : 0;
			//if (DEBUG) printf("a: %s, %d, channel: %d\n",a+7,a[6] == '?',channel);

			Query(*send, channel, seq?atoi(seq):0, true);
		}
		else if (!strncmp(a,"last",4))
		{
			type = "text/plain";
			Datum*r = Data(0);
			if (r)
			{
				*send << r->header.id;
			}
		}
		else if (!strncmp(a,"channels",8))
		{
			type = "text/plain";
			unsigned channel = 1;
			const char* c = Name(channel);
			while (c)
			{
				*send << c << "\n";
				c = Name(++channel);
			}
		}
		else if (!strncmp(a,"fetch",5))
		{
			type = "text/tab-separated-values";
			unsigned id = query ? atoi(query) : 0;
			Datum*r = Data(id);
			if (r)
			{
				*send << "id\ttime\tevent\tchannel\tvalue\n";
				*send << r->header.id << "\t" << writeTime(r->header.time) << "\t" << r->header.event << "\t"<< Name(r->header.channel) << "\t" ;
				send->write (r->data,r->length);
				*send << "\n";
			}
		}
		else // channel
		{
			type = "text/tab-separated-values";
			unsigned channel = a ? Channel(a) : 0;
			if (channel)
			{
				Datum* r = Snapshot(channel);
				if (r)
				{
					*send << Name(r->header.channel) << ":";
					send->write (r->data,r->length);
				}
			}
		}
//printf("length %d type %s\r\n",send->size(),type);
		*sub.stream << "Content-Length: " << send->size() << "\r\n";
		*sub.stream << "Content-Type: " << type << "\r\n\r\n";

		send->rewind();
		sub.stream->Append(*send);

		return false; //close immediately
	}
	else if (cmd == "channel") //channel name
	{
		unsigned channel = a ? Channel(a) : 0;
		if (channel)
		  *sub.stream << "ok " << channel << "\n";
		else
		  *sub.stream << "error \'" << (a?a:"") << "\' not found\n";
	}
	else if (command == "verbose")
	{
		sub.verbose = a ? (atoi(a) != 0) : true;
	}
	else if (command == "listen")
	{
		unsigned channel = a ? Channel(a,true) : 0;

		sub.lastId = nextId;

		if (!Has(sub.channels, channel))
		{
			bool done = false;
			for (size_t i=0; i<sub.channels.count; i++)
			{
				if (sub.channels[i] == 0xffffffff)
				{
					sub.channels[i] = channel;
					done = true;
				}
			}
			if (!done) sub.channels.Add(channel);
		}
	}
	else if (command == "stop")
	{
		unsigned channel = a ? Channel(a) : 0;

		for (size_t i=0; i<sub.channels.count; i++)
		{
			if (channel == 0 || sub.channels[i] == channel) sub.channels[i] = 0xffffffff;
		}
		sub.stream->writestr("ok\n");
	}
	else if (command == "snapshot") //show a current snapshot
	{
		unsigned id = a ? atoi(a) : 0;
		for (size_t i=0; i< channels.Count(); i++)
		{
			Datum* r = Snapshot(i+1,id);
			if (r)
			{
				write(r,*sub.stream,true);
			}
		}
	    sub.stream->writestr("ok\n");
	}
	else if (command == "replay") //show a current snapshot
	{
		unsigned id = a ? atoi(a) : 0;
		for (size_t i=0; i< channels.Count(); i++)
		{
			Datum* r = Replay(i+1,id);
			if (r)
			{
				write(r,*sub.stream,true);
			}
		}
	    sub.stream->writestr("ok\n");
	}
	else if (command == "digest") //select channel [id]
	{
		unsigned channel = a ? Channel(a) : 0;
		unsigned start = b ? atoi(b) : 0;

	    Query(*sub.stream, channel, start, false);
	    sub.stream->writestr("ok\n");
	}
	else if (command == "select") //select channel [id]
	{
		unsigned channel = a ? Channel(a) : 0;
		unsigned start = b ? atoi(b) : 0;

	    Query(*sub.stream, channel, start, true);
	    sub.stream->writestr("ok\n");
	}
	else if (command == "fetch") //fetch id
	{
		unsigned id = a ? atoi(a) : 0;

		Datum* r = Data(id);
		if (r)
		{
			write(r,*sub.stream, true);
		}
		else
		{
			*sub.stream << "error \'" << (a?a:"") << "\' not found\n" ;
		}
	}
	else if (command == "channels")
	{
		for (size_t i=0; i<channels.Count(); i++)
		{
			if (i) *sub.stream << " ";
			*sub.stream << channels[i];
		}
		*sub.stream << "\n";
	}
	else if (command == "info")
	{
		unsigned id = a ? atoi(a) : 0;
		Datum* r = Data(id);
		if (r)
		{
			write(r,*sub.stream, false);
		}
		else
		{
			*sub.stream << "error '" << id << "' not found\n" ;
		}
	}
	else
	{
//if (DEBUG) printf("Unknown: %s\n",(char*)command);
		*sub.stream << "error " << command << "\n" ;
	}
	return true;

}

void Summarize(Stream& console, const char* name, TStringList& fields, const char* marker, unsigned before=0, unsigned all = false)
{
	if (FileExists(TStr(name,"\\channels.txt")) && FileExists(TStr(name,"\\00000001.dat")) )
	{
		console << "Database: " << name << "\n";
		Database d(name, false);
		d.Summary(console, fields, before, all, marker ? d.Channel(marker) : 0);
		console << "\n";
	}
}

int main(int argc, char** argv)
{
	GetDateTime(startDateTime, 0, 5, false); //filenames are in local time
	char* dir = startDateTime;
	const char* searchFields = "";
	const char* markerChannel = 0;

    int pport = 8060;
    int sport = 8040;
    unsigned before = 0;
    bool writeable = true;

    bool openBrowser = true;

	while (argc > 1)
	{
		if (argc > 1 && !strcmp(argv[1],"-help"))
		{
			puts("Usage: rtdb [-readonly] [-help] [-stop] [-nogui] [-publish port] [-subscribe port] [-marker channel] [-fields note,...] [-summary [2009*]] [directory]");
			return 1;
		}
		else if (argc > 2 && !strcmp(argv[1],"-fields"))
		{
			searchFields = argv[2];
			argc--;
			argv++;
		}
		else if (!strcmp(argv[1],"-all"))
		{
			searchFields = "*";
			argc--;
			argv++;
		}
		else if (argc > 2 && !strcmp(argv[1],"-marker"))
		{
			markerChannel = argv[2];
			argc--;
			argv++;
		}
		else if (argc > 2 && !strcmp(argv[1],"-before"))
		{
			before = atoi(argv[2]);
			argc--;
			argv++;
		}
		else if (!strcmp(argv[1],"-summary")) // -fields column1,column2,... -before # -summary database_name -- print database snapshots, and optionally history
		{
			FileStream console("stdout",Stream::OMText,Stream::WriteOnly);
			TStringList fields(searchFields);
			while (argc > 2)
			{
				Summarize(console,argv[2], fields, markerChannel, before, fields.Count() && fields[0][0] == '*');
				argc--;
				argv++;
			}
			return 0;
		}
		else if (!strcmp(argv[1],"-stop"))
		{
			InternetStream s("127.0.0.1",pport);
			s.write("quit\n",5);
			return 0;
		}
		else if (!strcmp(argv[1],"-readonly"))
		{
			writeable = false;
			pport = 0;
		}
		else if (argc > 2 && !strcmp(argv[1],"-subscribe"))
		{
			sport = atoi(argv[2]);
			argc--;
			argv++;
		}
		else if (argc > 2 && !strcmp(argv[1],"-publish"))
		{
			pport = atoi(argv[2]);
			argc--;
			argv++;
		}
		else if (!strcmp(argv[1],"-nogui"))
		{
			openBrowser = false;
		}
		else if (argv[1][0] != '-')
		{
			dir = argv[1];
			char* fn = strstr(dir,"00000001.dat"); //reading a file
			if (fn) //double-click on a data file to browse contents
			{
				*fn =0;
				pport = 0;
				sport = 0;
				writeable = false;
			}
		}

		argc--;
		argv++;
	}


	if (pport)
	{
		try{
			InternetStream s("127.0.0.1",pport);
			s.writestr("info\n");
			TStr t;
			if (s.readline(t))
			{
			 printf("RTDB is already running in %s\n",(char*)t);
			 return 2;
			}
	   }
	   catch(...)
	   {
	   }
	}

try{
	FileStream console("stdout",Stream::OMText,Stream::WriteOnly);//stdout is automatically unbuffered
	bool running = true;
	bool any;

	size_t i,j;


	TList<Publisher> publishers;
	TList<Subscriber> subscribers;
	TPointer<Database> db(new Database(dir,writeable));

	console << "Database: " << dir << "\n";

	InternetServer sub(sport);
	TStr browserAddress("http://",sub.hostinfo,"/");
	console << "Subscribe: " << browserAddress << "\n";

	TPointer<InternetServer> pub;
	if (pport)
	{
		pub = new InternetServer(pport);
		console << "Publish: http://" << pub->hostinfo << "/\n\n";
	}


#ifdef XP_WIN

	if (openBrowser) ShellExecute(NULL,NULL,browserAddress,NULL,NULL,SW_SHOW);
#else
	if (openBrowser)
	{
		TStr cmd("firefox ",browserAddress," &");
		system(cmd);
	}
#endif

	while(running)
	{
		any = false;

		//printf("1\r");
		//check for new publishers
		if (pub) if (pub->AnyoneWaiting())
		{
			InternetStream * p = pub->Accept();
			if (p)
			{
				publishers.Add(new Publisher(p));
				any = true;
			}
			//continue;
		}

		//printf("2\r");
		//accept new data submissions
		j = publishers.Count();
		for(i = 1; i <= j; i++)
		{
			Publisher* p = publishers[j-i];
			int timeout = 128; //limit 128 submissions per channel
			bool anycmds = false;

			while (timeout-- && p->read())
			{
				//printf("3\r");

//if (DEBUG) printf("publish %d: %s\n",j-i,(char*)p->command);
				if (!strcasecmp(p->command,"quit"))
				{
					running = false;
					break;
				}
				if (!strcasecmp(p->command,"info"))
				{
					TStr response(dir,"\t",sub.hostinfo);
					p->stream->sendln(response);
					break;
				}
				if (!strcasecmp(p->command,"close") || !db->Command(p->command,*p))
				{
					publishers.Destroy(j-i);
					break;
				}
				p->reset();
				any = anycmds = true;
			}
			if (!anycmds && !p->stream->canread() && !p->stream->canwrite())
			{
//if (DEBUG) printf("%d %d %d %d %d\n",(int)anycmds, (int)p->stream->canread(),(int)p->stream->canwrite(), (int)p->stream->eof(), (int)p->stream->Type) ;
//if (DEBUG) printf("closing publisher %d\n",j-i);
				publishers.Destroy(j-i);
				continue;
			}
		}

		//printf("4\r");
		if (sub.AnyoneWaiting())
		{
			Subscriber* s = new Subscriber(sub.Accept());
			Sleep(10);
			any = true;
			if (s->read())
			{
				//printf("subscribe %s\r\n",(char*)s->command);
				if (!strncasecmp(s->command,"GET /quit",9))
				{
					running = false;
					delete s;
					continue;
				}
				if (!strncasecmp(s->command,"GET ",4) && !db->Command(s->command,*s))
				{
					Sleep(1);
					delete s;
					continue;
				}
			}
			subscribers.Add(s);
			//continue;
		}

		//printf("5\r");
		j = subscribers.Count();
		for(i = 1; i <= j; i++)
		{
			//printf("6\r");
			Subscriber* s = subscribers[j - i];
			if (!s->stream->canwrite())
			{
//if (DEBUG) printf("closing subscriber %d\n",j-i);
				subscribers.Destroy(j-i);
				continue;
			}
			else if (s->read())
			{
//printf("subscribe %d: %s\n",j-i,(char*)s->command);
				if (!strcasecmp(s->command,"quit") || !strncasecmp(s->command,"GET /quit",9))
				{
					running = false;
					break;
				}
				else if (!strcasecmp(s->command,"close") || !db->Command(s->command,*s))
				{
					any = true;
					subscribers.Destroy(j-i);
					continue;
				}
				any = true;
				s->reset();
			}
		}

		//printf("7\r");
		//subscriptions
		j = subscribers.Count();
		for(i = 0; i< j; i++)
		{
			//printf("8\r");
			size_t k;
			Subscriber* s = subscribers[i];
			for (k=0; k < s->channels.count; k++)
			{
				unsigned ch = s->channels[k];
				if (ch == 0xffffffff) continue;
				s->lastId = db->Query(*s->stream, ch, s->lastId, s->verbose);
			}
		}
		//printf("9\r");
		if (!any)
		{
			//printf("|\r");
			Sleep(15);
			//printf("-\r");
		}
	}
	}
	catch(xdb& x)
	{
		printf("error: %s\n",(char*)x.why(), (char*)x.info());
	}
	return 0;
}


#if 0
/*
#ifdef XP_WIN
	WIN32_FIND_DATA FindData;
	int skip = FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_OFFLINE;
	HANDLE finder = FindFirstFile(fspec,&FindData);

	if (finder != INVALID_HANDLE_VALUE)
	{
		do
		{
		if (FindData.cFileName[0] != '.'
			&& (FindData.dwFileAttributes & skip) == 0
			&& FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
			&& FileExists(TStr(FindData.cFileName,"\\channels.txt"))
			&& FileExists(TStr(FindData.cFileName,"\\00000001.dat"))
		   )
		   {
			   	const char* name = FindData.cFileName;
#else
	DIR *ff = opendir(dirname);
	if (ff)
	{
	 	struct dirent *dr;
		do
		{
			dr = readdir(ff);
			if (dr)
 			if (fnmatch(search,dr->d_name,0) == 0 && dr->d_name[0] != '.')
  			if (FileExists(TStr(FindData.cFileName,"/channels.txt"))
			if (FileExists(TStr(FindData.cFileName,"/00000001.dat"))
			{
  				name = dr->d_name;
#endif
				console << "Database: " << name << "\n";
				Database d(name, false);
				d.Summary(console, fields);
				console << "\n";

#ifdef XP_WIN
			}
		} while (FindNextFile(finder,&FindData));
		FindClose(finder);
	}
#else
			}
		} while (dr!=NULL);

		closedir(ff);
	}
#endif
*/
#endif
