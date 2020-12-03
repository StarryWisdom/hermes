#pragma once

#include "artemis-lib/packet.h"

#include "sol/sol.hpp"

#include <iostream>
#include <experimental/net>
#include <locale>
#include <filesystem>

#include "hermes.h"
#include "hermes_thread.h"
#include "artemis_server_info.h"

namespace net = std::experimental::net;

class hermes_watcher {
private:
	//I need to look into a better way for this
	//this makes hermes_watcher thread unsafe
	//its because when we call from lua back to c++ we wont have a this pointer
	static inline hermes_to_server* lua_to_server=nullptr;
//note this is wrong - its not connecting to each machine whem there are multiple servers
//also I really dont trust that it will work if ship0 is changed from the default server
//both of these issues will be fixed later if ever
	static void set_ship_type(const std::string& name, const uint32_t ship_id, const float accent) {
		const artemis_packet::ship_settings settings(false,ship_id,accent,name);

		const auto tmp=artemis_packet::client_to_server::make_ship_settings(settings);
		std::deque<std::byte> buffer{tmp.buffer.begin(),tmp.buffer.begin()+tmp.write_offset};

		lua_to_server->sock.enqueue_write(buffer);
	}

	static void set_ship_num(const uint8_t ship) {
		const auto tmp=artemis_packet::client_to_server::make_set_ship_packet(ship);
		std::deque<std::byte> buffer{tmp.buffer.begin(),tmp.buffer.begin()+tmp.write_offset};

		lua_to_server->sock.enqueue_write(buffer);
	}
public:
	hermes_watcher(net::io_context& io_context)
		: to_server(io_context)
	{}

	hermes_to_server to_server;

	void server_tick(class hermes* hermes_class,artemis_server_info* server_info, const uint32_t connection_id) {
		if (!to_server.connected) {
			to_server.async_connect(*server_info,hermes_class, connection_id);
		}
		if (to_server.connected) {
			if (!has_start_run) {
				sol::state lua;
				lua.set_function("setShipNum",set_ship_num);
				lua.set_function("setShipType",set_ship_type);
				lua_to_server=&to_server;

				//if there is no file on the disc we are going to just connect then sit here
				//this as we dont want to repeatedly retry due to a missing file
				//and it seems a smaller fix than stopping the connection in the first place
				const std::string connect_file{"hermes_connect.lua"};
				if (std::filesystem::exists(connect_file)) {
					lua.script_file(connect_file);
				}
				lua_to_server=nullptr;
				has_start_run=true;
			}
			for (;;) {
				if (!to_server.sock.get_next_artemis_packet().has_value()) {
					break;
				}
			}
			to_server.sock.attempt_write_buffer();
		}
	}

private:
	bool has_start_run{false};
};
