#include "gtest/gtest.h"
#include "artemis-lib/packet.h"

//bleh buffer shouldnt really be defined here and these functions should be static :/
class packet_tests {
public:
	void check_artemis_header(artemis_packet::direction dir) {
		EXPECT_EQ(buffer.read<uint32_t>(),0xdeadbeef);
		EXPECT_EQ(buffer.read<uint32_t>(),buffer.write_offset);
		EXPECT_EQ(buffer.read<uint32_t>(),static_cast<uint32_t>(dir));
		EXPECT_EQ(buffer.read<uint32_t>(),0);
		EXPECT_EQ(buffer.read<uint32_t>(),buffer.write_offset-20);
	}

	void check_value_int_header(artemis_packet::value_int subtype) {
		check_artemis_header(artemis_packet::direction::client_to_server);
		EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::client_to_server::value_int_jam32);
		EXPECT_EQ(buffer.read<uint32_t>(),static_cast<uint32_t>(subtype));
	}

	void check_value_four_int_header(artemis_packet::value_four_ints subtype) {
		EXPECT_EQ(0x69cc01d9,artemis_packet::client_to_server::value_four_ints_jam32);
		check_artemis_header(artemis_packet::direction::client_to_server);
		EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::client_to_server::value_four_ints_jam32);
		EXPECT_EQ(buffer.read<uint32_t>(),static_cast<uint32_t>(subtype));
	}

	void check_simple_event_header(artemis_packet::simple_event subtype) {
		check_artemis_header(artemis_packet::direction::server_to_client);
		EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::simple_event_jam32);
		EXPECT_EQ(buffer.read<uint32_t>(),static_cast<uint32_t>(subtype));
	}

	void check_client_console(const uint8_t ship_num, const uint8_t c0, const uint8_t c1, const uint8_t c2, const uint8_t c3, const uint8_t c4, const uint8_t c5, const uint8_t c6, const uint8_t c7, const uint8_t c8, const uint8_t c9, const uint8_t c10) {
		EXPECT_EQ(artemis_packet::server_to_client::client_consoles_jam32,0x19c6e2d4);
		check_artemis_header(artemis_packet::direction::server_to_client);
		EXPECT_EQ(buffer.read<uint32_t>(),artemis_packet::server_to_client::client_consoles_jam32);
		EXPECT_EQ(buffer.read<uint32_t>(),ship_num);
		EXPECT_EQ(buffer.read<uint8_t>(),c0);
		EXPECT_EQ(buffer.read<uint8_t>(),c1);
		EXPECT_EQ(buffer.read<uint8_t>(),c2);
		EXPECT_EQ(buffer.read<uint8_t>(),c3);
		EXPECT_EQ(buffer.read<uint8_t>(),c4);
		EXPECT_EQ(buffer.read<uint8_t>(),c5);
		EXPECT_EQ(buffer.read<uint8_t>(),c6);
		EXPECT_EQ(buffer.read<uint8_t>(),c7);
		EXPECT_EQ(buffer.read<uint8_t>(),c8);
		EXPECT_EQ(buffer.read<uint8_t>(),c9);
		EXPECT_EQ(buffer.read<uint8_t>(),c10);
	}

	packet_buffer buffer;
};

class artemis_packet_test :public ::testing::Test, public packet_tests {
protected:
	virtual ~artemis_packet_test() {
		EXPECT_EQ(buffer.read_offset,buffer.write_offset);
	}
};
