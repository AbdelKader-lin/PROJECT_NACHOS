// post.h 
//	Data structures for providing the abstraction of unreliable,
//	ordered, fixed-size message delivery to mailboxes on other 
//	(directly connected) machines.  Messages can be dropped by
//	the network, but they are never corrupted.
//
// 	The US Post Office delivers mail to the addressed mailbox. 
// 	By analogy, our post office delivers packets to a specific buffer 
// 	(MailBox), based on the mailbox number stored in the packet header.
// 	Mail waits in the box until a thread asks for it; if the mailbox
//      is empty, threads can wait for mail to arrive in it. 
//
// 	Thus, the service our post office provides is to de-multiplex 
// 	incoming packets, delivering them to the appropriate thread.
//
//      With each message, you get a return address, which consists of a "from
// 	address", which is the id of the machine that sent the message, and
// 	a "from box", which is the number of a mailbox on the sending machine 
//	to which you can send an acknowledgement, if your protocol requires 
//	this.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef TCPLITE_H
#define TCPLITE_H

#include "network.h"
#include "thread.h"
#include "synchlist.h"
#include "post.h"

#define MAXREEMISSIONS 3
#define ACKTIMEOUT 1000	// in ticks
#define TCP_BOX 2	// mailbox number for TCP messages
#define MaxTcpLiteDataSize (MaxMailSize - sizeof(struct TcpLiteHeader))
#define MaxTcpLiteMessageSize (MaxTcpLiteDataSize * 50)

typedef struct TcpLiteHeader {
    int seq;        // sequence number
    int ack;        // acknowledged seq
    bool isAck;     // ACK or DATA
    unsigned int length;     // length of data

    int  msgId;      // message identifier
    bool isFirst;    // first fragment
    bool isLast;     // last fragment

} TcpLiteHeader;

typedef struct MsgKey {
    int sender;
    int msgId;
} MsgKey;

typedef struct ReassemblyEntry {
    int sender;
    int msgId;
    char* data;      // growing byte buffer
    int totalLength;
} ReassemblyEntry;

bool MatchMsg(void* item, void* key);

class TcpLitePacket {
public:
    PacketHeader pktHdr;
    MailHeader mailHdr;
    TcpLiteHeader tcpHdr;
    char data[MaxTcpLiteMessageSize];

    TcpLitePacket(PacketHeader& p, MailHeader& m, TcpLiteHeader& t, char* buf) {
        tcpHdr = t;
        pktHdr = p;
        mailHdr = m;
        bcopy(buf, data, t.length);
    }
};

class TcpMessageBuffer {
public:
    TcpMessageBuffer();
    ~TcpMessageBuffer();

    void Put(PacketHeader pkt, MailHeader mail, TcpLiteHeader h, char* data);
    void Get(PacketHeader* pkt, MailHeader* mail, TcpLiteHeader* h, char* data);
private:
    SynchList* buffer;
    SynchList* partialMessages;
};

// The following class defines part of the message header.  
// This is prepended to the message by the PostOffice, before the message 
// is sent to the Network.

class TcpLite {
  public:
    TcpLite(PostOffice* po);
				// Allocate and initialize Post Office
				//   "reliability" is how many packets
				//   get dropped by the underlying network
    ~TcpLite();		// De-allocate Post Office data
    
    int Send(PacketHeader pktHdr, MailHeader mailHdr, TcpLiteHeader tcpHdr, const char *data);
    int SendChunk(PacketHeader pktHdr, MailHeader mailHdr, TcpLiteHeader h, const char *data);
    				// Send a message to a mailbox on a remote 
				// machine.  The fromBox in the MailHeader is 
				// the return box for ack's.
    
	void ReceiverThread();
	void SendAck(PacketHeader& pkt,
                 MailHeader& mail,
                 int seq);
	void OnReceive(PacketHeader& pkt,
                   MailHeader& mail,
                   TcpLiteHeader* h,
                   char* data);
	int Receive(PacketHeader* pkt, MailHeader* mail, TcpLiteHeader* tcpHdr, char* appBuffer);

	void HandleTimeout(int seq);
  private:

    SynchList *recvBuffer; // buffer for incoming messages
    PostOffice *postOffice; // To support higher-level functions
	int seqCounter;
    int expectedSeq;
    int msgCounter;
	bool waitingAck;
	Semaphore *ackSem;
	int retransmissions;
    TcpMessageBuffer* messageBuffer;
    int mailbox;
};

typedef struct TimeoutParams {
    TcpLite* tcp;
    int seq;
} TimeoutParams;

#endif
