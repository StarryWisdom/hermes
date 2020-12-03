#pragma once

//todo consider moving this to core_lib

#include <experimental/net>

namespace net = std::experimental::net;

class queued_nonblocking_socket {
public:
	net::ip::tcp::socket socket;//this wants to become private but needs more work elsewhere

	// this creates a kind of dummy socket which is wrong but given what I need out of hermes I am not doing better
	// code requiring this feature is probably really a bug hiding in disguse
	queued_nonblocking_socket(net::io_context& cont)
		: socket(cont)
	{
	}

	queued_nonblocking_socket(net::ip::tcp::socket sock)
		: socket(std::move(sock))
	{
		socket.non_blocking(true);
	}

	constexpr static size_t default_max_queue=1024*1024*10;

	void enqueue_write(const std::string& str, const size_t max_queue=default_max_queue) {
		std::deque<std::byte> tmp(str.size());
		std::transform(str.begin(),str.end(),tmp.begin(),[] (char c) {return std::byte(c);});
		enqueue_write(tmp,max_queue);
	}

	void enqueue_write(const std::deque<std::byte>& buffer, const size_t max_queue=default_max_queue) {
		write_remaining.insert(std::end(write_remaining), std::begin(buffer), std::end(buffer));
		if (write_remaining.size()>=max_queue) {
			throw std::runtime_error("network queue overflow");
		}
	}


	size_t attempt_write_buffer() {
		if (write_remaining.size()==0) {
			return 0;
		} else {
			std::error_code err;
			size_t write_len=socket.write_some(std::experimental::net::buffer(write_remaining), err);
			if (!!err && err != std::experimental::net::error::would_block) {
				throw std::runtime_error("error inside network write - "+err.message());
			}
			if (write_len==write_remaining.size()) {
				write_remaining.clear();
			} else {
				std::vector<std::byte> left(write_remaining.begin()+write_len,write_remaining.end());
				write_remaining=left;
			}
			return write_len;
		}
	}

	std::optional<std::vector<std::byte>> attempt_read(const size_t read_len) {
		if (read_len<read_bytes.size()) {
			throw std::runtime_error("network queue error on read");
		}
		const size_t remaining_len=read_len-read_bytes.size();
		size_t current_len=read_bytes.size();
		read_bytes.resize(read_len);
		std::error_code err;
		current_len+=socket.read_some(net::buffer(read_bytes.data()+current_len,remaining_len),err);
		if (!!err && err != std::experimental::net::error::would_block) {
			throw std::runtime_error("error inside network read - "+err.message());
		}
		if (current_len!=read_len) {
			read_bytes.resize(current_len);
			return std::optional<std::vector<std::byte>>();
		} else {
			std::optional<std::vector<std::byte>> ret=read_bytes;
			read_bytes.clear();
			return ret;
		}
	}

	size_t pending_write_bytes() {
		return write_remaining.size();
	}

	std::vector<std::byte> write_remaining;
	std::vector<std::byte> read_bytes;
};
