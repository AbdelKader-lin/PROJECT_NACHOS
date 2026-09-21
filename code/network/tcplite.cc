// post.cc 
// 	Routines to deliver incoming network messages to the correct
//	"address" -- a mailbox, or a holding area for incoming messages.
//	This module operates just like the US postal service (in other
//	words, it works, but it's slow, and you can't really be sure if
//	your mail really got through!).
//
//	Note that once we prepend the MailHdr to the outgoing message data,
//	the combination (MailHdr plus data) looks like "data" to the Network 
//	device.
//
// 	The implementation synchronizes incoming messages with threads
//	waiting for those messages.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "tcplite.h"
#include "system.h"

#include <strings.h> /* for bzero */
bool MatchMsg(void* item, void* key)
{
    ReassemblyEntry* e = (ReassemblyEntry*) item;
    MsgKey* k = (MsgKey*) key;

    return (e->sender == k->sender &&
            e->msgId  == k->msgId);
}

static void TimeoutHandler(int arg)
{     
    TimeoutParams* params = (TimeoutParams *) arg;
    params->tcp->HandleTimeout(params->seq);
}

static void ReceiverThreadHelper(int arg) {
    TcpLite* tcp1234 = (TcpLite *) arg;
    tcp1234->ReceiverThread();
}

TcpMessageBuffer::TcpMessageBuffer() {
    buffer = new SynchList();
    partialMessages = new SynchList();
}
TcpMessageBuffer::~TcpMessageBuffer() {
    delete buffer;
}
void TcpMessageBuffer::Put(PacketHeader pkt, MailHeader mail, TcpLiteHeader tcpHdr, char* data) {
    ReassemblyEntry* entry = nullptr;
    if (!tcpHdr.isFirst) {
        MsgKey key;
        key.sender = pkt.from;
        key.msgId = tcpHdr.msgId;
        entry = (ReassemblyEntry*) partialMessages->Find((void*)&key, MatchMsg);
        ASSERT(entry != nullptr); // must exist
    } else {
        ASSERT(entry == nullptr); // should not exist
    }
    if (!tcpHdr.isLast) {
        if (tcpHdr.isFirst) {
            entry = new ReassemblyEntry();
            entry->sender = pkt.from;
            entry->msgId = tcpHdr.msgId;
            entry->data = new char[MaxTcpLiteMessageSize]; // allocate large buffer
            bzero(entry->data, MaxTcpLiteMessageSize);
            partialMessages->Append((void*)entry);  
        } 
        bcopy(data, entry->data + entry->totalLength, tcpHdr.length);
        entry->totalLength += tcpHdr.length;
    } else {
        if (tcpHdr.isFirst) {
            // Single fragment message
            TcpLitePacket* tcpPkt = new TcpLitePacket(pkt, mail, tcpHdr, data);
            buffer->Append((void*)tcpPkt);
            return;
        } else {
            bcopy(data, entry->data + entry->totalLength, tcpHdr.length);
            entry->totalLength += tcpHdr.length;
            tcpHdr.length = entry->totalLength;
            TcpLitePacket* tcpPkt = new TcpLitePacket(pkt, mail, tcpHdr, entry->data);
            buffer->Append((void*)tcpPkt);

            partialMessages->RemoveItem((void*)entry);
            delete[] entry->data;
            delete entry;

        }
    }
}

void TcpMessageBuffer::Get(PacketHeader* pkt, MailHeader* mail, TcpLiteHeader* tcpHdr, char* data) {
    DEBUG('n', "Waiting for TCP message in buffer\n");
    TcpLitePacket* tcpPkt = (TcpLitePacket*) buffer->Remove();
    *pkt = tcpPkt->pktHdr;
    *mail = tcpPkt->mailHdr;
    *tcpHdr = tcpPkt->tcpHdr;
    if (DebugIsEnabled('n')) {
        printf("Got TCP message from buffer: seq=%d, ack=%d, isAck=%d\n", tcpHdr->seq, tcpHdr->ack, tcpHdr->isAck);
    }
    bcopy(tcpPkt->data, data, tcpPkt->tcpHdr.length);
    delete tcpPkt;
}

TcpLite::TcpLite(PostOffice* po)
{
    recvBuffer = new SynchList();
    messageBuffer = new TcpMessageBuffer(); 
    postOffice = po;
    seqCounter = 0;
    expectedSeq = 0;
    msgCounter = 0;
    waitingAck = false;
    ackSem = new Semaphore("tcp ack", 0);
    retransmissions = 0;
    mailbox = TCP_BOX;
    Thread* receiver = new Thread("tcp receiver");
    receiver->Fork(ReceiverThreadHelper, (int)this);
}

TcpLite::~TcpLite()
{
}

//----------------------------------------------------------------------
// PostOffice::PostalDelivery
// 	Wait for incoming messages, and put them in the right mailbox.
//
//      Incoming messages have had the PacketHeader stripped off,
//	but the MailHeader is still tacked on the front of the data.
//----------------------------------------------------------------------

void 
TcpLite::ReceiverThread() {
    PacketHeader pktHdr;
    MailHeader mailHdr;
    char *buffer = new char[MaxPacketSize];
    bzero(buffer, MaxPacketSize);

    while (true) {
        postOffice->Receive(mailbox, &pktHdr, &mailHdr, buffer);
        TcpLiteHeader* tcpHdr = new TcpLiteHeader();
        bcopy(buffer, tcpHdr, sizeof(TcpLiteHeader));
        OnReceive(pktHdr, mailHdr, tcpHdr, buffer + sizeof(TcpLiteHeader));
    }
}

