#pragma once

#include <experimental/net>
#include <optional>

#include "hermes_socket.h"
#include "artemis_server_info.h"

namespace net = std::experimental::net;

class hermes_to_server {
public:
	hermes_to_server(net::io_context& io_context)
		: sock(io_context)
	{}

	// todo both of these probably want to become private
	bool connected{false};
	bool connect_in_flight{false};
	queued_nonblocking_socket sock;

	void handle_connect(const std::error_code& ec) {
		connect_in_flight=false;
		if (ec) {
			// connect failed
			// this would be nice to log
			// but it will spam the log incredibly
			// there is a nice solution with backing off connecting
			// but its not worth the effort to code it at this time
		} else {
			sock.socket.non_blocking(true);
			connected=true;
		}
	}

	void async_connect(const artemis_server_info& server_info, class hermes* hermes, const uint32_t connection_id) {
		if (connected) {
			throw std::runtime_error("usage error in hermes_to_server - trying to reconnect without resetting socket");
		}
		if (!connect_in_flight) {
			connect_in_flight=true;
			{
				//this may be uneeded, but is an attempt to fix ghost data
				//this also is kind of ugly from an abstaction sense
				//probably hermes_socket needs reworking as well
				sock.write_remaining.clear();
				sock.read_bytes.clear();
			}
			hermes->async_resolve(server_info,connection_id);
		}
	}
};
