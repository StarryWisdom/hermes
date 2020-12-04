#pragma once

#include <stdint.h>
#include <string>
#include <optional>
#include <locale>
#include <deque>

#include "core-lib/packet_buffer.h"
#include "jam32.h"
#include "core-lib/string-util.h"
#include "core-lib/memory_buffer.h"

class artemis_packet {
public:
	inline static const uint32_t header_size{24};
	enum class direction {
		server_to_client=1,
		client_to_server=2
	};
	
	enum class value_four_ints {
		set_coolant=0x0,
		//unknown
		load_tube=0x2,
		convert_torpedo=0x3,
		send_damcon=0x4
	};

	enum class value_float {
		//HelmSetImpulsePacket 0x00
		//HelmSetSteeringPacket 0x01
		//HelmSetPitchPacket 0x02
		//unknown 0x03
		//EngSetEnergyPacket 0x04
		//HelmJumpPacket 0x05
		//GameMasterSelectLocationPacket 0x06
		//FighterPilotPacket 0x07
	};

	enum class value_int {
		warp=0x0,
		mainscreen=0x1,
		weapons_target=0x02,
		toggle_auto_beam=0x03,
		toggle_shields=0x04,
		shields_up=0x05,
		shields_down=0x06,
		request_dock=0x07,
		fire_tube=0x08,
		unload_tube=0x9,
		toggle_red_alert=0xa,
		beam_freq=0xb,
		auto_damcon=0xc,
		set_ship=0xd,
		set_console=0xe,
		ready=0xf,
		sci_target=0x10,
		capt_target=0x11,
		gm_target=0x12,
		sci_scan=0x13,
		//KeystrokePacket 0x14
		//ButtonClickPacket 0x15
		unknown=0x16,
		ship_settings=0x17,
		reset_coolant=0x18,
		toggle_reverse=0x19,
		request_eng_grid_update=0x1a,
		toggle_perspective=0x1b,
		//ActivateUpgradePacket 0x1c
		climb_dive=0x1d,
		//FighterLaunchPacket 0x1e
		//FighterShootPacket 0x1f
		//EmergencyJumpPacket 0x20
		fighter_settings=0x21,
		//BeaconConfigPacket 0x22
	};

	enum class simple_event {
		explosion=0x0,
		//simple event 0x01 - red alert
		//0x02 unknown
		sound_effect=0x03,
		//0x04 pause
		player_ship_damage=0x5,
		end_game=0x6,
		//0x07 green thing decloak?
		//0x08 related to sound effect?
		//0x09 skybox
		popup=0xa,
		//0x0b auto damcons
		//0x0c jump start
		//0x0d jump end
		//0x0e unknown
		all_ship_settings=0xf,
		//0x10 dmx
		key_capture=0x11,
		perspective=0x12,
		detonation=0x13,
		//0x14 game over reason
		//0x15 game over stats
		//0x16 unknown
		//0x17 fighter launch
		//0x18 single seat damage shake
		//0x19 biomech hostile
		docked=0x1a,
		//0x1b smoke puff
		//0x1c fighter text
		//0x1da tagged
		game_over=0x1e,
	};

	class ship_settings {
	public:
		bool jump{false};
		uint32_t id{0};
		float color{0};
		std::optional<std::u16string> name;

		ship_settings(bool jump, uint32_t id, float color, std::string u8name)
			: jump(jump)
			, id(id)
			, color(color)
		{
			name=string_util::get_u16_string(u8name);
		}

		ship_settings(bool jump, uint32_t id, float color, std::optional<std::u16string> name=std::optional<std::u16string>{})
			: jump(jump)
			, id(id)
			, color(color)
			, name(name)
		{}

		ship_settings(packet_buffer& buffer) {
			jump=(buffer.read<uint32_t>()==1);
			id=buffer.read<uint32_t>();
			color=buffer.read<float>();
			if (buffer.read<uint32_t>()!=0) {
				name=buffer.read_artemis_string();
			}
		}

