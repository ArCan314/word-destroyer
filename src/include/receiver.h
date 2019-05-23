#pragma once
#include <thread>
#include <sstream>

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include "my_socket.h"
#include "log.h"

class JobQueue;

// server
class Receiver
{
public:
    Receiver() = delete;
    Receiver(const Receiver &other) = delete;
    Receiver &operator=(const Receiver &rhs) = delete;

    Receiver(JobQueue* job_queue) : sockrecv_(RECV_SOCKET, "9961"), job_queue_(job_queue)
	{
		Log::WriteLog(std::string("Resolver") + " :init.");
	}
    ~Receiver() = default;
    
	void Start();

private:

	JobQueue* job_queue_;

	MySocket sockrecv_;
};

