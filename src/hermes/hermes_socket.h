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
		fill_read_buffer(read_len);
		if (read_bytes.size()!=read_len) {
			return std::optional<std::vector<std::byte>>();
		} else {
			std::optional<std::vector<std::byte>> ret=read_bytes;
			read_bytes.clear();
			return ret;
		}
	}

	// I think this may be sutiable for swtiching to co-routines,
	// also there is a distinct lack of testing code which really should be fixed
	std::optional<std::deque<std::byte>> get_next_artemis_packet() {
		auto ret{std::optional<std::deque<std::byte>>()};
		const auto length_sub20{get_uint32_at(16)}; // this will force a read, thus it wants to be first to avoid multiple reads if not needed
		const auto length{get_uint32_at(4)};
		const auto header{get_uint32_at(0)};
		if (header.has_value() && length.has_value() && length_sub20.has_value()) {
			if (*header!=0xdeadbeef || *length_sub20+20!=*length) {
				throw std::runtime_error("error in artemis packet");
			}
			const auto read{attempt_read(*length)};
			if (read.has_value()) {
				ret=std::deque<std::byte>{read->begin(),read->end()};
			}
		}
		return ret;
	}

	size_t pending_write_bytes() {
		return write_remaining.size();
	}

	std::vector<std::byte> write_remaining;
	std::vector<std::byte> read_bytes;
private:
	void fill_read_buffer(const size_t read_len) {
		if (read_len<read_bytes.size()) {
			// this needs much better testing if we are going to rely on not dropping data
			// as such we are just going to abort if this is called
			// this can and should be fixed
			// throwing here makes it possible to fix with only a tiny chance of breakage elsewhere
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
		}
	}

	std::optional<uint32_t> get_uint32_at(const size_t offset) {
		if (read_bytes.size()<offset+4) {
			fill_read_buffer(offset+4);
		}
		if (read_bytes.size()<offset+4) {
			return std::optional<uint32_t>();
		} else {
			uint32_t val;
			val=*((uint32_t*)(&read_bytes.at(offset)));
			return val;
		}
	}
};
