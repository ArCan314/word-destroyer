#include "WinSock2.h"
#include "WS2tcpip.h"
#pragma comment(lib, "Ws2_32.lib")

#include <string>

#include "resolver.h"
#include "controller.h"
#include "job_queue.h"

void Resolver::Start()
{
	while (true)
	{
		get_packet();
		send_packet_ = controller_->get_responce(packet_);
		Responce();
	}
}

void Resolver::get_packet()
{
	packet_ = job_queue_->pop_front();
}

void Resolver::Responce()
{
    // error handling
    socksend_.SendTo(send_packet_);
}