#include "gtest/gtest.h"
#include "packet.test.h"

class artemis_system_sync_test :public artemis_packet_test, public artemis_packet::server_to_client {
protected:
	std::vector<uint32_t> damcon_nx{1,2,3};
	std::vector<uint32_t> damcon_ny{11,12,13};
	std::vector<uint32_t> damcon_nz{21,22,23};
	std::vector<uint32_t> damcon_x{31,32,33};
	std::vector<uint32_t> damcon_y{41,42,43};
	std::vector<uint32_t> damcon_z{51,52,53};
	std::vector<float> damcon_progress{61,62,63};
	std::vector<uint32_t> damcon_members{71,72,73};
};

TEST_F (artemis_packet_test,make_convert_torpedo) {
	buffer=artemis_packet::client_to_server::make_convert_torpedo(1.0f);
	check_value_four_int_header(artemis_packet::value_four_ints::convert_torpedo);
	EXPECT_EQ(buffer.read<float>(),1.0f);
	EXPECT_EQ(buffer.read<uint32_t>(),0);
	EXPECT_EQ(buffer.read<uint32_t>(),0);
	EXPECT_EQ(buffer.read<uint32_t>(),0);
}
TEST_F (artemis_packet_test,make_load_tube) {
	buffer=artemis_packet::client_to_server::make_load_tube(4,5);
	check_value_four_int_header(artemis_packet::value_four_ints::load_tube);
	EXPECT_EQ(buffer.read<uint32_t>(),4);
	EXPECT_EQ(buffer.read<uint32_t>(),5);
	EXPECT_EQ(buffer.read<uint32_t>(),0);
	EXPECT_EQ(buffer.read<uint32_t>(),0);
}

TEST_F (artemis_packet_test,object_bitstream) {
	std::deque<std::byte> tmp{std::byte{1}};
	std::vector<std::deque<std::byte>> tmp2{tmp};
	auto buffer=artemis_packet::server_to_client::finalize_object_bitstreams(std::move(tmp2)).at(0);
	EXPECT_EQ(buffer.size(),29u);
	check_artemis_header(buffer,artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,20),artemis_packet::server_to_client::object_bit_stream_jam32);
	EXPECT_EQ(buffer::read_at<uint8_t>(buffer,24),1);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,25),0);
}

TEST_F (artemis_packet_test,make_climb_dive) {
	buffer=artemis_packet::client_to_server::make_climb_dive(5);
	check_value_int_header(artemis_packet::value_int::climb_dive);
	EXPECT_EQ(buffer.read<int32_t>(),5);
}

TEST_F (artemis_packet_test,comm_text) {
	const auto buffer{artemis_packet::server_to_client::make_comm_text(10,"a","b")};
	EXPECT_EQ(artemis_packet::server_to_client::comm_text_jam32,0xd672c35f);
	EXPECT_EQ(buffer.size(),42);

	check_artemis_header(buffer,artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,20),artemis_packet::server_to_client::comm_text_jam32);
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,24),10);//filters
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,26),2u);
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,30),'a');
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,32),0u);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,34),2u);
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,38),'b');
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,40),0u);
}

TEST_F (artemis_packet_test,connected) {
	buffer=artemis_packet::server_to_client::make_connected(1,2,3,4,5);
	EXPECT_EQ(artemis_packet::server_to_client::connected_jam32,0xe548e74a);
	check_artemis_header(artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::connected_jam32);
	EXPECT_EQ(buffer.read<uint32_t>(),1u);
	EXPECT_EQ(buffer.read<float>(),2.0f);
	EXPECT_EQ(buffer.read<uint32_t>(),3u);
	EXPECT_EQ(buffer.read<uint32_t>(),4u);
	EXPECT_EQ(buffer.read<uint32_t>(),5u);
}

TEST_F (artemis_packet_test,idle_test) {
	const auto buffer=artemis_packet::server_to_client::make_idle_text("a","b");
	EXPECT_EQ(buffer.size(),40);
	EXPECT_EQ(artemis_packet::server_to_client::idle_text_jam32,0xbe991309);
	check_artemis_header(buffer,artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,20),artemis_packet::server_to_client::idle_text_jam32);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,24),2u);
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,28),'a');
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,30),0u);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,32),2u);
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,36),'b');
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,38),0u);
}

TEST_F (artemis_packet_test,object_delete) {
	buffer=artemis_packet::server_to_client::make_object_delete(1,2);
	EXPECT_EQ(artemis_packet::server_to_client::object_delete_jam32,0xcc5a3e30);
	check_artemis_header(artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::object_delete_jam32);
	EXPECT_EQ(buffer.read<uint8_t>(),1);
	EXPECT_EQ(buffer.read<uint32_t>(),2);
}

TEST_F (artemis_packet_test,object_test) {
	buffer=artemis_packet::server_to_client::make_object_text(10,11,"a");
	EXPECT_EQ(artemis_packet::server_to_client::object_text_jam32,0xee665279);
	check_artemis_header(artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::object_text_jam32);
	EXPECT_EQ(buffer.read<uint32_t>(),10);
	EXPECT_EQ(buffer.read<uint8_t>(),11);
	EXPECT_EQ(buffer.read<uint32_t>(),2u);
	EXPECT_EQ(buffer.read<uint16_t>(),'a');
	EXPECT_EQ(buffer.read<uint16_t>(),0u);
}

