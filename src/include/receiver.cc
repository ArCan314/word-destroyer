#include "WS2tcpip.h"
#include "WinSock2.h"
#pragma comment(lib, "Ws2_32.lib")

#include "my_packet.h"
#include "my_socket.h"
#include "job_queue.h"
#include "receiver.h"

void Receiver::start()
{
    while (true)
    {
        MyPacket temp;
        temp = sockrecv_.RecvFrom();
        job_queue_->Push(temp);
    }
}