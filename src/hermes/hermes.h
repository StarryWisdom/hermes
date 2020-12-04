#pragma once

#include <array>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>
#include <vector>
#include <experimental/net>

#define SOL_USING_CXX_LUA 1

#include "sol/sol.hpp"

#include "settings.h"
#include "artemis_server_info.h"
#include "client_socket.h"

namespace net = std::experimental::net;

class hermes {
public:
	hermes(uint16_t artemis_port=2010, std::optional<uint16_t> n_to_one_port=std::optional<uint16_t>());
	inline static uint32_t version{17};

	void main_loop();

	void async_resolve(const artemis_server_info& server_info, const uint32_t connection_id) {
		resolver.async_resolve(server_info.server_name,server_info.port, [this,connection_id](auto ec, auto end_point) {
			handle_resolve(connection_id,ec,end_point);
		});
	}

	void handle_resolve(uint32_t id, const std::error_code& ec, const net::ip::tcp::resolver::results_type& endpoints);

	void log_and_print(const std::string& msg) {
		std::ostringstream stream;
		std::time_t t = std::time(nullptr);
		stream << std::put_time(std::localtime(&t), "%c %Z") << ": " << msg << "\n";
		std::cout << stream.str();
		if (log.size()>(1024*1024)) {
			log.clear();
		}
		log+=stream.str();
	}

	class settings settings;
	artemis_server_info server_info;
	/* this is nearly completely wrong
	 * fix it up when I support multiple servers
	std::array<artemis_server_info,8> server_info{
		artemis_server_info(),
		artemis_server_info(),
		artemis_server_info(),
		artemis_server_info(),
		artemis_server_info(),
		artemis_server_info(),
		artemis_server_info(),
		artemis_server_info()};
	*/
	void start_gm_accept();
	void start_proxy_accept();
	void start_n_to_one_accept();

	template <typename fn> void for_each_proxy(fn func);

	net::io_context context;
	void add_proxy(std::unique_ptr<client_socket>&& soc);
private:
	class impl;
	std::unique_ptr<impl> _impl;

	template <typename T, typename fn> static void for_each_client(std::unordered_map<uint32_t,T>* t, fn func) {
		auto it=t->begin();
		for (;it!=t->end();) {
			bool remove(false);
			try {
				func(it->second,it->first);
			} catch (std::exception& e) {
				std::cout << std::string("thread disconnect reason ")+e.what()+"\n";
				remove=true;
			}
			if (remove) {
				it=t->erase(it);
			} else {
				it++;
			}
		}
	}

	net::ip::tcp::resolver resolver{context};
	std::experimental::net::ip::tcp::acceptor artemis_accept;
	std::experimental::net::ip::tcp::acceptor gm_accept{context, net::ip::tcp::endpoint(net::ip::tcp::v4(), 2015)};
	std::optional<std::experimental::net::ip::tcp::acceptor> n_to_one_accept;
	std::string log;
};

#include "hermes_watcher.h"
#include "hermes_gm.h"
#include "hermes_proxy.h"
#include "hermes_n_to_one.h"

class hermes::impl {
public:
	// we need an id for the threads to make it work in a aync context later
	uint32_t id{0};//todo make private
	std::unordered_map<uint32_t,hermes_proxy> threads;
	std::unordered_map<uint32_t,hermes_gm> gm_threads;
	std::unordered_map<uint32_t,hermes_watcher> watcher_threads;
	std::unordered_map<uint32_t,hermes_n_to_one> n_to_one_threads;
};

template <typename fn> void hermes::for_each_proxy(fn func) {
	for_each_client(&_impl->threads,func);
}