		packet_buffer serialise() const {
			packet_buffer buffer;
			buffer.write<uint32_t>(jump==true);
			buffer.write<uint32_t>(id);
			buffer.write<float>(color);
			buffer.write<uint32_t>(!!name);
			if (!!name) {
				buffer.write_artemis_string(*name);
			}
			return buffer;
		}
	};

	class server_to_client {
	public:
		inline static const uint32_t heartbeat_jam32{jam32::value("heartbeat")};
		inline static const uint32_t simple_event_jam32{jam32::value("simpleEvent")};
		inline static const uint32_t attack_jam32{jam32::value("attack")};
		inline static const uint32_t start_game_jam32{jam32::value("startGame")};
		inline static const uint32_t plain_text_greeting_jam32{jam32::value("plainTextGreeting")};
		inline static const uint32_t client_consoles_jam32{jam32::value("clientConsoles")};
		inline static const uint32_t idle_text_jam32{jam32::value("idleText")};
		inline static const uint32_t object_text_jam32{jam32::value("objectText")};
		inline static const uint32_t object_delete_jam32{jam32::value("objectDelete")};
		inline static const uint32_t connected_jam32{jam32::value("connected")};
		inline static const uint32_t comm_text_jam32{jam32::value("commText")};
		inline static const uint32_t object_bit_stream_jam32{jam32::value("objectBitStream")};
		inline static const uint32_t ship_system_sync_jam32{jam32::value("shipSystemSync")};
		//TitlePacket 0x902f0b1a bigMess
		//FighterBayStatusPacket 0x9ad1f23b carrierRecord
		//IncomingAudioPacket 0xae88e058 incomingMessage
		//CommsButtonPacket 0xca88f050 commsButton

		//gm buttons
		//RemoveGMButtonPacket 0x00
		//AddGMButtonPacket server 0x01
		//AddGMButtonAtPositionPacket 0x02
		//SetDefaultGMButtonWidthPacket 0x03
		//GameMasterInstructionsPacket 0x63
		//RemoveAllGMButtonsPacket 0x64
		//comms buttons
		//commsButton 0x00  Remove
		//commsButton 0x02  Add
		//commsButton 0x64  Remove all
	public:
		static std::vector<std::deque<std::byte>> finalize_object_bitstreams(std::vector<std::deque<std::byte>>&& bitstreams) {
			for (auto& i : bitstreams) {
				buffer::push_back<uint32_t>(i,0);//termination quad
				add_artemis_header(i, object_bit_stream_jam32);
			}
			return bitstreams;
		}

		static std::deque<std::byte> make_comm_text(uint16_t filter, std::string sender, std::string message) {
			std::deque<std::byte> buffer;
			buffer::push_back<uint16_t>(buffer,filter);
			buffer::write_artemis_string(buffer,sender);
			buffer::write_artemis_string(buffer,message);

			add_artemis_header(buffer,comm_text_jam32);
			return buffer;
		}

		static packet_buffer make_connected(uint32_t unknown, float old_version, uint32_t major, uint32_t minor, uint32_t patch) {
			return make_buffer(connected_jam32,[=](packet_buffer& buffer) {
				buffer.write<uint32_t>(unknown);
				buffer.write<float> (old_version);
				buffer.write<uint32_t>(major);
				buffer.write<uint32_t>(minor);
				buffer.write<uint32_t>(patch);
			});
		}

		static packet_buffer make_object_delete(uint8_t type, uint32_t id) {
			return make_buffer(object_delete_jam32,[=](packet_buffer& buffer) {
				buffer.write<uint8_t>(type);
				buffer.write<uint32_t>(id);
			});
		}

		static packet_buffer make_object_text(uint32_t obj, uint8_t type, std::string text) {
			return make_buffer(object_text_jam32,[=](packet_buffer& buffer) {
				buffer.write<uint32_t>(obj);
				buffer.write<uint8_t>(type);
				buffer.write_artemis_string(text);
			});
		}

		static std::deque<std::byte> make_idle_text(std::string from, std::string text) {
			std::deque<std::byte> buffer;
			buffer::write_artemis_string(buffer,from);
			buffer::write_artemis_string(buffer,text);

			add_artemis_header(buffer,idle_text_jam32);
			return buffer;
		}

