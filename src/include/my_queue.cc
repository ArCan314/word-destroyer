#include <deque>
#include <mutex>
#include <utility>
#include <condition_variable>

#include "my_queue.h"
#include "my_packet.h"

void MyQueue::push_back(const MyPacket &qdata)
{
	std::lock_guard<std::mutex> guard(mutex_);
	data_queue_.push_back(qdata);
	size_++;
	cond_.notify_one();
}

MyPacket MyQueue::pop_front()
{
	MyPacket res;
	std::unique_lock<std::mutex> guard(mutex_);
	if (!size_)
		cond_.wait(guard, [this] { return size_ > 0; });

	res = data_queue_.front();
	data_queue_.pop_front();
	size_--;
	return res;
}
