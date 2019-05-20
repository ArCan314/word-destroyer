#pragma once

#include "WS2tcpip.h"
#include "WinSock2.h"
#pragma comment(lib, "Ws2_32.lib")

#include "my_socket.h"
#include "my_packet.h"

class Controller;
class MyQueue;

class Resolver
{
public:
    Resolver() = delete;
    Resolver(const Resolver &other) = delete;
    Resolver &operator=(const Resolver &rhs) = delete;
    Resolver(MyQueue *queue, Controller *controller) : job_queue_(queue), controller_(controller_), socksend_(SEND_SOCKET){};

    void Start();
	void get_packet();
	// 供job_queue分配
	void set_queue(MyQueue *queue);
	// 响应
	void Responce();

private:
	MyQueue *job_queue_; // 从该队列中取出数据并处理发送

    Controller *controller_;
    MySocket socksend_;	 // 发回给用户
	MyPacket packet_;
    MyPacket send_packet_;
};