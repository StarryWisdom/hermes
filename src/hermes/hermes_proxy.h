#pragma once

#include <iostream>
#include <experimental/net>
#include <optional>
#include <deque>

#include "artemis-lib/packet.h"
#include "artemis-lib/bitstream.h"
#include "hermes.h"
#include "hermes_thread.h"

namespace net = std::experimental::net;

class hermes_proxy {
public:
	uint8_t ship_num{0};
	std::array<uint8_t,11> consoles{0,0,0,0,0,0,0,0,0,0,0};
private:
	std::chrono::time_point<std::chrono::system_clock> last_fixup_time{std::chrono::system_clock::now()};
	std::unordered_map<uint32_t,std::chrono::time_point<std::chrono::system_clock>> hull_id_change_time;
	packet_buffer from_server;//pending input from server
	enum class proxy_mode {
		single = 0,
		multiplex = 1,
	};
	proxy_mode mode{proxy_mode::single};
	hermes& cached_hermes;
	std::optional<queued_nonblocking_socket> to_client;
public:
	hermes_to_server to_server;

	hermes_proxy(hermes& hermes, queued_nonblocking_socket s)
		: cached_hermes(hermes)
		, to_client(std::move(s))
		, to_server(hermes.context)
	{}

	// if constructed without a socket we are in multiplex mode
	hermes_proxy(hermes& hermes)
		: cached_hermes(hermes)
		, to_server(hermes.context)
	{
		mode=proxy_mode::multiplex;
	}

	std::vector<std::deque<std::byte>> hermes_cmd(const std::string& msg) {
		std::vector<std::deque<std::byte>> ret;
		if (msg=="/hermes giveMeGM") {
			if (!cached_hermes.settings.disable_give_me_gm) {
				ret.push_back(artemis_packet::server_to_client::make_idle_text("hermes","gm granted"));
				consoles[10]=1;
			} else {
				ret.push_back(artemis_packet::server_to_client::make_idle_text("hermes","gm denied (hermes startup option)"));
			}
			ret.push_back(artemis_packet::server_to_client::make_client_consoles(ship_num,consoles));
		} else {
			ret.push_back(artemis_packet::server_to_client::make_idle_text("hermes","unknown command"));
		}
		return ret;
	}

private:
	artemis_sent_data transmitted_to_client;
	bool first_connect_run{false};
	bool suppress_next_ship_num_packet{false};
	// the reason we support spressing a ship_num packets is as follows
	// we want to suppress the first one on a new server connection
	// stock artemis on a connection will say "yes you are on ship 0"
	// this would be fine as we would overwrite it quickly ***but***
	// if we support multiple servers imagine
	// ship0=server0 ship1=server1
	// as soon as we connect to ship1 (and thus switch to server1)
	// the server will say "hello you are on ship0" which will result in hermes disconnecting and reconnecting

	packet_buffer incomplete_from_client;
	std::optional<packet_buffer> get_next_client_packet() {
		if (mode==proxy_mode::single) {
			if (!to_client.has_value()) {
				throw std::runtime_error("somehow client socket is not set, this shoudnt be possible");
			}
			if (artemis_packet::fetch_artemis_packet(incomplete_from_client,
				[this](uint32_t read_len) {
				incomplete_from_client.realloc_if_needed(read_len);
				std::error_code err;
				incomplete_from_client.write_offset += to_client->socket.read_some(std::experimental::net::buffer(incomplete_from_client.buffer.data() + incomplete_from_client.write_offset, read_len), err);
				if (!!err&&err != std::experimental::net::error::would_block) {
					throw std::runtime_error("client read fail");
				}
			})) {
				packet_buffer ret=incomplete_from_client;
				incomplete_from_client=packet_buffer();
				return ret;
			}
			return std::optional<packet_buffer>();
		} else {
			throw std::runtime_error("TODO");
		}
	}

	void enqueue_client_write_inner(const std::deque<std::byte>& buffer) {
		if (mode==proxy_mode::single) {
			if (!to_client.has_value()) {
				throw std::runtime_error("somehow client socket is not set, this shoudnt be possible");
			}
			to_client->enqueue_write(buffer);
		} else {
			throw std::runtime_error("TODO");
		}
	}
public:
	void attempt_client_write() {
		// we only need to do anything if we are in single mode
		if (mode==proxy_mode::single) {
			if (!to_client.has_value()) {
				throw std::runtime_error("somehow client socket is not set, this shoudnt be possible");
			}
			to_client->attempt_write_buffer();
		}
	}

