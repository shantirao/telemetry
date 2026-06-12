# rtdb
Real-time database for monitoring changing laboratory equipment

The real-time database server maintains a wide table with many columns (channels) and a single row. Each transaction modifies one cell in that table, and is identified by a sequential ID#, starting with 1. A history of the system can be reconstructed by assembling the transactions in chronological order. You can also create a virtual table showing the history of a column with the select command.

Transactions are stored in memory in reverse-chronological order. If you want to query the database, you usually
askfor everything that's happened since your last query.

Publishers connect on socket 8060, subscribers on 8040.

Publisher commands:

  >channel name
  ok {number}

  >insert channel [length] [opaque] [time] [CRC]
  {data}

  Shut down the database

  >quit


Subscriber commands

  >channel name
  
  ok number
  error 'name' not found

  Returns the channel number for that name.

  >listen [channel name or number]

  Subscribes to changes on that channel. Will create a channel if one of that name doesn't already exist. When that channel changes, the server will send out-of-band "info" messages.

  >stop [channel name or number]
  
  ok

  Stop listening to a channel (or blank for all)

  >snapshot [age]
  data id channel length sequence time crc
  [bytes]
  ...
  ok

  Returns the last entry from each channel on or before age

  >select [channel name or number] [id]
  info id channel length sequence time crc
  ...
  ok

  Returns all updates to a channel after event id

  >select channel [start]
  info id length channel frame time crc

  >channels
  channel_1_name channel_2_name channel_3_name ...

  >info id

  >listen channel [start]

  >stop channel

  >info id
  info id channel length sequence time crc

  error 'id' not found

  >fetch id
  data id channel length sequence time crc
  {data}

  error 'id' not found

  >close

  >quit