		static std::deque<std::byte> make_client_consoles(uint32_t ship, const std::array<uint8_t,11> consoles) {
			std::deque<std::byte> buffer;
			buffer::push_back<uint32_t>(buffer,ship);
			for (auto i : consoles) {
				buffer::push_back<uint8_t>(buffer,i);
			}
			add_artemis_header(buffer,client_consoles_jam32);
			return buffer;
		}

		static packet_buffer make_heartbeat() {
			return make_buffer(heartbeat_jam32,[](packet_buffer&) {});
		}

		static packet_buffer make_explosion(uint32_t type, uint32_t id) {
			return make_simple_event(simple_event::explosion,[=](packet_buffer& buffer) {
				buffer.write(type);
				buffer.write(id);
			});
		}

		static packet_buffer make_detonation(uint32_t type, uint32_t id) {
			return make_simple_event(simple_event::detonation,[=](packet_buffer& buffer) {
				buffer.write(type);
				buffer.write(id);
			});
		}

		static packet_buffer make_all_ship_settings(const artemis_packet::ship_settings set1, const artemis_packet::ship_settings set2, const artemis_packet::ship_settings set3, const artemis_packet::ship_settings set4, const artemis_packet::ship_settings set5, const artemis_packet::ship_settings set6, const artemis_packet::ship_settings set7, const artemis_packet::ship_settings set8) {
			return make_simple_event(simple_event::all_ship_settings,[=](packet_buffer& buffer) {
				buffer.write<packet_buffer>(set1.serialise());
				buffer.write<packet_buffer>(set2.serialise());
				buffer.write<packet_buffer>(set3.serialise());
				buffer.write<packet_buffer>(set4.serialise());
				buffer.write<packet_buffer>(set5.serialise());
				buffer.write<packet_buffer>(set6.serialise());
				buffer.write<packet_buffer>(set7.serialise());
				buffer.write<packet_buffer>(set8.serialise());
			});
		}

		static packet_buffer make_attack(uint32_t id, uint32_t port, uint32_t origin_type, uint32_t target_type ,uint32_t origin ,uint32_t target, float x,float y,float z,uint32_t manual) {
			return make_buffer(attack_jam32,[=](packet_buffer& buffer) {
				buffer.write<uint32_t>(id);
				buffer.write<uint32_t>(9);//type
				buffer.write<uint32_t>(0);//beam port - FIX WRONG
				buffer.write<uint32_t>(port);
				buffer.write<uint32_t>(origin_type);
				buffer.write<uint32_t>(target_type);
				buffer.write<uint32_t>(0);
				buffer.write<uint32_t>(origin);
				buffer.write<uint32_t>(target);
				buffer.write<float>(x);
				buffer.write<float>(y);
				buffer.write<float>(z);
				buffer.write<uint32_t>(manual);
			});
		};

		static packet_buffer make_key_capture(uint8_t capture) {
			return make_simple_event(simple_event::key_capture,[=](packet_buffer& buffer) {
				buffer.write<uint8_t>(capture);
			});
		}

		static packet_buffer make_plain_text_greeting(std::string msg) {
			return make_buffer(plain_text_greeting_jam32,[=](packet_buffer& buffer) {
				buffer.write<uint32_t>(msg.size());//bleh this is off by one according to nicks parser, I'm less than certian is null terminated correctly
				buffer.write<std::string>(msg);
			});
		}

		static packet_buffer make_player_ship_damage(uint32_t index, float duration) {
			return make_simple_event(simple_event::player_ship_damage,[=](packet_buffer& buffer) {
				buffer.write<uint32_t>(index);
				buffer.write<float>(duration);
			});
		};

		static std::deque<std::byte> make_popup(std::string msg) {
			std::deque<std::byte> buffer;
			buffer::write_artemis_string(buffer,msg);

			add_simple_event_header(buffer,simple_event::popup);
			return buffer;
		}

