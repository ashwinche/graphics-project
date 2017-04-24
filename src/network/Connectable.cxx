#include "Connectable.hxx"

Connectable::Connectable()
{}

Connectable::Connectable(short port, const char *hostname)
{}

Connectable::~Connectable()
{}

/* Reads the least recently recieved message from our listening
    socket and copies it into the message buffer as a messageContainer. */
void Connectable::recieveOneMessage(int socket)
{
    // Read a packet from the UDP socket
    uint8_t bytebuf[MAX_MESSAGE_LENGTH+1];
    struct sockaddr_in src;
    socklen_t s = sizeof(sockaddr_in);
    int bytesRead;

    bytesRead = recvfrom(socket, bytebuf, MAX_MESSAGE_LENGTH, 
    	0, (struct sockaddr *)&src, &s);

    bytebuf[bytesRead] = '\0'; //make it a string

    // Create message struct
    MessageContainer *m = new MessageContainer(src, (char*) &bytebuf, bytesRead+1);

    //add to the queue
    //log(LOGMEDIUM, "Recieved the message: %s\n", bytebuf);
    this->queue.addMessage(m);

}


/* Sends a message to the given target through the given socket. */
void Connectable::sendOneMessage(MessageContainer *msg, int socket, struct sockaddr* target)
{
	log(LOGMEDIUM, "Sending message: %s (length: %d)\nTo: %s at port on socket %d\n", msg->msg, msg->msgLen,
		inet_ntoa(((struct sockaddr_in*)target)->sin_addr), socket);
	int bytesSent = sendto(socket, msg->msg, msg->msgLen, 0, target, sizeof(struct sockaddr_in));
	log(LOGMEDIUM, "Sent %d bytes\n", bytesSent);
}

/* Creates a new thread which constantly reads the socket and adds
incoming messages to the message buffer. We can't call this until
out socket has been set up.  */
void Connectable::initalizeListeningThread()
{
    pthread_create(&this->t, NULL, listeningDeamon, this);
}




/* A function to be called as the main function of a new thread.
Constantly checks the socket for incoming messages and reads them
into the queue to be processed by the main thread. */
void * listeningDeamon(void *connectable)
{
    /* Stack exchange tells me we don't have to lock the socket
    before reading since concurrent calls to recvfrom and sendto
    are syncronized by the OS since sockets belong to the process
    and not the thread. */

    Connectable * c = (Connectable*)connectable;

    while(1){ // forever
        c->recieveOneMessage(c->getSocket());
    }
}