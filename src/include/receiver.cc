#include "WinSock2.h"
#include "WS2tcpip.h"
#pragma comment(lib, "Ws2_32.lib")

#include "my_packet.h"
#include "my_socket.h"
#include "job_queue.h"
#include "receiver.h"

void Receiver::Start()
{
    while (true)
    {
        MyPacket temp;
        temp = sockrecv_.RecvFrom();
        job_queue_->Push(temp);
    }
}