		static packet_buffer make_docked(uint32_t id) {
			return make_simple_event(simple_event::docked,[=](packet_buffer& buffer) {
				buffer.write<uint32_t>(id);
			});
		}

		static packet_buffer start_game(uint32_t difficulty, uint32_t mode) {
			return make_buffer(start_game_jam32,[=](packet_buffer& buffer) {
					buffer.write(difficulty);
					buffer.write(mode);
				});
		}

		static packet_buffer make_perspective() {
			return make_simple_event(simple_event::perspective,[](packet_buffer& buffer) {
					buffer.write<uint32_t>(0);
			});
		}

		static packet_buffer make_sound_effect(const std::string& file) {
			return make_simple_event(simple_event::sound_effect,[=](packet_buffer& buffer) {
					buffer.write_artemis_string(file);
			});
		}
	private:
		static void add_simple_event_header(std::deque<std::byte>& buffer, const simple_event subtype) {
			buffer::push_front(buffer,static_cast<uint32_t>(subtype));
			add_artemis_header(buffer,simple_event_jam32);
		}

		static void add_artemis_header(std::deque<std::byte>& buffer, uint32_t id) {
			artemis_packet::add_artemis_header(buffer,direction::server_to_client,id);

		}

		template <typename fun> static packet_buffer make_simple_event(simple_event subtype, fun f) [[deprecated]] {
			return make_buffer(simple_event_jam32,[=](packet_buffer& buffer) {
					buffer.write<uint32_t>(static_cast<uint32_t>(subtype));
					f(buffer);
				});
		}
		template <typename fun> static packet_buffer make_buffer(uint32_t id, fun f) [[deprecated]] {
			return artemis_packet::make_buffer(direction::server_to_client,id,f);
		}
	};

	class client_to_server {
	public:
		enum class comm_target {
			player=0,
			enemy=1,
			station=2,
			other=3
		};

		// this has been moved here from hermes
		// this probably wants to be fixed up at some point
		// gm_text_hermes_temp needs renaming when fixed
		/*
		class gm_text_hermes_temp {
		public:
			gm_text_hermes_temp(packet_buffer& buffer) {
				for (;;) {
					uint32_t id(buffer.read<uint32_t>());
					if (id==uint32_t(0)) {
						break;
					}
					recipients.push_back(id);
				}
				presentation=buffer.read<uint32_t>();
				uint32_t sender_len=buffer.read<uint32_t>();
				bool found_null{false};//artemis strings tend to have nulls in random places thus we have this to prevent a non null terminated string
				for (;;) {//this should be a read for sender but ... eh
					uint16_t c=buffer.read<uint16_t>();
					sender_len--;
					if (c==0) {
						found_null=true;
					}
					if (sender_len==0) {
						if (!found_null) {
							throw std::runtime_error("invalid sender name in gm text");
						}
						break;
					}
				}

				found_null=true;
				uint32_t message_len=buffer.read<uint32_t>();
				for (;;) {//fancy way to read a string, not ideal
					uint16_t c=buffer.read<uint16_t>();
					message_len--;
					if (c==0) {
						found_null=true;
					}
					if (message_len==0) {
						if (!found_null) {
							throw std::runtime_error("invalid message in gm text");
						}
						break;
					}
					temp_message+=c;
				}

				if (buffer.read_offset!=buffer.write_offset) {
					throw std::runtime_error("incorrect packet length for gm_text");
				}
			}

			//packet type accoring to protocol docs are
			std::vector<uint32_t> recipients;
			uint32_t presentation;
			std::u16string sender;// we should read these but currently dont
//			std::u16string message;// when we have a proper artemis string type we should use this not the temp_message below
			std::string temp_message;
			uint16_t fliter;
		};
*/
		inline static const uint32_t value_int_jam32{jam32::value("valueInt")};
		inline static const uint32_t value_four_ints_jam32{jam32::value("valueFourInts")};
		inline static const uint32_t comms_message_jam32{jam32::value("commsMessage")};
		//AudioCommandPacket 0x6aadc57f controlMessage
		inline static const uint32_t gm_text_jam32{jam32::value("gmText")};
		inline static const uint32_t beam_request_jam32{jam32::value("beamRequest")};