	void enqueue_client_write(const std::vector<std::deque<std::byte>>& buffers) {
		for (const auto& i : buffers) {
			enqueue_client_write(i);
		}
	}

	void enqueue_client_write(const std::deque<std::byte>& buffer) {
		parse_s2c_packet(buffer);
		enqueue_client_write_inner(buffer);
	}

	std::vector<std::deque<std::byte>> stationary_fixup() {
		object_bitstream bitstream;
		bool tractor_active{false};
		cached_hermes.server_info.server_data.for_each_npc([&tractor_active](uint32_t, const npc& server_real) {
			if (server_real.hermes_active_specials & 64) {
				tractor_active=true;
			}
		});
		// if there are no tractor beams active we wont send the packets keeping ships idle
		// this is due to the delay when impulse and/or warp is enabled for the first time
		// tractor beams are rare enough the extra jitter for other ships when a unrelated ship
		// starting to move isnt a problem
		if (!tractor_active) {
			cached_hermes.server_info.server_data.for_each_pc([&bitstream](uint32_t id, const pc& server_real) {
				if (server_real.hermes_velocity == 0) {
					update_player_data bs;
					bs.id = id;
					bs.x = server_real.hermes_x;
					bs.y = server_real.hermes_y;
					bs.z = server_real.hermes_z;
					bitstream.add_bitstream(bs.build_data());
				}
			});
		}
		return artemis_packet::server_to_client::finalize_object_bitstreams(bitstream.get_bitstreams());
	}

	std::vector<std::deque<std::byte>> data_fixup() {
		object_bitstream bitstream;
		cached_hermes.server_info.server_data.for_each_pc([&bitstream,this](uint32_t id, const pc& server_real){
				const auto client{transmitted_to_client.get_eng_data(id)};
				bitstream.add_bitstream(update_player_data(id,server_real,client).build_data());
				bitstream.add_bitstream(update_engineering_data(id,server_real,client).build_data());
				bitstream.add_bitstream(update_weapons_data(id,server_real,client).build_data());
			});
		cached_hermes.server_info.server_data.for_each_npc([&bitstream,this](uint32_t id, const npc& server_real){
				const auto client{transmitted_to_client.get_npc_data(id)};
				bitstream.add_bitstream(update_npc_data(id,server_real,client).build_data());
			});
		return artemis_packet::server_to_client::finalize_object_bitstreams(bitstream.get_bitstreams());
	}

	void server_tick(const uint32_t connection_id) {
		//this probably wants to be folded into hermes - the sycnronised data breaks the idea that each thread is independent
		if (!to_server.connected) {
			first_connect_run=false;
			to_server.async_connect(cached_hermes.server_info,&cached_hermes, connection_id);
		}
		if (to_server.connected) {
			first_connect();
		}
		if (to_server.connected) {
			client_to_server();
		}
		if (to_server.connected) {
			server_to_client();
		}
		if (to_server.connected) {
			if (cached_hermes.settings.engineering_data_fix) {
				const auto now=std::chrono::system_clock::now();
				std::vector<uint32_t> tmp_rm;
				object_bitstream bitstream;
				// there probably is a nice standard algorithm for this (creating a temp list to be removed after something is done with it) but I dont know what it is
				for (const auto& [id,time] : hull_id_change_time) {
					const auto retransmit_time=std::chrono::milliseconds(1000);
					if (now-time>retransmit_time) {
						if (cached_hermes.server_info.server_data.pc_data.count(id) && transmitted_to_client.pc_data.count(id)) {
							//copied out of data fixup, probably should be merged
							const auto server_real=cached_hermes.server_info.server_data.pc_data.at(id);
							auto delta{update_player_data(id,server_real,nullptr)};
							delta.visual_id.reset();
							bitstream.add_bitstream(delta.build_data());
							bitstream.add_bitstream(update_engineering_data(id,server_real,nullptr).build_data());
							bitstream.add_bitstream(update_weapons_data(id,server_real,nullptr).build_data());
						}
						if (cached_hermes.server_info.server_data.npc_data.count(id) && transmitted_to_client.npc_data.count(id)) {
							//copied out of data fixup, probably should be merged
							const auto server_real=cached_hermes.server_info.server_data.npc_data.at(id);
							auto delta{update_npc_data(id,server_real,nullptr)};
							// if we resend the visual id then all the other data will be reset resulting in more miss predictions
							// if the visual id is still pending from the server IE transmitted to client!=server setting
							// this is fine - it will just be overwritten and dealt with
							delta.visual_id.reset();
							bitstream.add_bitstream(delta.build_data());
						}
						tmp_rm.push_back(id);
					}
				}
				for (const auto& i : tmp_rm) {
					hull_id_change_time.erase(i);
				}
				auto hull_fix{artemis_packet::server_to_client::finalize_object_bitstreams(bitstream.get_bitstreams())};
				enqueue_client_write(hull_fix);
				const auto send_fixes_timer{std::chrono::milliseconds(50)};
				if (now-last_fixup_time>send_fixes_timer) {
					last_fixup_time=now;
					enqueue_client_write(data_fixup());
					enqueue_client_write(stationary_fixup());
				}
			}
		}
		attempt_client_write();
		try {
			to_server.sock.attempt_write_buffer();
		} catch (std::runtime_error&) {
			to_server.connected=false;
		}
	}

