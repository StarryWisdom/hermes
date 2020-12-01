#include "memory_buffer.h"

#include "gtest/gtest.h"

// Ideally there would be a few more tests to ensure that front and back arent getting confused at any point
class memory_buffer_test :public ::testing::Test {
public:
	std::deque<std::byte> a;
	~memory_buffer_test() {
		EXPECT_EQ(a.empty(),true);
	}
};

TEST_F (memory_buffer_test, front_roundtrip_uint8_order) {
	buffer::push_front<uint8_t>(a,1);
	buffer::push_front<uint8_t>(a,2);
	EXPECT_EQ(buffer::pop_front<uint8_t>(a),2);
	EXPECT_EQ(buffer::pop_front<uint8_t>(a),1);
}

TEST_F (memory_buffer_test, front_roundtrip_uint16) {
	buffer::push_front<uint16_t>(a,300);
	EXPECT_EQ(buffer::peek_front<uint16_t>(a),300);
	EXPECT_EQ(buffer::pop_front<uint16_t>(a),300);
}

TEST_F (memory_buffer_test, back_roundtrip_uint8_order) {
	buffer::push_back<uint8_t>(a,1);
	buffer::push_back<uint8_t>(a,2);
	EXPECT_EQ(buffer::pop_back<uint8_t>(a),2);
	EXPECT_EQ(buffer::pop_back<uint8_t>(a),1);
}

TEST_F (memory_buffer_test, back_roundtrip_uint16) {
	buffer::push_back<uint16_t>(a,300);
	EXPECT_EQ(buffer::peek_back<uint16_t>(a),300);
	EXPECT_EQ(buffer::pop_back<uint16_t>(a),300);
}

TEST_F (memory_buffer_test, back_roundtrip_uint32) {
	buffer::push_back<uint32_t>(a,300);
	EXPECT_EQ(buffer::peek_back<uint32_t>(a),300);
	EXPECT_EQ(buffer::pop_back<uint32_t>(a),300);
}

TEST_F (memory_buffer_test, interleaved) {
	buffer::push_back<uint8_t>(a,1);
	buffer::push_front<uint8_t>(a,2);
	buffer::push_back<uint8_t>(a,3);
	EXPECT_EQ(buffer::pop_back<uint8_t>(a),3);
	EXPECT_EQ(buffer::pop_back<uint8_t>(a),1);
	EXPECT_EQ(buffer::pop_back<uint8_t>(a),2);
}

TEST_F (memory_buffer_test, invalid_peek_back) {
	EXPECT_THROW(buffer::peek_back<uint8_t>(a),std::runtime_error);
}

TEST_F (memory_buffer_test, pop_front_n) {
	buffer::push_back<std::byte>(a,std::byte(1));
	buffer::push_back<std::byte>(a,std::byte(2));
	buffer::push_back<std::byte>(a,std::byte(3));
	buffer::push_back<std::byte>(a,std::byte(4));
	const auto& b1=buffer::pop_front_n<1>(a);
	EXPECT_EQ(b1[0],std::byte(1));
	const auto& b2=buffer::pop_front_n<3>(a);
	EXPECT_EQ(b2.size(),3);
	EXPECT_EQ(b2[0],std::byte(2));
	EXPECT_EQ(b2[1],std::byte(3));
	EXPECT_EQ(b2[2],std::byte(4));
}

TEST_F (memory_buffer_test, read_at) {
	buffer::push_back<uint16_t>(a,300);
	buffer::push_back<uint8_t>(a,1);
	buffer::push_front<uint8_t>(a,2);
	EXPECT_EQ(buffer::read_at<uint16_t>(a,1),300);
	EXPECT_EQ(buffer::read_at<uint8_t>(a,0),2);
	EXPECT_EQ(buffer::read_at<uint8_t>(a,3),1);
	a.clear();
}
