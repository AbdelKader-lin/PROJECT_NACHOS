// nettest.cc 
//	Test out message delivery between two "Nachos" machines,
//	using the Post Office to coordinate delivery.
//
//	Two caveats:
//	  1. Two copies of Nachos must be running, with machine ID's 0 and 1:
//		./nachos -m 0 -o 1 &
//		./nachos -m 1 -o 0 &
//
//	  2. You need an implementation of condition variables,
//	     which is *not* provided as part of the baseline threads 
//	     implementation.  The Post Office won't work without
//	     a correct implementation of condition variables.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"
#include "tcplite.h"

// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our 
//	    original message

#define RING_INITIATOR 0
#define RING_SIZE 3
void
MailTest(int farAddr)
{
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    const char *data = "Hello there!";
    const char *ack = "Got it!";
    char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = farAddr;		
    outMailHdr.to = 0;
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    postOffice->Send(outPktHdr, outMailHdr, data); 

    // Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(ack) + 1;
    postOffice->Send(outPktHdr, outMailHdr, ack); 

    // Wait for the ack from the other machine to the first message we sent.
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Then we're done!
    interrupt->Halt();
}

int MailTestRing(int dump) {
    ASSERT(postOffice->GetNetAddr() < RING_SIZE);
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    int farAddr = (postOffice->GetNetAddr() + 1) % RING_SIZE;
    const char *data = "RING TEST";
    const char *ack = "ACK";
    char buffer[MaxMailSize];
    if (postOffice->GetNetAddr() != RING_INITIATOR) {

        // Wait for the first message from the other machine
        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
        printf("1Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from,
               inMailHdr.from);
        fflush(stdout);

        // Send acknowledgement to the other machine (using "reply to" mailbox
        // in the message that just arrived
        outPktHdr.to = inPktHdr.from;
        outMailHdr.to = inMailHdr.from;
        outMailHdr.length = strlen(ack) + 1;
        postOffice->Send(outPktHdr, outMailHdr, ack);
    }
    outPktHdr.to = farAddr;
    outMailHdr.to = 0;
    outMailHdr.from = 1;

    if (postOffice->GetNetAddr() == RING_INITIATOR) {
        outMailHdr.length = strlen(data) + 1;

        // Send the first message
        postOffice->Send(outPktHdr, outMailHdr, data);
    } else {
        outMailHdr.length = strlen(buffer) + 1;
        postOffice->Send(outPktHdr, outMailHdr, buffer);
    }
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("2Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from,
           inMailHdr.from);
    fflush(stdout);

    ASSERT(postOffice->GetNetAddr() + 1 == inPktHdr.from % RING_SIZE);

    if (postOffice->GetNetAddr() == RING_INITIATOR) {
        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
        printf("3Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from,
               inMailHdr.from);
        fflush(stdout);
        ASSERT(inPktHdr.from == (RING_SIZE - 1));

        outPktHdr.to = inPktHdr.from;
        outMailHdr.to = inMailHdr.from;
        outMailHdr.length = strlen(ack) + 1;
        postOffice->Send(outPktHdr, outMailHdr, ack);
    }

    // Then we're done!
    interrupt->Halt();
    return 0;
}

void
TcpTestSender(int farAddr)
{
    printf("Sender: sending message to %d\n", farAddr);
    PacketHeader pkt;
    MailHeader mail;
    TcpLiteHeader tcpHdr;
    char msg[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Lorem ipsum dolor sit amet, consectetur adipiscing elit.";

    // char msg[] = "Lorem ipsum dolor sit amet.";
    pkt.to = farAddr;      // machine 1

    tcpHdr.length = strlen(msg);

    tcp->Send(pkt, mail, tcpHdr, msg);

    printf("Sender: message sent reliably\n");
}

void
TcpTestReceiver()
{
    printf("Receiver: waiting for message\n");
    PacketHeader pktHdr;
    MailHeader mailHdr;
    TcpLiteHeader tcpHdr;
    char buffer[MaxTcpLiteMessageSize];
    bzero(buffer, MaxTcpLiteMessageSize);

    int len = tcp->Receive(&pktHdr, &mailHdr, &tcpHdr, buffer);

    printf("Receiver: got \"%s\" (%d bytes)\n", buffer, len);
}

void
FTPTest(int farAddr, char *filename) {
    ftp->SendFile(farAddr, filename);
}
void
FTPReceiverTest() {
    ftp->Serve();
}