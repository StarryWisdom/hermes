#include <iostream>
#include <locale>

#include "hermes.h"
#include "hermes_thread.h"
#include "hermes_gm.h"
#include "hermes_proxy.h"
#include "hermes_watcher.h"
#include "hermes_gm_stats.h"

#include "settings_impl.h"
namespace net = std::experimental::net;

void hermes::start_gm_accept() {
	gm_accept.async_accept([this](const std::error_code ec,net::ip::tcp::socket sock) {
		if (!ec) {
			_impl->gm_threads.emplace(std::piecewise_construct,std::forward_as_tuple(_impl->id++),std::forward_as_tuple(std::move(sock)));
		} else {
			std::cout << "accept failed - " << ec.message() << "\n";//probably wants to be swaped to logging
		}
		start_gm_accept();
	});
}

void hermes::start_proxy_accept() {
	artemis_accept.async_accept([this](const std::error_code ec,net::ip::tcp::socket sock) {
		if (!ec) {
			add_proxy(std::make_unique<client_real_socket>(std::move(sock)));
		} else {
			std::cout << "accept failed - " << ec.message() << "\n";//probably wants to be swaped to logging
		}
		start_proxy_accept();
	});
}

void hermes::start_n_to_one_accept() {
	n_to_one_accept->async_accept([this](const std::error_code ec,net::ip::tcp::socket sock) {
		if (!ec) {
			_impl->n_to_one_threads.emplace(std::piecewise_construct,std::forward_as_tuple(_impl->id++),std::forward_as_tuple(*this,std::move(sock)));
		} else {
			std::cout << "accept failed - " << ec.message() << "\n";//probably wants to be swaped to logging
		}
		start_n_to_one_accept();
	});
}

void hermes::add_proxy(std::unique_ptr<client_socket>&& client) {
	_impl->threads.emplace(_impl->id++,hermes_proxy(*this,std::move(client)));
}

void hermes::main_loop() {
	start_gm_accept();
	start_proxy_accept();
	if (n_to_one_accept.has_value()) {
		start_n_to_one_accept();
	}
	auto last_loop=std::chrono::high_resolution_clock::now();
	auto last_loop_without_sleep=last_loop;
	uint32_t tick{0};
	hermes_gm_stats stats;
	for (;;) {
		for (;;) {
			try {
				if(context.poll_one()==0) {
					break;
				}
			} catch (const std::exception& e) {
				std::cout << "error in io_context " << e.what() << "\n";
			}
		}
		context.restart();
		for_each_proxy(
				[this](auto& proxy, uint32_t id) {
					// this has been badly hacked up since supporting multiple servers isnt fully supported
					// and bugs that have been found with the retransmit logic
					const auto previous_ship=proxy.ship_num;
					proxy.server_tick(id);
					if (previous_ship!=proxy.ship_num) {
						if (previous_ship<=8 && proxy.ship_num <=8) {
							const auto& previous_server=server_info/*[previous_ship]*/;
							const auto& current_server=server_info/*[proxy.ship_num]*/;
							if (previous_server.server_name!=current_server.server_name ||
									previous_server.port!=current_server.port) {
								log_and_print("disconnecting client that is jumping between servers");
								proxy.fake_server_disconnect();
							}
						}
					}
				}
		);
		for_each_client(&_impl->gm_threads,[this](auto& proxy,uint32_t){proxy.server_tick(this);});
		for_each_client(&_impl->watcher_threads,[this](auto& proxy,uint32_t id){proxy.server_tick(this,&server_info,id);});//TODO only watch the server for the first ship
		for_each_client(&_impl->n_to_one_threads,[this](auto &n_to_one, uint32_t){n_to_one.server_tick();});
		// this is kind of wrong - it sends it every 100 cycles through this loop
		// rather than a nominal rate of (say) every 100 ms
		if (tick==100) {
			tick=0;
			if (_impl->watcher_threads.size()==0) {
				// we are assuming that if the watcher thread goes down then the server went down
				server_info.server_data.clear_all_data();
				try {
					//TODO add watchers for each server
					_impl->watcher_threads.emplace(std::piecewise_construct,std::forward_as_tuple(_impl->id++),std::forward_as_tuple(context));
				} catch (std::exception&) {
					//hmm this is tricky we want to print the exceptions
					//however also it will have (expected) errors often in running
					//this makes ut hard to pring without spaming messages
					//as such I am just going to comment here
					//maybe a more obvious answer will come to mind when we have a better gm client
				}
			}

			for (auto& [id,i] : _impl->gm_threads) {
				hermes_proto::hermes_server_msg protobuf;

				protobuf.set_allocated_stats(new hermes_proto::hermes_stats(stats.serialize()));
				settings.num_threads=_impl->threads.size()+_impl->gm_threads.size();
				settings.serialize_into(&protobuf,server_info);

				for (const auto& [id, pc] : server_info.server_data.pc_data)  {
					auto pc_dat=protobuf.add_pc_data();
					pc_dat->set_x(pc.hermes_x);
					pc_dat->set_y(pc.hermes_y);
					pc_dat->set_z(pc.hermes_z);
					pc_dat->set_ship_num(pc.ship_num);
				}

				if (i.send_log) {
					protobuf.set_log(log);
					i.send_log=false;
				}

				std::string tmp;
				protobuf.SerializeToString(&tmp);

				std::deque<std::byte> buffer;
				std::transform(tmp.begin(),tmp.end(),buffer.begin(),[] (char c) {return std::byte(c);});

				buffer::push_front<uint32_t>(buffer,tmp.size());
				i.client.enqueue_write(buffer);
			}
		}
		const auto current_loop=std::chrono::high_resolution_clock::now();
		const auto delta_t=current_loop-last_loop;
		stats.increment_timing_stat_with_sleep(std::chrono::duration_cast<std::chrono::milliseconds>(delta_t).count());
		const auto delta_t_without_sleep=current_loop-last_loop_without_sleep;
		stats.increment_timing_stat_without_sleep(std::chrono::duration_cast<std::chrono::milliseconds>(delta_t_without_sleep).count());
		last_loop=current_loop;
		if (delta_t<std::chrono::milliseconds(1)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		last_loop_without_sleep=std::chrono::high_resolution_clock::now();
		++tick;
	}
}

void hermes::handle_resolve(uint32_t id, const std::error_code& ec, const net::ip::tcp::resolver::results_type& endpoints) {
	hermes_to_server* to_server=nullptr;
	if (_impl->threads.count(id)!=0) {
		to_server=&_impl->threads.at(id).to_server;
	}
	if (_impl->watcher_threads.count(id)!=0) {
		to_server=&_impl->watcher_threads.at(id).to_server;
	}
	if (to_server!=nullptr) {
		if (ec) {
			to_server->connected=false;
			to_server->connect_in_flight=false;
			// we may want to log this
			// but I am very concerned about logging spam in the retry loop
			// so we are going to go without logging
		} else {
			net::async_connect(to_server->sock.socket, endpoints,
					[to_server] (auto ec, auto) {
						to_server->handle_connect(ec);
					}
				);
		}
	}
}

hermes::hermes(uint16_t artemis_port, std::optional<uint16_t> n_to_one_port)
	: _impl(new impl)
	,  artemis_accept(context, net::ip::tcp::endpoint(net::ip::tcp::v4(), artemis_port))
{
	if (n_to_one_port.has_value()) {
		n_to_one_accept=std::experimental::net::ip::tcp::acceptor(context, net::ip::tcp::endpoint(net::ip::tcp::v4(), *n_to_one_port));
	}
}