		static packet_buffer make_convert_torpedo(const float direction) {
			return make_value_four_int(value_four_ints::convert_torpedo,[=](packet_buffer& buffer) {
				buffer.write<float>(direction);
				buffer.write<uint32_t>(0);
				buffer.write<uint32_t>(0);
				buffer.write<uint32_t>(0);
			});
		}

		static packet_buffer make_load_tube(const uint32_t tube_num, const uint32_t type) {
			return make_value_four_int(value_four_ints::load_tube,[=](packet_buffer& buffer) {
				buffer.write<uint32_t>(tube_num);
				buffer.write<uint32_t>(type);
				buffer.write<uint32_t>(0);
				buffer.write<uint32_t>(0);
			});
		}

		static packet_buffer make_ship_settings(const ship_settings settings) {
			return make_value_int(value_int::ship_settings,[=](packet_buffer& buffer) {
					buffer.write<packet_buffer>(settings.serialise());
				});
		}

		static packet_buffer make_set_console(uint32_t console, bool taken) {
			return make_value_int(value_int::set_console,[=](packet_buffer& buffer){
					buffer.write<uint32_t>(console);
					buffer.write<uint32_t>(taken==true);
				});
		}

		static packet_buffer make_set_ship_packet(uint32_t num) {
			return make_value_int(value_int::set_ship,[=](packet_buffer& buffer){
					buffer.write<uint32_t>(num);
				});
		}

		static packet_buffer make_request_eng_grid_update() {
			return make_value_int(value_int::request_eng_grid_update,[=](packet_buffer& buffer) {
					buffer.write<uint32_t>(0);
				});
		}

		static packet_buffer make_climb_dive(int32_t val) {
			return make_value_int(value_int::climb_dive,[=](packet_buffer& buffer) {
					buffer.write<int32_t>(val);
				});
		}

	private:
		template <typename func> static packet_buffer make_value_int (value_int subtype, func f) {
			return make_buffer(value_int_jam32,[=](packet_buffer& buffer) {
					buffer.write<uint32_t>(static_cast<uint32_t>(subtype));
					f(buffer);
				});
		}

		template <typename func> static packet_buffer make_value_four_int (value_four_ints subtype, func f) {
			return make_buffer(value_four_ints_jam32,[=](packet_buffer& buffer) {
					buffer.write<uint32_t>(static_cast<uint32_t>(subtype));
					f(buffer);
			});
		}

		template <typename fun> static packet_buffer make_buffer(uint32_t id, fun f) {
			return artemis_packet::make_buffer(direction::client_to_server,id,f);
		}
	};
private:
	template <typename func> static packet_buffer make_buffer(direction direction, uint32_t id, func f) [[deprecated]]{
		packet_buffer buffer;
		buffer.realloc_if_needed(header_size);
		buffer.write_offset=header_size;

		f(buffer);

		auto size_offset=buffer.write_offset;
		buffer.write_offset=0;
		buffer.write<uint32_t>(0xdeadbeef);
		buffer.write<uint32_t>(size_offset);//size 1
		buffer.write<uint32_t>(static_cast<uint32_t>(direction));//origin
		buffer.write<uint32_t>(0);//padding - must be 0
		buffer.write<uint32_t>(size_offset-20);//size 2
		buffer.write<uint32_t>(id);//packet type
		buffer.write_offset=size_offset;
		return buffer;
	}

	static void add_artemis_header(std::deque<std::byte>& buffer, direction direction, uint32_t id)
	{
		buffer::push_front<uint32_t>(buffer,id);
		buffer::push_front<uint32_t>(buffer,buffer.size());
		buffer::push_front<uint32_t>(buffer,0);
		buffer::push_front<uint32_t>(buffer,static_cast<uint32_t>(direction));
		buffer::push_front<uint32_t>(buffer,buffer.size()+8);
		buffer::push_front<uint32_t>(buffer,0xdeadbeef);
	}
};
