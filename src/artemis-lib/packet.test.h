#include "gtest/gtest.h"
#include "artemis-lib/packet.h"
#include "core-lib/memory_buffer.h"

//bleh buffer shouldnt really be defined here and these functions should be static :/
class packet_tests {
public:
	[[deprecated]] void check_artemis_header(artemis_packet::direction dir) {
		EXPECT_EQ(buffer.read<uint32_t>(),0xdeadbeef);
		EXPECT_EQ(buffer.read<uint32_t>(),buffer.write_offset);
		EXPECT_EQ(buffer.read<uint32_t>(),static_cast<uint32_t>(dir));
		EXPECT_EQ(buffer.read<uint32_t>(),0);
		EXPECT_EQ(buffer.read<uint32_t>(),buffer.write_offset-20);
	}

	void check_artemis_header(const std::deque<std::byte>& buffer, artemis_packet::direction dir) {
		EXPECT_GE(buffer.size(),20);
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,0),0xdeadbeef);
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,4),buffer.size());
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,8),static_cast<uint32_t>(dir));
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,12),0);
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,16),buffer.size()-20);
	}

	[[deprecated]] void check_value_int_header(artemis_packet::value_int subtype) {
		check_artemis_header(artemis_packet::direction::client_to_server);
		EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::client_to_server::value_int_jam32);
		EXPECT_EQ(buffer.read<uint32_t>(),static_cast<uint32_t>(subtype));
	}

	[[deprecated]] void check_value_four_int_header(artemis_packet::value_four_ints subtype) {
		EXPECT_EQ(0x69cc01d9,artemis_packet::client_to_server::value_four_ints_jam32);
		check_artemis_header(artemis_packet::direction::client_to_server);
		EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::client_to_server::value_four_ints_jam32);
		EXPECT_EQ(buffer.read<uint32_t>(),static_cast<uint32_t>(subtype));
	}

	void check_simple_event_header(const std::deque<std::byte>& buffer, artemis_packet::simple_event subtype) {
		EXPECT_GE(buffer.size(),28);
		check_artemis_header(buffer,artemis_packet::direction::server_to_client);
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,20),artemis_packet::server_to_client::simple_event_jam32);
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,24),static_cast<uint32_t>(subtype));
	}

	[[deprecated]] void check_simple_event_header(artemis_packet::simple_event subtype) {
		check_artemis_header(artemis_packet::direction::server_to_client);
		EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::simple_event_jam32);
		EXPECT_EQ(buffer.read<uint32_t>(),static_cast<uint32_t>(subtype));
	}

	void check_client_console(const std::deque<std::byte>& buffer, const uint8_t ship_num, const uint8_t c0, const uint8_t c1, const uint8_t c2, const uint8_t c3, const uint8_t c4, const uint8_t c5, const uint8_t c6, const uint8_t c7, const uint8_t c8, const uint8_t c9, const uint8_t c10) {
		EXPECT_EQ(artemis_packet::server_to_client::client_consoles_jam32,0x19c6e2d4);
		EXPECT_EQ(buffer.size(),39);
		check_artemis_header(buffer,artemis_packet::direction::server_to_client);
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,20),artemis_packet::server_to_client::client_consoles_jam32);
		EXPECT_EQ(buffer::read_at<uint32_t>(buffer,24),ship_num);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,28),c0);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,29),c1);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,30),c2);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,31),c3);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,32),c4);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,33),c5);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,34),c6);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,35),c7);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,36),c8);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,37),c9);
		EXPECT_EQ(buffer::read_at<uint8_t>(buffer,38),c10);
	}

	packet_buffer buffer;
};

class artemis_packet_test :public ::testing::Test, public packet_tests {
protected:
	virtual ~artemis_packet_test() {
		EXPECT_EQ(buffer.read_offset,buffer.write_offset);
	}
};
