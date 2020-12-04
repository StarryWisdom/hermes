#pragma once

#include "hermes_socket.h"
#include <experimental/net>

namespace net = std::experimental::net;

class client_socket {
public:
	virtual std::optional<std::deque<std::byte>> get_next_artemis_packet()=0;
	virtual void enqueue_write(const std::deque<std::byte>&)=0;
	virtual void attempt_write_buffer()=0;
	virtual ~client_socket(){}
};

class client_real_socket :public client_socket {
private:
	queued_nonblocking_socket socket;
public:
	client_real_socket(net::ip::tcp::socket&& s)
		: socket(std::move(s))
		{}

	std::optional<std::deque<std::byte>> get_next_artemis_packet() final {
		return socket.get_next_artemis_packet();
	}

	void enqueue_write(const std::deque<std::byte>& data) final {
		socket.enqueue_write(data);

	}

	void attempt_write_buffer() final {
		socket.attempt_write_buffer();
	}
};

class client_socket_multiplexer :public client_socket {
public:
	std::optional<std::deque<std::byte>> get_next_artemis_packet() final {
		return std::optional<std::deque<std::byte>>();
	};

	void enqueue_write(const std::deque<std::byte>&) final {}
	void attempt_write_buffer() final {}
};
