#pragma once

#include <experimental/net>

#include "sol/sol.hpp"

#include "hermes.pb.h"

class hermes_gm {
private:
	sol::state lua;
	std::string pending_data;
	std::string from_client;
	unsigned int write_offset=0;
	static inline hermes_gm* this_gm{0};
	static inline class hermes* hermes{0};
public:
	bool send_log{false};
	queued_nonblocking_socket client;

	hermes_gm(std::experimental::net::ip::tcp::socket s)
			: client(std::move(s))
	{
		lua.set_function("sendIdleTextAllClients",send_idle_text);
		lua.set_function("sendCommsTextAllClients",send_comms_text);
		lua.set_function("sendPopup",send_popup);
		lua["commsFilterAlert"]=0x01;
		lua["commsFilterSide"]=0x02;
		lua["commsFilterStatus"]=0x04;
		lua["commsFilterPlayer"]=0x08;
		lua["commsFilterStation"]=0x10;
		lua["commsFilterEnemy"]=0x20;
		lua["commsFilterFriend"]=0x40;
		lua["popupShip1"]=0x01;
		lua["popupShip2"]=0x02;
		lua["popupShip3"]=0x04;
		lua["popupShip4"]=0x08;
		lua["popupShip5"]=0x10;
		lua["popupShip6"]=0x20;
		lua["popupShip7"]=0x40;
		lua["popupShip8"]=0x80;
		lua["popupMainScreen"]=0x001;
		lua["popupHelm"]=0x002;
		lua["popupWeapons"]=0x004;
		lua["popupEngineering"]=0x008;
		lua["popupScience"]=0x010;
		lua["popupCommunications"]=0x020;
		lua["popupFighter"]=0x040;
		lua["popupData"]=0x80;
		lua["popupObserver"]=0x100;
		lua["popupCaptainsMap"]=0x200;
		lua["popupGameMaster"]=0x400;

		lua.set_function("kickClients",kick_client);
		lua["kickShip1"]=0x0;
		lua["kickShip2"]=0x1;
		lua["kickShip3"]=0x2;
		lua["kickShip4"]=0x3;
		lua["kickShip5"]=0x4;
		lua["kickShip6"]=0x5;
		lua["kickShip7"]=0x6;
		lua["kickShip8"]=0x7;
		lua["kickMainScreen"]=0x0;
		lua["kickHelm"]=0x01;
		lua["kickWeapons"]=0x02;
		lua["kickEngineering"]=0x03;
		lua["kickScience"]=0x04;
		lua["kickCommunications"]=0x05;
		lua["kickFighter"]=0x06;
		lua["kickData"]=0x7;
		lua["kickObserver"]=0x8;
		lua["kickCaptainsMap"]=0x9;
		lua["kickGameMaster"]=0xa;

		lua.set_function("updateShipServer",update_ship_server_info);
		lua["ship1"]=0x0;
		lua["ship2"]=0x1;
		lua["ship3"]=0x2;
		lua["ship4"]=0x3;
		lua["ship5"]=0x4;
		lua["ship6"]=0x5;
		lua["ship7"]=0x6;
		lua["ship8"]=0x7;

		lua.set_function("setSetting",set_setting);

		lua.set_function("requestLog",request_log);
	}

	static void assert_hermes() {
		if (hermes==nullptr) {
			throw std::runtime_error("internal error for gm client");
		}
	}

	static void request_log() {
		assert_hermes();
		if (this_gm!=0) {
			this_gm->send_log=true;
		}
	}

	static void kick_client(uint8_t ship_num, uint8_t console) {
		assert_hermes();
		if (ship_num<=7 && console<=11) {
			hermes->for_each_proxy(
				[ship_num, console]
				(auto& prox, uint32_t) {
					if (prox.ship_num==ship_num && prox.consoles[console]==1) {
						prox.enqueue_client_write_slow(artemis_packet::server_to_client::make_popup("this client is being kicked as requested by the GM"));
						// we don't wait for the client to receive the popup, via testing sending, flushing and immediately closing gives enough time for the client to display
						prox.attempt_client_write();
						// the throw will be caught within for_each_proxy, implicitly destroying the hermes_proxy
						throw std::runtime_error("kicked at gm request");
					}
				}
			);
		}
	}

	static void send_idle_text(const std::string& from, const std::string& text) {
		assert_hermes();
		if (from!="" && text!="") {
			hermes->for_each_proxy(
				[from,text]
				(auto& prox, uint32_t)
				{
					prox.enqueue_client_write_slow(artemis_packet::server_to_client::make_idle_text(from,text));
				}
			);
		}
	}

	static void send_comms_text(uint16_t filters, const std::string& from, const std::string& msg) {
		assert_hermes();
		if (from!="" && msg!="") {
			hermes->for_each_proxy(
				[filters, from, msg]
				(auto& prox, uint32_t) {
					prox.enqueue_client_write_slow(artemis_packet::server_to_client::make_comm_text(filters,from,msg));
				}
			);
		}
	}

	static void send_popup(const uint16_t console_bitfield, const uint8_t ship_bitfield, const std::string& msg) {
		assert_hermes();
		if (msg!="") {
			hermes->for_each_proxy(
				[console_bitfield, ship_bitfield, msg]
				(auto& prox,uint32_t) {
					uint8_t console_num{0};
					if ((ship_bitfield&(1<<prox.ship_num))) {
						for (uint8_t lock : prox.consoles) {
							if (lock == 1) {
								uint32_t bit=1<<console_num;
								if (console_bitfield&bit) {
									prox.enqueue_client_write_slow(artemis_packet::server_to_client::make_popup(msg));
									break;
								}
							}
							++console_num;
						}
					}
				}
			);
		}
	}

	static void update_ship_server_info(uint8_t ship_num, const std::string name, const std::string port) {
		assert_hermes();
		//this is horrible but fixes will be hard, do that later
		std::cout << "update_ship_server_info is broken at the moment - sorry\n";
		// we rely on the reconnection logic to fix up the currently connected clients
		if (ship_num<=8) {
			hermes->server_info/*[ship_num]*/=artemis_server_info(name,port);
			hermes->for_each_proxy(
					[ship_num]
					(auto& prox,uint32_t) {
						if (ship_num==prox.ship_num) {
							prox.fake_server_disconnect();
						}
					}
				);
		}
	}

	static void set_setting(const std::string setting_name, const bool enabled) {
		assert_hermes();
		hermes->settings.set_setting(setting_name,enabled);
	}

	void server_tick(class hermes* hermes_class) {
		hermes=hermes_class;
		this_gm=this;
		//we are going to read 1000 charaters at a time
		//not really right but at 1000 bytes a sec thats 1MB/s read speed which is fine
		const int resize_amount(1000);
		std::error_code err;
		from_client.resize(write_offset+resize_amount);
		write_offset+=client.socket.read_some(std::experimental::net::buffer(from_client.data() + write_offset, resize_amount), err);
		if (!!err&&err != std::experimental::net::error::would_block) {
			throw std::runtime_error("gm read fail");
		}
		auto newline_loc=from_client.find('\n');
		if (newline_loc!=std::string::npos && newline_loc<=write_offset) {//we have found a newline
			from_client.resize(write_offset);
			std::string cmd=from_client.substr(0,newline_loc);
			from_client=from_client.substr(newline_loc+1);
			write_offset=from_client.size();
			try {
				lua.script(cmd);
			} catch (const std::exception& e) {
				std::cout << "failed gm command =" << cmd << "\n";
				throw;
			}
		}
		client.attempt_write_buffer();
		this_gm=0;
		hermes=0;
	}
};
