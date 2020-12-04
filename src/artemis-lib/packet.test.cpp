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

TEST_F (artemis_packet_test,popup) {
	const auto buffer=artemis_packet::server_to_client::make_popup("a");
	EXPECT_EQ(buffer.size(),36);
	check_simple_event_header(buffer,artemis_packet::simple_event::popup);
	EXPECT_EQ(buffer::read_at<uint32_t>(buffer,28),2u);
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,32),'a');
	EXPECT_EQ(buffer::read_at<uint16_t>(buffer,34),0u);
}

TEST_F (artemis_packet_test,client_consoles) {
	const auto buffer=artemis_packet::server_to_client::make_client_consoles(12,std::array<uint8_t,11>{0,1,2,3,4,5,6,7,8,9,10});
	check_client_console(buffer,12,0,1,2,3,4,5,6,7,8,9,10);
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

TEST_F (artemis_packet_test,constant_test) {
	EXPECT_EQ(0x4c821d3c,artemis_packet::client_to_server::value_int_jam32);
	EXPECT_EQ(0xe548e74a,artemis_packet::server_to_client::connected_jam32);
	EXPECT_EQ(0xf5821226,artemis_packet::server_to_client::heartbeat_jam32);
	EXPECT_EQ(0x6d04b3da,artemis_packet::server_to_client::plain_text_greeting_jam32);
	EXPECT_EQ(0x3de66711,artemis_packet::server_to_client::start_game_jam32);
	EXPECT_EQ(0xee665279,artemis_packet::server_to_client::object_text_jam32);
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