	// this probably should be replaced with a function that really disconnects from the server
	// but currently just faking it is good enough
	void fake_server_disconnect() {
		to_server.connected=false;
	}

	void first_connect() {
		if (!first_connect_run) {
			// it may be worth considering sending a inital connection state to clients
			// we are suppressing it as it breaks the inital connection
			suppress_next_ship_num_packet=true;

			const auto tmp=artemis_packet::client_to_server::make_set_ship_packet(ship_num);
			std::deque<std::byte> buffer{tmp.buffer.begin(),tmp.buffer.begin()+tmp.write_offset};

			to_server.sock.enqueue_write(buffer);
			for (auto i = 0; i != 11; i++) {
				if (consoles[i]==1) {
					const auto tmp=artemis_packet::client_to_server::make_set_console(i,true);
					std::deque<std::byte> buffer{tmp.buffer.begin(),tmp.buffer.begin()+tmp.write_offset};
					to_server.sock.enqueue_write(buffer);
				}
			}
			//finally we delete old packets sent from c2s as they may have changed selections of ships etc (and thus confuse users)
			for (;;) {
				if (!get_next_client_packet().has_value()) { // TODO check me
					break;
				}
			}
			from_server=packet_buffer();//clear contents from the last connection (should this somehow be moved to a constructor somewhere?)
			first_connect_run=true;
		}
	}

	void client_to_server() {
		for (;;) {
			auto packet=get_next_client_packet();
			if (packet.has_value()) {
				auto from_client=*packet;
				std::deque<std::byte> d_packet{packet->buffer.begin(),packet->buffer.begin()+packet->write_offset};
				parse_c2s_packet(&from_client);
				if (d_packet.size() >= 24) {
					//if we are a select ship message we will send a release gm console message to avoid an artemis bug
					uint32_t type{buffer::read_at<uint32_t>(d_packet,20)};
					if (type == artemis_packet::client_to_server::value_int_jam32) {
						if (d_packet.size() >= 28) {
							uint32_t subtype{buffer::read_at<uint32_t>(d_packet,24)};
							if (subtype==static_cast<uint32_t>(artemis_packet::value_int::set_ship)) {
								const auto tmp=artemis_packet::client_to_server::make_set_console(10,false);
								std::deque<std::byte> buffer{tmp.buffer.begin(),tmp.buffer.begin()+tmp.write_offset};

								to_server.sock.enqueue_write(buffer);
							}
						}
					//inner gm text commands
					} else if (type == artemis_packet::client_to_server::gm_text_jam32) {
						try {
							auto gm_text{artemis_packet::client_to_server::gm_text_hermes_temp(from_client)};
							if (gm_text.temp_message.substr(0,1) == "/") {
								packet.reset();
								enqueue_client_write(hermes_cmd(gm_text.temp_message));
							}
						} catch (std::runtime_error&) {
						}
					}
				}
				if (packet.has_value()) {
					to_server.sock.enqueue_write(d_packet);
				}
			} else {
				break;
			}
		}
	}

	void server_to_client() {
		for (;;) {
			if (artemis_packet::fetch_artemis_packet(from_server,
					[this](uint32_t read_len) {
						from_server.realloc_if_needed(read_len);
						std::error_code err;
						from_server.write_offset += to_server.sock.socket.read_some(std::experimental::net::buffer(from_server.buffer.data() + from_server.write_offset, read_len), err);
						if (!!err&&err != std::experimental::net::error::would_block) {
							std::cout << "proxy to server read fail\n";
							to_server.connected = false;
							return; // should this be an exception?
						}
			} )) {
				std::deque<std::byte> tmp_buffer{from_server.buffer.begin(),from_server.buffer.begin()+from_server.write_offset};
				enqueue_client_write(tmp_buffer);
				from_server=packet_buffer();
			} else {
				return;
			}
		}
	}

