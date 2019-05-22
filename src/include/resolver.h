#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include "my_socket.h"
#include "my_packet.h"
#include "job_queue.h"

class ServerController;
class MyQueue;

class Resolver
{
public:
    Resolver() = delete;
    Resolver(const Resolver &other) = delete;
    Resolver &operator=(const Resolver &rhs) = delete;
    Resolver(JobQueue *queue, ServerController *controller) : controller_(controller), socksend_(SEND_SOCKET)
	{
		queue->Bind(this);
	};

    void Start();
	void get_packet();
	void set_queue(MyQueue *queue) { job_queue_ = queue; }

	// 响应
	void Responce();

private:
	MyQueue *job_queue_; // 从该队列中取出数据并处理发送

    ServerController *controller_;
    MySocket socksend_;	 // 发回给用户
	MyPacket packet_;
    MyPacket send_packet_;
};