TEST_F (artemis_packet_test,perspective) {
	buffer=artemis_packet::server_to_client::make_perspective();
	check_simple_event_header(artemis_packet::simple_event::perspective);
	EXPECT_EQ(buffer.read<uint32_t>(),0u);
}

TEST_F (artemis_packet_test,docked) {
	buffer=artemis_packet::server_to_client::make_docked(10);
	check_simple_event_header(artemis_packet::simple_event::docked);
	EXPECT_EQ(buffer.read<uint32_t>(),10u);
}

TEST_F (artemis_packet_test,popup) {
	const auto buffer=artemis_packet::server_to_client::make_popup("a");
	EXPECT_EQ(buffer.size(),36);
	check_simple_event_header(buffer,artemis_packet::simple_event::popup);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,28),2u);
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,32),'a');
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,34),0u);
}

TEST_F (artemis_packet_test,game_start) {
	buffer=artemis_packet::server_to_client::start_game(10,11);
	EXPECT_EQ(artemis_packet::server_to_client::start_game_jam32,0x3de66711);
	check_artemis_header(artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::start_game_jam32);
	EXPECT_EQ(buffer.read<uint32_t>(),10u);
	EXPECT_EQ(buffer.read<uint32_t>(),11u);
}

TEST_F (artemis_packet_test,client_consoles) {
	const auto buffer=artemis_packet::server_to_client::make_client_consoles(12,std::array<uint8_t,11>{0,1,2,3,4,5,6,7,8,9,10});
	check_client_console(buffer,12,0,1,2,3,4,5,6,7,8,9,10);
}

TEST_F (artemis_packet_test,plain_text_greeting) {
	buffer=artemis_packet::server_to_client::make_plain_text_greeting("test");
	EXPECT_EQ(artemis_packet::server_to_client::plain_text_greeting_jam32,0x6d04b3da);
	check_artemis_header(artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::plain_text_greeting_jam32);
	EXPECT_EQ(buffer.read<uint32_t>(),4);
	EXPECT_EQ(buffer.read<std::string>(),"test");
}

TEST_F (artemis_packet_test,ship_settings) {
	buffer=artemis_packet::client_to_server::make_ship_settings(artemis_packet::ship_settings{false,5,0.5f,"a"});
	check_value_int_header(artemis_packet::value_int::ship_settings);
	artemis_packet::ship_settings ship1{buffer};
	EXPECT_EQ(ship1.jump,false);
	EXPECT_EQ(ship1.id,5);
	EXPECT_EQ(ship1.color,0.5f);
	EXPECT_EQ(!!ship1.name,true);
	std::u16string u16ship1name;
	u16ship1name+='a';
	EXPECT_EQ(*ship1.name,u16ship1name);
}

TEST_F (artemis_packet_test,key_capture) {
	buffer=artemis_packet::server_to_client::make_key_capture(1);
	check_simple_event_header(artemis_packet::simple_event::key_capture);
	EXPECT_EQ(buffer.read<uint8_t>(),1);
}

TEST_F (artemis_packet_test,make_all_ship_settings) {
	buffer=artemis_packet::server_to_client::make_all_ship_settings(
		artemis_packet::ship_settings{false,0,0.0f,"a"},
		artemis_packet::ship_settings{true,1,0.1f,"test"},
		artemis_packet::ship_settings{false,2,0.2f},
		artemis_packet::ship_settings{true,3,0.3f},
		artemis_packet::ship_settings{false,4,0.4f},
		artemis_packet::ship_settings{true,5,0.5f},
		artemis_packet::ship_settings{false,6,0.6f},
		artemis_packet::ship_settings{true,7,0.7f});
	check_simple_event_header(artemis_packet::simple_event::all_ship_settings);
	//we could make a struct and compare vs that but this makes it easier to confirm that the code is correct rather than just self consistant
	artemis_packet::ship_settings ship1{buffer};
	artemis_packet::ship_settings ship2{buffer};
	artemis_packet::ship_settings ship3{buffer};
	artemis_packet::ship_settings ship4{buffer};
	artemis_packet::ship_settings ship5{buffer};
	artemis_packet::ship_settings ship6{buffer};
	artemis_packet::ship_settings ship7{buffer};
	artemis_packet::ship_settings ship8{buffer};
	EXPECT_EQ(ship1.jump,false);
	EXPECT_EQ(ship1.id,0);
	EXPECT_EQ(ship1.color,0);
	EXPECT_EQ(!!ship1.name,true);
	std::u16string u16ship1name;
	u16ship1name+='a';
	EXPECT_EQ(*ship1.name,u16ship1name);

	EXPECT_EQ(ship2.jump,true);
	EXPECT_EQ(ship2.id,1);
	EXPECT_EQ(ship2.color,0.1f);
	EXPECT_EQ(!!ship2.name,true);
	std::u16string u16ship2name;
	u16ship2name+='t';
	u16ship2name+='e';
	u16ship2name+='s';
	u16ship2name+='t';
	EXPECT_EQ(*ship2.name,u16ship2name);

	EXPECT_EQ(ship3.jump,false);
	EXPECT_EQ(ship3.id,2);
	EXPECT_EQ(ship3.color,0.2f);
	EXPECT_EQ(!!ship3.name,false);

	EXPECT_EQ(ship4.jump,true);
	EXPECT_EQ(ship4.id,3);
	EXPECT_EQ(ship4.color,0.3f);
	EXPECT_EQ(!!ship4.name,false);

	EXPECT_EQ(ship5.jump,false);
	EXPECT_EQ(ship5.id,4);
	EXPECT_EQ(ship5.color,0.4f);
	EXPECT_EQ(!!ship5.name,false);

	EXPECT_EQ(ship6.jump,true);
	EXPECT_EQ(ship6.id,5);
	EXPECT_EQ(ship6.color,0.5f);
	EXPECT_EQ(!!ship6.name,false);

	EXPECT_EQ(ship7.jump,false);
	EXPECT_EQ(ship7.id,6);
	EXPECT_EQ(ship7.color,0.6f);
	EXPECT_EQ(!!ship7.name,false);

	EXPECT_EQ(ship8.jump,true);
	EXPECT_EQ(ship8.id,7);
	EXPECT_EQ(ship8.color,0.7f);
	EXPECT_EQ(!!ship8.name,false);
}