	void parse_c2s_packet(packet_buffer* buffer) {
		buffer->read_offset=20;
		const auto packet_id_opt=buffer->attempt_read<uint32_t>();
		if (!packet_id_opt) {
			return;
		}
		const auto packet_id=*packet_id_opt;
		if (packet_id==artemis_packet::client_to_server::value_int_jam32) {
			const auto id_opt=buffer->attempt_read<uint32_t>();
			if (!id_opt) {
				return;
			}
			const auto id=*id_opt;
			if (id==static_cast<uint32_t>(artemis_packet::value_int::ready)) {
				transmitted_to_client.clear_all_data();
			}
		}
	}

	void parse_s2c_packet(const std::deque<std::byte>& buffer) {
		auto server_info=&cached_hermes.server_info;
		if (buffer.size()<24) {
			return;
		}
		const auto packet_id{buffer::read_at<uint32_t>(buffer,20)};
		if (buffer.size()==39 && packet_id==artemis_packet::server_to_client::client_consoles_jam32) {
			std::deque<std::byte> data{buffer.begin()+24,buffer.end()};
			if (!suppress_next_ship_num_packet) {
				ship_num=buffer::pop_front<uint32_t>(data);
				for (auto i = 0l; i != 11; i++) {
					consoles[i]=buffer::pop_front<uint8_t>(data);
				}
			}
			suppress_next_ship_num_packet=false;
		}
		if (packet_id==artemis_packet::server_to_client::object_delete_jam32) {
			std::deque<std::byte> data{buffer.begin()+24,buffer.end()};
			const auto obj_type{buffer::pop_front<uint8_t>(data)};
			const auto id{buffer::pop_front<uint32_t>(data)};
			if (obj_type==1) { // 1==player ship, this should be an
				transmitted_to_client.delete_player_data(id);
				server_info->server_data.delete_player_data(id);
			} else if (obj_type==5) {
				transmitted_to_client.delete_npc_data(id);
				server_info->server_data.delete_npc_data(id);
			}
		}
		if (packet_id==artemis_packet::server_to_client::simple_event_jam32) {
			std::deque<std::byte> data{buffer.begin()+24,buffer.end()};
			const auto sub_id{buffer::pop_front<artemis_packet::simple_event>(data)};
			if (sub_id==artemis_packet::simple_event::game_over) {
				server_info->server_data.clear_all_data();
				transmitted_to_client.clear_all_data();
			}
			if (sub_id==artemis_packet::simple_event::end_game || sub_id==artemis_packet::simple_event::game_over) {
				transmitted_to_client.clear_all_data();
				server_info->server_data.clear_all_data();
			}
		}
		if (packet_id==artemis_packet::server_to_client::object_bit_stream_jam32) {
			std::deque<std::byte> data{buffer.begin()+24,buffer.end()};
			for (;;) {
				const auto type=buffer::peek_front<uint8_t>(data);
				if (type==1) { //pc data, this really should be moved to a constant somewhere
					update_player_data update(&data);
					if (update.visual_id) {
						if (!transmitted_to_client.pc_data.count(update.id) || (transmitted_to_client.pc_data.at(update.id).hermes_visual_id != *update.visual_id)) {
							hull_id_change_time.emplace(update.id,std::chrono::system_clock::now());
						}
					}
					server_info->server_data.update_player(update);
					transmitted_to_client.update_player(update);
				} else if (type==2) { // weapons data - this really should become a constant somewhere
					update_weapons_data update(&data);
					server_info->server_data.update_wep(update);
					transmitted_to_client.update_wep(update);
				} else if (type==3) { //engineering data, this really should be moved to a constant somewhere
					update_engineering_data update(&data);
					server_info->server_data.update_eng(update);
					transmitted_to_client.update_eng(update);
				} else if (type==5) { //npc data, this really should be moved to a constant somewhere
					update_npc_data update(&data);
					if (update.visual_id) {
						if (!transmitted_to_client.npc_data.count(update.id) || (transmitted_to_client.npc_data.at(update.id).hermes_visual_id != *update.visual_id)) {
							hull_id_change_time.emplace(update.id,std::chrono::system_clock::now());
						}
					}
					server_info->server_data.update_npc(update);
					transmitted_to_client.update_npc(update);
				} else {
					break;
				}
			}
		}
	}
};
