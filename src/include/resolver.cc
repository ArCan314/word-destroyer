#include <string>

#include "WS2tcpip.h"
#include "WinSock2.h"
#pragma comment(lib, "Ws2_32.lib")

#include "resolver.h"
#include "job_queue.h"

void Resolver::Start()
{
	while (true)
	{
		get_packet();
        send_packet_ = controller_.process_query(packet_);
        Responce();
	}
}

void Resolver::get_packet()
{
	MyPacket temp_data = job_queue_->pop_front();
}

void Resolver::set_queue(MyQueue *queue)
{
	job_queue_ = queue;
}

void Resolver::Responce()
{
    // error handling
    socksend_.SendTo(send_packet_);
}