TEST_F (artemis_packet_test,constant_test) {
	EXPECT_EQ(0x4c821d3c,artemis_packet::client_to_server::value_int_jam32);
	EXPECT_EQ(0xf5821226,artemis_packet::server_to_client::heartbeat_jam32);
}

TEST_F (artemis_packet_test,make_set_console) {
	buffer=artemis_packet::client_to_server::make_set_console(6,true);
	check_value_int_header(artemis_packet::value_int::set_console);
	EXPECT_EQ(buffer.read<uint32_t>(),6u);
	EXPECT_EQ(buffer.read<uint32_t>(),1u);
}

TEST_F (artemis_packet_test,make_set_ship_packet) {
	buffer=artemis_packet::client_to_server::make_set_ship_packet(5);
	check_value_int_header(artemis_packet::value_int::set_ship);
	EXPECT_EQ(buffer.read<uint32_t>(),5u);
}

TEST_F (artemis_packet_test,make_heartbeat) {
	buffer=artemis_packet::server_to_client::make_heartbeat();
	check_artemis_header(artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::heartbeat_jam32);
}

TEST_F (artemis_packet_test,make_request_eng_grid_update) {
	buffer=artemis_packet::client_to_server::make_request_eng_grid_update();
	check_value_int_header(artemis_packet::value_int::request_eng_grid_update);
	EXPECT_EQ(buffer.read<uint32_t>(),0);
}

TEST_F (artemis_packet_test,make_explosion) {
	buffer=artemis_packet::server_to_client::make_explosion(1,2);
	check_simple_event_header(artemis_packet::simple_event::explosion);
	EXPECT_EQ(buffer.read<uint32_t>(),1u);
	EXPECT_EQ(buffer.read<uint32_t>(),2u);
}

TEST_F (artemis_packet_test,make_detonation) {
	buffer=artemis_packet::server_to_client::make_detonation(1,2);
	check_simple_event_header(artemis_packet::simple_event::detonation);
	EXPECT_EQ(buffer.read<uint32_t>(),1u);
	EXPECT_EQ(buffer.read<uint32_t>(),2u);
}

TEST_F (artemis_packet_test,make_player_damage) {
	buffer=artemis_packet::server_to_client::make_player_ship_damage(1,2.0f);
	check_simple_event_header(artemis_packet::simple_event::player_ship_damage);
	EXPECT_EQ(buffer.read<uint32_t>(),1u);
	EXPECT_EQ(buffer.read<float>(),2.0f);
}

TEST_F (artemis_packet_test,make_attack) {
	buffer=artemis_packet::server_to_client::make_attack(1,2,3,4,5,6,7.0f,8.0f,9.0f,10);
	check_artemis_header(artemis_packet::direction::server_to_client);
	EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::attack_jam32);
	EXPECT_EQ(buffer.read<uint32_t>(),1u);
	EXPECT_EQ(buffer.read<uint32_t>(),9u);//beam subtype
	EXPECT_EQ(buffer.read<uint32_t>(),0u);//beam port index?
	EXPECT_EQ(buffer.read<uint32_t>(),2u);
	EXPECT_EQ(buffer.read<uint32_t>(),3u);
	EXPECT_EQ(buffer.read<uint32_t>(),4u);
	EXPECT_EQ(buffer.read<uint32_t>(),0u);
	EXPECT_EQ(buffer.read<uint32_t>(),5u);
	EXPECT_EQ(buffer.read<uint32_t>(),6u);
	EXPECT_EQ(buffer.read<float>(),7.0f);
	EXPECT_EQ(buffer.read<float>(),8.0f);
	EXPECT_EQ(buffer.read<float>(),9.0f);
	EXPECT_EQ(buffer.read<uint32_t>(),10u);
}
