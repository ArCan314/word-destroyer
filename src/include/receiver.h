#pragma once

#include "WS2tcpip.h"
#include "WinSock2.h"
#pragma comment(lib, "Ws2_32.lib")

#include "my_socket.h"

class JobQueue;

class Receiver
{
public:
    Receiver() = delete;
    Receiver(const Receiver &other) = delete;
    Receiver &operator=(const Receiver &rhs) = delete;

    Receiver(JobQueue* job_queue) : sockrecv_(RECV_SOCKET, "9961"), job_queue_(job_queue){}
    ~Receiver() = default;
    
	void start();

private:

	JobQueue* job_queue_;

	MySocket sockrecv_;
};