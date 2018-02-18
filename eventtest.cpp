#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <condition_variable>

struct Event {
	int pollfd;
	int fd;
	bool manual;
};

void event_create(Event* ev, bool manual)
{
	ev->fd = eventfd(0, EFD_NONBLOCK);
	ev->pollfd = epoll_create1(0);
	ev->manual = manual;
	epoll_event event;
	event.events = EPOLLIN | EPOLLET;
	if (ev->manual) {
		event.events |= EPOLLONESHOT;
	}
	event.data.fd = ev->fd;
	epoll_ctl(ev->pollfd, EPOLL_CTL_ADD, ev->fd, &event);
}

void event_signal(Event* ev)
{
	uint64_t value = 1;
	write(ev->fd, &value, sizeof(value));
}

void event_wait(Event* ev)
{
	epoll_event result;
	printf("enter wait\n");
	epoll_wait(ev->pollfd, &result, 1, -1);
	
	if (ev->manual) {
		uint64_t buf;
		read(ev->fd, &buf, sizeof(buf));
		epoll_ctl(ev->pollfd, EPOLL_CTL_MOD, result.data.fd, &result);
	}
}

void event_reset(Event* ev)
{
	uint64_t buf;
	read(ev->fd, &buf, sizeof(buf));
	epoll_event event;
	event.data.fd = ev->fd;
	event.events = EPOLLIN | EPOLLET;
	if (ev->manual) {
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl(ev->pollfd, EPOLL_CTL_MOD, ev->fd, &event);
}

int main()
{
	//Event ev;
	//event_create(&ev, false);

	//std::thread t1([ev]() mutable {event_wait(&ev);printf("hoge\n");});
	//std::thread t2([ev]() mutable {event_wait(&ev);printf("uhe\n");});

	//event_signal(&ev);	
	//t1.join();
	//t2.join();
	bool ready = false;
	std::mutex mut;
	std::condition_variable cond;
	std::thread t1([&]() {std::unique_lock<std::mutex> l(mut);cond.wait(l, [&]()->bool{return ready;}); printf("hoge\n");});
	std::thread t2([&]() {std::unique_lock<std::mutex> l(mut);cond.wait(l, [&]()->bool{return ready;}); printf("uhe\n");});

	{
		std::unique_lock<std::mutex> l(mut);
		ready = true;
		cond.notify_one();
	}
	t1.join();
	t2.join();
	return 0;
}
