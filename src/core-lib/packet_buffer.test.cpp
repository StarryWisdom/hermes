#include "gtest/gtest.h"

#include "packet_buffer.h"

class packet_buffer_test :public ::testing::Test {
protected:
	packet_buffer buffer;
};

TEST_F (packet_buffer_test,write_u8_artemis_string) {
	std::u16string str;
	str+='a';//put any data in the str
	std::string u8str="a";
	buffer.write_artemis_string("a");
	EXPECT_EQ(buffer.read_artemis_string(),str);
}

TEST_F (packet_buffer_test,read_artemis_string) {
	std::u16string str;
	str+='a';//put any data in the str
	buffer.write_artemis_string(str);
	EXPECT_EQ(buffer.read_artemis_string(),str);
}

TEST_F (packet_buffer_test,read_string_throw) {
	EXPECT_EQ(buffer.read_offset,0);
	EXPECT_THROW(buffer.read<std::string>(),std::runtime_error);
}

TEST_F (packet_buffer_test,read_string) {
	buffer.write<std::string>("a");
	EXPECT_EQ(buffer.read<std::string>(),"a");
}

TEST_F (packet_buffer_test,write_packet_buffer) {
	packet_buffer buffer2;
	buffer2.write<uint8_t>(0);
	buffer2.write<uint8_t>(1);
	buffer.write(buffer2);
	EXPECT_EQ(buffer.read<uint8_t>(),0);
	EXPECT_EQ(buffer.read<uint8_t>(),1);
}

TEST_F (packet_buffer_test,write_packet_buffer_mid_buffer) {
	packet_buffer buffer2;
	buffer2.write<uint8_t>(0);
	buffer2.read<uint8_t>();
	buffer2.write<uint8_t>(1);
	EXPECT_THROW(buffer.write(buffer2),std::runtime_error);
}

TEST_F (packet_buffer_test,attempt_read_fail) {
	EXPECT_EQ(buffer.attempt_read<uint8_t>().has_value(),false);
}
