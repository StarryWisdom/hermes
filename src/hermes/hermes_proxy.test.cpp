#include "gtest/gtest.h"

#include "hermes.h"

class hermes_proxy_test :public ::testing::Test {
public:
	hermes hermes_class;
	hermes_proxy proxy{hermes_class,std::make_unique<client_socket_multiplexer>(client_socket_multiplexer())};
};

TEST_F (hermes_proxy_test,fixup_single_change) {
	uint32_t id{10};
	artemis_server_info& info{hermes_class.server_info};
	info.server_data.pc_data.emplace(id,pc());
	{
		//flush through the changes
		auto tmp1=proxy.data_fixup();
		proxy.enqueue_client_write(tmp1);
		EXPECT_NE(tmp1.size(),0);
		auto tmp2=proxy.data_fixup();
		EXPECT_EQ(tmp2.size(),0);
	}
	EXPECT_EQ(proxy.data_fixup().size(),0);
	info.server_data.pc_data.at(id).coolant[0]=254;
	const auto buf=proxy.data_fixup().at(0);

	//ignore header
	EXPECT_EQ(buf.size(),38);

	EXPECT_EQ(buffer::read_at<uint8_t>(buf,24),3);
	EXPECT_EQ(buffer::read_at<uint32_t>(buf,25),10);

	EXPECT_EQ(buffer::read_at<uint8_t>(buf,29),0);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,30),0);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,31),1);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,32),0);

	EXPECT_EQ(buffer::read_at<uint8_t>(buf,33),254);
	EXPECT_EQ(buffer::read_at<uint32_t>(buf,34),0);
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
		EXPECT_NE(tmp1.size(),0);
		auto tmp2=proxy.data_fixup();
		EXPECT_EQ(tmp2.size(),0);
	}
	EXPECT_EQ(proxy.data_fixup().size(),0);
	info.server_data.pc_data.at(id1).coolant[0]=254;
	info.server_data.pc_data.at(id2).coolant[1]=253;
	const auto buf=proxy.data_fixup().at(0);
	EXPECT_EQ(buf.size(),48);

	EXPECT_EQ(buffer::read_at<uint8_t>(buf,24),3);
	EXPECT_EQ(buffer::read_at<uint32_t>(buf,25),11);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,29),0);//bitfield start
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,30),0);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,31),2);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,32),0);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,33),253);//data

	EXPECT_EQ(buffer::read_at<uint8_t>(buf,34),3);
	EXPECT_EQ(buffer::read_at<uint32_t>(buf,35),10);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,39),0);//bitfield start
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,40),0);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,41),1);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,42),0);
	EXPECT_EQ(buffer::read_at<uint8_t>(buf,43),254);//data

	EXPECT_EQ(buffer::read_at<uint32_t>(buf,44),0);
}

TEST_F (hermes_proxy_test,parse_eng_data_updated) {
	update_engineering_data update;
	update.coolant[0]=10;
	update.id=10;

	artemis_server_info& info{hermes_class.server_info};
	EXPECT_EQ(info.server_data.pc_data.count(10),0);

	auto data_bytes=std::vector<std::deque<std::byte>>{update.build_data()};
	auto buffer=artemis_packet::server_to_client::finalize_object_bitstreams(std::move(data_bytes));
	std::deque<std::byte> tmp{buffer.at(0).begin(),buffer.at(0).end()};
	proxy.parse_s2c_packet(tmp);

	EXPECT_EQ(info.server_data.pc_data.count(10),1);
	EXPECT_EQ(info.server_data.pc_data.at(10).coolant[0],10);
}

TEST_F (hermes_proxy_test,parse_data_change) {
	update_engineering_data update;
	update.coolant[0]=10;
	update.id=10;
	artemis_server_info& info{hermes_class.server_info};

	auto data_bytes{std::vector<std::deque<std::byte>>{update.build_data()}};
	auto buffer=artemis_packet::server_to_client::finalize_object_bitstreams(std::move(data_bytes));

	EXPECT_EQ(info.server_data.pc_data.count(10),0);
	std::deque<std::byte> tmp{buffer.at(0).begin(),buffer.at(0).end()};
	proxy.parse_s2c_packet(tmp);
	EXPECT_EQ(info.server_data.pc_data.count(10),1);

	info.server_data.pc_data.at(10).coolant[0]=11;
	auto tmp2=proxy.data_fixup();
	EXPECT_NE(tmp2.size(),0);
	proxy.enqueue_client_write(tmp2);

	auto tmp3=proxy.data_fixup();
	EXPECT_EQ(tmp3.size(),0);
}

TEST_F (hermes_proxy_test,inital_data_sync) {
	artemis_server_info& info{hermes_class.server_info};
	info.server_data.pc_data.emplace(10,pc());
	info.server_data.pc_data.at(10).coolant[0]=1;
	auto tmp=proxy.data_fixup();
	EXPECT_NE(tmp.size(),0);
}
