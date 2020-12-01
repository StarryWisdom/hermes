#pragma once

//to think about class name is probably wrong. committed in the big mess a few commits back

#include <experimental/net>

#include <iostream>

enum class mux_cmds{
	request_new_connection = 1,
	disconnect = 2,
	data = 5, //TODO change to 3?
	recv = 3,
	send = 4,
};

class hermes_n_to_one{
private:
	hermes& cached_hermes;
public:
	hermes_n_to_one(hermes& hermes,std::experimental::net::ip::tcp::socket s)
	: cached_hermes(hermes)
	, client(std::move(s))
	{}

	bool read_a_multiplexed(){//const hermes& hermes) {
		if (!next_cmd.has_value()) {
			const auto tmp=client.attempt_read(1);
			if (tmp.has_value()) {
				next_cmd=static_cast<mux_cmds>((*tmp)[0]);
			} else {
				return false;
			}
		}
		if (next_cmd.has_value()) {
			if (!(	*next_cmd==mux_cmds::request_new_connection ||
				*next_cmd==mux_cmds::disconnect ||
				*next_cmd==mux_cmds::recv ||
				*next_cmd==mux_cmds::send)) {
					throw std::runtime_error("invalid multiplex stream");
			}
			const auto tmp=client.attempt_read(4);
			if (tmp.has_value()) {
				uint32_t len=0;
				len+=static_cast<uint32_t>((*tmp)[0]);
				len+=static_cast<uint32_t>((*tmp)[1])<<8;
				len+=static_cast<uint32_t>((*tmp)[2])<<16;
				len+=static_cast<uint32_t>((*tmp)[3])<<24;
				cmd_len=len;
			} else {
				return false;
			}
		}
		// all commands have a length
		if (cmd_len.has_value()) {
			const auto buffer{client.attempt_read(*cmd_len)};
			if (buffer.has_value()) {
				if (*next_cmd==mux_cmds::request_new_connection) {
					cached_hermes.add_proxy(cached_hermes);
std::cout << "connect\n";
				} else if (*next_cmd==mux_cmds::disconnect) {
std::cout << "disconnect\n";
				} else if (*next_cmd==mux_cmds::recv) {
std::cout << "recv\n";
				} else if (*next_cmd==mux_cmds::send) {
std::cout << "send\n";
				} else if (*next_cmd==mux_cmds::data) {
//					if (*buffer.length()<4) {
//						throw std::runtime_error("multiplexer error - data is missing stream ID");
//					}
				} else {
					// this shouldnt happen - the check for this case should be covered above
					// but given how often things that shouldnt happen do happen ....
					throw std::runtime_error("multiplexer stream has broken inside hermes");
				}
				next_cmd.reset();
				cmd_len.reset();
			} else {
				return false;
			}
		}
		return true;
	}

	void read_multiplexed() {
		read_a_multiplexed();
	}

	void server_tick(){//class hermes* ) {
		read_multiplexed();//*hermes);
		// send
		// recv
	}

private:
	std::unordered_map<uint32_t,uint32_t> stream_details;
	queued_nonblocking_socket client;
	std::optional<mux_cmds> next_cmd;
	std::optional<uint32_t> cmd_len;
};