void
TcpLite::OnReceive(PacketHeader& pkt,
                   MailHeader& mail,
                   TcpLiteHeader* tcpHdr,
                   char* data)
{
    // if (tcpHdr->isAck) {
    //     printf("Received ACK for seq=%d\n", tcpHdr->ack);
    // } else {
    //     printf("Received packet: seq=%d, isAck=%d, len=%d\n", tcpHdr->seq, tcpHdr->isAck, tcpHdr->length);
    // }
    if (tcpHdr->isAck && waitingAck && tcpHdr->ack == seqCounter) {
        waitingAck = false;
        ackSem->V();    // wake Send()
        return;
    }

    if (!tcpHdr->isAck) {
        SendAck(pkt, mail, tcpHdr->seq);
        if (tcpHdr->seq == expectedSeq) {
            ++expectedSeq;
            messageBuffer->Put(pkt, mail, *tcpHdr, data);
        }
    }
}

int TcpLite::SendChunk(PacketHeader pktHdr,
                   MailHeader mailHdr,
                   TcpLiteHeader tcpHdr,
                   const char* data)
{
    ASSERT(tcpHdr.length <= MaxTcpLiteDataSize);
    tcpHdr.seq = seqCounter;
    tcpHdr.isAck = false;

    char *buffer = new char[MaxMailSize];
    bzero(buffer, MaxMailSize);
    bcopy(&tcpHdr, buffer, sizeof(TcpLiteHeader));
    bcopy(data, buffer + sizeof(TcpLiteHeader), tcpHdr.length);
    mailHdr.length = tcpHdr.length + sizeof(TcpLiteHeader);
    waitingAck = true;

    TimeoutParams* params = new TimeoutParams();
    params->tcp = this;
    params->seq = seqCounter;
    interrupt->Schedule(TimeoutHandler, (int)params, ACKTIMEOUT, NetworkRecvInt);
    postOffice->Send(pktHdr, mailHdr, buffer);
    ackSem->P();   // wait for ACK
    if (waitingAck) {
        Delay(1); // wait a bit before retransmission
        if (retransmissions < MAXREEMISSIONS) {
            retransmissions++;
            return SendChunk(pktHdr, mailHdr, tcpHdr, data);
        } else {
            waitingAck = false;
            retransmissions = 0;
            printf("Max retransmissions reached, giving up on packet %d\n", seqCounter);
            return -1;
        }
    } else {
        retransmissions = 0;
        seqCounter++;
    }
    return 0;
}

int TcpLite::Send(PacketHeader pktHdr,
                   MailHeader mailHdr,
                   TcpLiteHeader tcpHdr,
                   const char* data)
{
    unsigned int totalLength = tcpHdr.length;
    ASSERT(totalLength <= MaxTcpLiteMessageSize);
    mailHdr.from = mailbox;
    mailHdr.to = mailbox;
    int isFirst = 1;
    int msgId = msgCounter++;

    for (unsigned int offset = 0; offset < totalLength; ) {
        tcpHdr.msgId = msgId;
        tcpHdr.isFirst = isFirst;
        isFirst = 0;
        char *chunkData = new char[MaxTcpLiteDataSize];
        bzero(chunkData, MaxTcpLiteDataSize);
        if (offset + MaxTcpLiteDataSize >= totalLength) {
            tcpHdr.isLast = true;
            tcpHdr.length = totalLength - offset;
            mailHdr.length = tcpHdr.length + sizeof(TcpLiteHeader);
        } else {
            tcpHdr.isLast = false;
            tcpHdr.length = MaxTcpLiteDataSize;
            mailHdr.length = MaxMailSize;
        }
        bcopy(data + offset, chunkData, tcpHdr.length);
        int res = SendChunk(pktHdr, mailHdr, tcpHdr, chunkData);
        if (res == -1) {
            printf("Failed to send chunk, aborting message send\n");
            return -1;
        }
        offset += tcpHdr.length;
    }
    return totalLength;
}
void TcpLite::SendAck(PacketHeader& pkt,
                        MailHeader& mail,
                        int seq)
{
    TcpLiteHeader *tcpHdr = new TcpLiteHeader();
    tcpHdr->seq = 0; // Not used in ACK
    tcpHdr->ack = seq;
    tcpHdr->isAck = true;

    char *buffer = new char[MaxMailSize];
    bzero(buffer, MaxMailSize);
    bcopy(tcpHdr, buffer, sizeof(TcpLiteHeader));
    MailHeader ackMail;
    ackMail.to = mail.from;
    ackMail.from = mail.to;
    ackMail.length = sizeof(TcpLiteHeader);

    PacketHeader ackPkt;
    ackPkt.to = pkt.from;
    ackPkt.from = pkt.to;
    // printf("Sending ACK for seq=%d\n", seq);
    postOffice->Send(ackPkt, ackMail, buffer);
}

int
TcpLite::Receive(PacketHeader* pkt, MailHeader* mail, TcpLiteHeader* tcpHdr, char* appBuffer)
{
    char* data = new char[MaxTcpLiteMessageSize];
    bzero(data, MaxTcpLiteMessageSize);

    messageBuffer->Get(pkt, mail, tcpHdr, data);
    bcopy(data, appBuffer, tcpHdr->length);
    return tcpHdr->length;
}

void TcpLite::HandleTimeout(int seq) {
    if (waitingAck && seq == seqCounter) {
        ackSem->V();   // unblock Send()
    }
}