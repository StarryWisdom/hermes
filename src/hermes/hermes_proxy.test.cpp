#include "gtest/gtest.h"

#include "hermes.h"

class hermes_proxy_test :public ::testing::Test {
public:
	hermes_proxy_test()
		: proxy{hermes_class,std::move(tmp)}
	{}
	hermes hermes_class;
	net::io_context context;
	queued_nonblocking_socket tmp{context};
	hermes_proxy proxy;
};

TEST_F (hermes_proxy_test,fixup_single_change) {
	uint32_t id{10};
	artemis_server_info& info{hermes_class.server_info};
	info.server_data.pc_data.emplace(id,pc());
	{
		//flush through the changes
		auto tmp1=proxy.data_fixup().at(0);
		proxy.enqueue_client_write(tmp1);
		EXPECT_NE(tmp1.write_offset,0);
		auto tmp2=proxy.data_fixup().at(0);
		EXPECT_EQ(tmp2.write_offset,0);
	}
	EXPECT_EQ(proxy.data_fixup().at(0).write_offset,0);
	info.server_data.pc_data.at(id).coolant[0]=254;
	auto buf=proxy.data_fixup().at(0);
	buf.read_offset=24;//ignore header

	EXPECT_EQ(buf.read<uint8_t>(),3);
	EXPECT_EQ(buf.read<uint32_t>(),10);

	EXPECT_EQ(buf.read<uint8_t>(),0);
	EXPECT_EQ(buf.read<uint8_t>(),0);
	EXPECT_EQ(buf.read<uint8_t>(),1);
	EXPECT_EQ(buf.read<uint8_t>(),0);

	EXPECT_EQ(buf.read<uint8_t>(),254);
	EXPECT_EQ(buf.read<uint32_t>(),0);
	EXPECT_EQ(buf.read_offset,buf.write_offset);
}

TEST_F (hermes_proxy_test,fixup_multiple_change) {
	uint32_t id1{10};
	uint32_t id2{11};
	artemis_server_info& info{hermes_class.server_info};
	info.server_data.pc_data.emplace(id1,pc());
	info.server_data.pc_data.emplace(id2,pc());
	{
		//flush through the changes
		auto tmp1=proxy.data_fixup();
		proxy.enqueue_client_write(tmp1);
		EXPECT_NE(tmp1.at(0).write_offset,0);
		auto tmp2=proxy.data_fixup();
		EXPECT_EQ(tmp2.at(0).write_offset,0);
	}
	EXPECT_EQ(proxy.data_fixup().at(0).write_offset,0);
	info.server_data.pc_data.at(id1).coolant[0]=254;
	info.server_data.pc_data.at(id2).coolant[1]=253;
	auto buf=proxy.data_fixup().at(0);
	buf.read_offset=24;//ignore header

	EXPECT_EQ(buf.read<uint8_t>(),3);
	EXPECT_EQ(buf.read<uint32_t>(),11);
	EXPECT_EQ(buf.read<uint8_t>(),0);//bitfield start
	EXPECT_EQ(buf.read<uint8_t>(),0);
	EXPECT_EQ(buf.read<uint8_t>(),2);
	EXPECT_EQ(buf.read<uint8_t>(),0);
	EXPECT_EQ(buf.read<uint8_t>(),253);//data

	EXPECT_EQ(buf.read<uint8_t>(),3);
	EXPECT_EQ(buf.read<uint32_t>(),10);
	EXPECT_EQ(buf.read<uint8_t>(),0);//bitfield start
	EXPECT_EQ(buf.read<uint8_t>(),0);
	EXPECT_EQ(buf.read<uint8_t>(),1);
	EXPECT_EQ(buf.read<uint8_t>(),0);
	EXPECT_EQ(buf.read<uint8_t>(),254);//data

	EXPECT_EQ(buf.read<uint32_t>(),0);
	EXPECT_EQ(buf.read_offset,buf.write_offset);
}

TEST_F (hermes_proxy_test,parse_eng_data_updated) {
	update_engineering_data update;
	update.coolant[0]=10;
	update.id=10;

	artemis_server_info& info{hermes_class.server_info};
	EXPECT_EQ(info.server_data.pc_data.count(10),0);

	const auto data_bytes=std::vector<std::deque<std::byte>>{update.build_data()};
	auto buffer=artemis_packet::server_to_client::make_object_bitstreams(data_bytes);
	std::deque<std::byte> tmp{buffer.at(0).buffer.begin(),buffer.at(0).buffer.begin()+buffer.at(0).write_offset};
	proxy.parse_s2c_packet(tmp);

	EXPECT_EQ(info.server_data.pc_data.count(10),1);
	EXPECT_EQ(info.server_data.pc_data.at(10).coolant[0],10);
}

TEST_F (hermes_proxy_test,parse_data_change) {
	update_engineering_data update;
	update.coolant[0]=10;
	update.id=10;
	artemis_server_info& info{hermes_class.server_info};

	const auto data_bytes{std::vector<std::deque<std::byte>>{update.build_data()}};
	auto buffer=artemis_packet::server_to_client::make_object_bitstreams(data_bytes);

	EXPECT_EQ(info.server_data.pc_data.count(10),0);
	std::deque<std::byte> tmp{buffer.at(0).buffer.begin(),buffer.at(0).buffer.begin()+buffer.at(0).write_offset};
	proxy.parse_s2c_packet(tmp);
	EXPECT_EQ(info.server_data.pc_data.count(10),1);

	info.server_data.pc_data.at(10).coolant[0]=11;
	auto tmp2=proxy.data_fixup();
	EXPECT_NE(tmp2.at(0).write_offset,0);
	proxy.enqueue_client_write(tmp2);

	auto tmp3=proxy.data_fixup();
	EXPECT_EQ(tmp3.at(0).write_offset,0);
}

TEST_F (hermes_proxy_test,inital_data_sync) {
	artemis_server_info& info{hermes_class.server_info};
	info.server_data.pc_data.emplace(10,pc());
	info.server_data.pc_data.at(10).coolant[0]=1;
	auto tmp=proxy.data_fixup();
	EXPECT_NE(tmp.at(0).write_offset,0);
}
