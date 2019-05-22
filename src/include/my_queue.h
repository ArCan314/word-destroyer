#pragma once

#include <deque>
#include <mutex>
#include <utility>
#include <condition_variable>

#include "my_packet.h"

class MyQueue
{
public:
	MyQueue() = default;

	void push_back(const MyPacket &qdata);
	MyPacket pop_front();

private:
	std::deque <MyPacket> data_queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
	int size_ = 0;
};