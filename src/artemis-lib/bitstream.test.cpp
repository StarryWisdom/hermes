#include "gtest/gtest.h"
#include "bitstream.h"
#include "pc.h"

class artemis_bitfield_test :public ::testing::Test {
protected:
	void read_bitfield(uint8_t type,uint32_t aid) {
		EXPECT_EQ(buffer.read<uint8_t>(),type);
		EXPECT_EQ(buffer.read<uint32_t>(),aid);
	}
	~artemis_bitfield_test() {
		EXPECT_EQ(buffer.read_offset,buffer.write_offset);
	}
	packet_buffer buffer;
};

TEST_F (artemis_bitfield_test,too_many_elements) {
	object_update_bitfield bf{false,0,0,0};
	EXPECT_THROW(bf.write<uint8_t>(1),std::runtime_error);
}

TEST_F (artemis_bitfield_test,generic_write) {
	object_update_bitfield bf{false,1,1,2};
	bf.write<uint8_t>(3);
	buffer=bf.finalise();
	read_bitfield(1,2);
	EXPECT_EQ(buffer.read<uint8_t>(),1);//bitfield
	EXPECT_EQ(buffer.read<uint8_t>(),3);
}

TEST_F (artemis_bitfield_test,insufficent_writes) {
	object_update_bitfield bf{false,1,1,2};
	EXPECT_THROW(bf.finalise(),std::runtime_error);
}

TEST_F (artemis_bitfield_test,write_string) {
	object_update_bitfield bf{false,1,1,2};
	bf.write<std::string>("a");
	buffer=bf.finalise();
	read_bitfield(1,2);
	EXPECT_EQ(buffer.read<uint8_t>(),1);//bitfield
	EXPECT_EQ(buffer.read<uint32_t>(),2);//str len
	EXPECT_EQ(buffer.read<uint16_t>(),'a');
	EXPECT_EQ(buffer.read<uint16_t>(),0);//null byte
}

TEST_F (artemis_bitfield_test,empty) {
	object_update_bitfield bf{false,1,1,2};
	bf.write<uint8_t>(0,0);
	buffer=bf.finalise();
	EXPECT_EQ(buffer.write_offset,0);
}

TEST_F (artemis_bitfield_test,not_writing) {
	object_update_bitfield bf{false,2,1,2};
	bf.write<uint8_t>(3,3);
	bf.write<uint8_t>(3);
	buffer=bf.finalise();
	read_bitfield(1,2);
	EXPECT_EQ(buffer.read<uint8_t>(),2);//bitfield bit 1 off, bit 2 on
	EXPECT_EQ(buffer.read<uint8_t>(),3);
}

TEST_F(artemis_bitfield_test,added_padding_byte) {
	object_update_bitfield bf{false,8,1,2};
	bf.write<uint8_t>(3);
	bf.write<uint8_t>(3,3);
	bf.write<uint8_t>(3,3);
	bf.write<uint8_t>(3,3);
	bf.write<uint8_t>(3,3);
	bf.write<uint8_t>(3,3);
	bf.write<uint8_t>(3,3);
	bf.write<uint8_t>(3,3);
	buffer=bf.finalise();
	read_bitfield(1,2);
	EXPECT_EQ(buffer.read<uint8_t>(),1);//bitfield
	EXPECT_EQ(buffer.read<uint8_t>(),0);//padding byte when bitlen %8 ==0
	EXPECT_EQ(buffer.read<uint8_t>(),3);
}

TEST_F (artemis_bitfield_test,is_new_write) {
	object_update_bitfield bf{true,1,1,2};
	bf.write<uint8_t>(5,5);
	buffer=bf.finalise();
	read_bitfield(1,2);
	EXPECT_EQ(buffer.read<uint8_t>(),1);
	EXPECT_EQ(buffer.read<uint8_t>(),5);
}

TEST_F (artemis_bitfield_test,different_val_write) {
	object_update_bitfield bf{false,1,1,2};
	bf.write<uint8_t>(4,5);
	buffer=bf.finalise();
	read_bitfield(1,2);
	EXPECT_EQ(buffer.read<uint8_t>(),1);
	EXPECT_EQ(buffer.read<uint8_t>(),5);
}

class bitfield_header_test :public ::testing::Test {
};

TEST_F (bitfield_header_test,setting) {
	bitfield_header<9> a;

	EXPECT_EQ(a.is_bit_set(0),false);
	a.set_bit(0);
	auto r1=a.get_header();
	EXPECT_EQ(r1[0],std::byte(1));
	EXPECT_EQ(a.is_bit_set(0),true);

	EXPECT_EQ(a.is_bit_set(1),false);
	a.set_bit(1);
	r1=a.get_header();
	EXPECT_EQ(r1[0],std::byte(3));
	EXPECT_EQ(a.is_bit_set(1),true);

	EXPECT_EQ(a.is_bit_set(7),false);
	a.set_bit(7);
	r1=a.get_header();
	EXPECT_EQ(r1[0],std::byte(1+2+128));
	EXPECT_EQ(r1[1],std::byte(0));
	EXPECT_EQ(a.is_bit_set(7),true);

	EXPECT_EQ(a.is_bit_set(8),false);
	a.set_bit(8);
	r1=a.get_header();
	EXPECT_EQ(r1[0],std::byte(1+2+128));
	EXPECT_EQ(r1[1],std::byte(1));
	EXPECT_EQ(a.is_bit_set(8),true);

	EXPECT_EQ(a.is_bit_set(2),false);
}

TEST_F (bitfield_header_test,deque_constructor) {
	std::deque q{std::byte{255},std::byte{0}};
	auto q2=q;
	bitfield_header<7> a{&q2};
	EXPECT_EQ(a.is_bit_set(1),true);

	bitfield_header<8> b{&q};
	EXPECT_EQ(a.is_bit_set(1),true);
}

TEST_F (bitfield_header_test,size) {
	EXPECT_EQ(bitfield_header<1>::bitfield_size,1);
	EXPECT_EQ(bitfield_header<7>::bitfield_size,1);
	EXPECT_EQ(bitfield_header<8>::bitfield_size,2);
	EXPECT_EQ(bitfield_header<15>::bitfield_size,2);
	EXPECT_EQ(bitfield_header<16>::bitfield_size,3);
	EXPECT_EQ(bitfield_header<23>::bitfield_size,3);
	EXPECT_EQ(bitfield_header<24>::bitfield_size,4);
}

TEST_F (bitfield_header_test,set_invalid) {
	bitfield_header<7> a;
	EXPECT_THROW(a.set_bit(8),std::runtime_error);
}

TEST_F (bitfield_header_test,get_invalid) {
	bitfield_header<7> a;
	EXPECT_THROW(a.set_bit(8),std::runtime_error);
}

TEST_F (bitfield_header_test,invalid_data) {
	std::deque<std::byte> tmp;
	EXPECT_THROW(bitfield_header<8> d1{&tmp},std::runtime_error);
}

class engineering_data_test :public ::testing::Test {
public:
	engineering_data_test() {
		full_example.id=10;
		full_example.power[0]=p[0];
		full_example.power[1]=p[1];
		full_example.power[2]=p[2];
		full_example.power[3]=p[3];
		full_example.power[4]=p[4];
		full_example.power[5]=p[5];
		full_example.power[6]=p[6];
		full_example.power[7]=p[7];
		full_example.heat[0]=h[0];
		full_example.heat[1]=h[1];
		full_example.heat[2]=h[2];
		full_example.heat[3]=h[3];
		full_example.heat[4]=h[4];
		full_example.heat[5]=h[5];
		full_example.heat[6]=h[6];
		full_example.heat[7]=h[7];
		full_example.coolant[0]=c[0];
		full_example.coolant[1]=c[1];
		full_example.coolant[2]=c[2];
		full_example.coolant[3]=c[3];
		full_example.coolant[4]=c[4];
		full_example.coolant[5]=c[5];
		full_example.coolant[6]=c[6];
		full_example.coolant[7]=c[7];
	}

	void check_full_example(const update_engineering_data& to_check) const {
		EXPECT_EQ(to_check.id,10);
		EXPECT_EQ(*to_check.power[0],p[0]);
		EXPECT_EQ(*to_check.power[1],p[1]);
		EXPECT_EQ(*to_check.power[2],p[2]);
		EXPECT_EQ(*to_check.power[3],p[3]);
		EXPECT_EQ(*to_check.power[4],p[4]);
		EXPECT_EQ(*to_check.power[5],p[5]);
		EXPECT_EQ(*to_check.power[6],p[6]);
		EXPECT_EQ(*to_check.power[7],p[7]);
		EXPECT_EQ(*to_check.heat[0],h[0]);
		EXPECT_EQ(*to_check.heat[1],h[1]);
		EXPECT_EQ(*to_check.heat[2],h[2]);
		EXPECT_EQ(*to_check.heat[3],h[3]);
		EXPECT_EQ(*to_check.heat[4],h[4]);
		EXPECT_EQ(*to_check.heat[5],h[5]);
		EXPECT_EQ(*to_check.heat[6],h[6]);
		EXPECT_EQ(*to_check.heat[7],h[7]);
		EXPECT_EQ(*to_check.coolant[0],c[0]);
		EXPECT_EQ(*to_check.coolant[1],c[1]);
		EXPECT_EQ(*to_check.coolant[2],c[2]);
		EXPECT_EQ(*to_check.coolant[3],c[3]);
		EXPECT_EQ(*to_check.coolant[4],c[4]);
		EXPECT_EQ(*to_check.coolant[5],c[5]);
		EXPECT_EQ(*to_check.coolant[6],c[6]);
		EXPECT_EQ(*to_check.coolant[7],c[7]);
	}

	void check_full_example(const pc& to_check) const {
		//id impossible to check (as it has been removed by the time we have the pc class)
		EXPECT_EQ(to_check.power[0],p[0]);
		EXPECT_EQ(to_check.power[1],p[1]);
		EXPECT_EQ(to_check.power[2],p[2]);
		EXPECT_EQ(to_check.power[3],p[3]);
		EXPECT_EQ(to_check.power[4],p[4]);
		EXPECT_EQ(to_check.power[5],p[5]);
		EXPECT_EQ(to_check.power[6],p[6]);
		EXPECT_EQ(to_check.power[7],p[7]);
		EXPECT_EQ(to_check.heat[0],h[0]);
		EXPECT_EQ(to_check.heat[1],h[1]);
		EXPECT_EQ(to_check.heat[2],h[2]);
		EXPECT_EQ(to_check.heat[3],h[3]);
		EXPECT_EQ(to_check.heat[4],h[4]);
		EXPECT_EQ(to_check.heat[5],h[5]);
		EXPECT_EQ(to_check.heat[6],h[6]);
		EXPECT_EQ(to_check.heat[7],h[7]);
		EXPECT_EQ(to_check.coolant[0],c[0]);
		EXPECT_EQ(to_check.coolant[1],c[1]);
		EXPECT_EQ(to_check.coolant[2],c[2]);
		EXPECT_EQ(to_check.coolant[3],c[3]);
		EXPECT_EQ(to_check.coolant[4],c[4]);
		EXPECT_EQ(to_check.coolant[5],c[5]);
		EXPECT_EQ(to_check.coolant[6],c[6]);
		EXPECT_EQ(to_check.coolant[7],c[7]);
	}

	update_engineering_data full_example;
	const std::array<float,8> p{0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7};
	const std::array<float,8> h{1.0,1.1,1.2,1.3,1.4,1.5,1.6,1.7};
	const std::array<uint8_t,8> c{0,1,2,3,4,5,6,7};
};

TEST_F (engineering_data_test,round_trip) {
	auto update=full_example.build_data();
	update_engineering_data dat2{&update};
	check_full_example(dat2);
}

TEST_F (engineering_data_test,merge_into_pc) {
	pc pc;
	full_example.merge_into(&pc);
	check_full_example(pc);
}

TEST_F (engineering_data_test,extra_data_safe) {
	auto data=full_example.build_data();
	data.push_back(std::byte(255));
	update_engineering_data tmp(&data);
	EXPECT_EQ(data.size(),1);
	EXPECT_EQ(data[0],std::byte(255));
}

TEST_F (engineering_data_test,identical_class_constructor) {
	pc a;
	update_engineering_data c{10,a,&a};
	EXPECT_EQ(c.id,10);
	EXPECT_EQ(c.coolant[0].has_value(),false);
	EXPECT_EQ(c.coolant[1].has_value(),false);
	EXPECT_EQ(c.coolant[2].has_value(),false);
	EXPECT_EQ(c.coolant[3].has_value(),false);
	EXPECT_EQ(c.coolant[4].has_value(),false);
	EXPECT_EQ(c.coolant[5].has_value(),false);
	EXPECT_EQ(c.coolant[6].has_value(),false);
	EXPECT_EQ(c.coolant[7].has_value(),false);
	EXPECT_EQ(c.heat[0].has_value(),false);
	EXPECT_EQ(c.heat[1].has_value(),false);
	EXPECT_EQ(c.heat[2].has_value(),false);
	EXPECT_EQ(c.heat[3].has_value(),false);
	EXPECT_EQ(c.heat[4].has_value(),false);
	EXPECT_EQ(c.heat[5].has_value(),false);
	EXPECT_EQ(c.heat[6].has_value(),false);
	EXPECT_EQ(c.heat[7].has_value(),false);
	EXPECT_EQ(c.power[0].has_value(),false);
	EXPECT_EQ(c.power[1].has_value(),false);
	EXPECT_EQ(c.power[2].has_value(),false);
	EXPECT_EQ(c.power[3].has_value(),false);
	EXPECT_EQ(c.power[4].has_value(),false);
	EXPECT_EQ(c.power[5].has_value(),false);
	EXPECT_EQ(c.power[6].has_value(),false);
	EXPECT_EQ(c.power[7].has_value(),false);
}

TEST_F (engineering_data_test,NaN_check) {
	pc a;
	pc b;
	a.power[0]=NAN;
	b.power[0]=a.power[0];
	update_engineering_data c(10,a,&b);
	EXPECT_EQ(c.power[0].has_value(),false);
}

TEST_F (engineering_data_test,parital_diff) {
	pc a;
	pc b=a;
	a.power[0]=0;
	a.power[1]=0;
	a.power[2]=0;
	a.power[3]=0;
	a.power[4]=0;
	a.power[5]=0;
	a.power[6]=0;
	a.power[7]=0;
	a.heat[0]=1;
	a.heat[1]=1;
	a.heat[2]=1;
	a.heat[3]=1;
	a.heat[4]=1;
	a.heat[5]=1;
	a.heat[6]=1;
	a.heat[7]=1;
	a.coolant[0]=1;
	a.coolant[1]=0;
	a.coolant[2]=1;
	a.coolant[3]=0;
	a.coolant[4]=1;
	a.coolant[5]=0;
	a.coolant[6]=1;
	a.coolant[7]=0;
	update_engineering_data c(10,a,&b);
	EXPECT_EQ(c.id,10);
	EXPECT_EQ(c.coolant[0].has_value(),true);
	EXPECT_EQ(c.coolant[1].has_value(),false);
	EXPECT_EQ(c.coolant[2].has_value(),true);
	EXPECT_EQ(c.coolant[3].has_value(),false);
	EXPECT_EQ(c.coolant[4].has_value(),true);
	EXPECT_EQ(c.coolant[5].has_value(),false);
	EXPECT_EQ(c.coolant[6].has_value(),true);
	EXPECT_EQ(c.coolant[7].has_value(),false);
	EXPECT_EQ(*c.coolant[0],1);
	EXPECT_EQ(*c.coolant[2],1);
	EXPECT_EQ(*c.coolant[4],1);
	EXPECT_EQ(*c.coolant[6],1);
	EXPECT_EQ(c.heat[0].has_value(),true);
	EXPECT_EQ(c.heat[1].has_value(),true);
	EXPECT_EQ(c.heat[2].has_value(),true);
	EXPECT_EQ(c.heat[3].has_value(),true);
	EXPECT_EQ(c.heat[4].has_value(),true);
	EXPECT_EQ(c.heat[5].has_value(),true);
	EXPECT_EQ(c.heat[6].has_value(),true);
	EXPECT_EQ(c.heat[7].has_value(),true);
	EXPECT_EQ(c.heat[0],1);
	EXPECT_EQ(c.heat[1],1);
	EXPECT_EQ(c.heat[2],1);
	EXPECT_EQ(c.heat[3],1);
	EXPECT_EQ(c.heat[4],1);
	EXPECT_EQ(c.heat[5],1);
	EXPECT_EQ(c.heat[6],1);
	EXPECT_EQ(c.heat[7],1);
	EXPECT_EQ(c.power[0].has_value(),true);
	EXPECT_EQ(c.power[1].has_value(),true);
	EXPECT_EQ(c.power[2].has_value(),true);
	EXPECT_EQ(c.power[3].has_value(),true);
	EXPECT_EQ(c.power[4].has_value(),true);
	EXPECT_EQ(c.power[5].has_value(),true);
	EXPECT_EQ(c.power[6].has_value(),true);
	EXPECT_EQ(c.power[7].has_value(),true);
	EXPECT_EQ(c.power[0],0);
	EXPECT_EQ(c.power[1],0);
	EXPECT_EQ(c.power[2],0);
	EXPECT_EQ(c.power[3],0);
	EXPECT_EQ(c.power[4],0);
	EXPECT_EQ(c.power[5],0);
	EXPECT_EQ(c.power[6],0);
	EXPECT_EQ(c.power[7],0);
}

TEST_F (engineering_data_test,full_write) {
	pc a;
	a.power[0]=0;
	a.power[1]=0;
	a.power[2]=0;
	a.power[3]=0;
	a.power[4]=0;
	a.power[5]=0;
	a.power[6]=0;
	a.power[7]=0;
	a.heat[0]=1;
	a.heat[1]=1;
	a.heat[2]=1;
	a.heat[3]=1;
	a.heat[4]=1;
	a.heat[5]=1;
	a.heat[6]=1;
	a.heat[7]=1;
	a.coolant[0]=0;
	a.coolant[1]=1;
	a.coolant[2]=2;
	a.coolant[3]=3;
	a.coolant[4]=4;
	a.coolant[5]=5;
	a.coolant[6]=6;
	a.coolant[7]=7;
	update_engineering_data c(10,a,nullptr);
	EXPECT_EQ(c.coolant[0].has_value(),true);
	EXPECT_EQ(c.coolant[1].has_value(),true);
	EXPECT_EQ(c.coolant[2].has_value(),true);
	EXPECT_EQ(c.coolant[3].has_value(),true);
	EXPECT_EQ(c.coolant[4].has_value(),true);
	EXPECT_EQ(c.coolant[5].has_value(),true);
	EXPECT_EQ(c.coolant[6].has_value(),true);
	EXPECT_EQ(c.coolant[7].has_value(),true);
	EXPECT_EQ(*c.coolant[0],0);
	EXPECT_EQ(*c.coolant[1],1);
	EXPECT_EQ(*c.coolant[2],2);
	EXPECT_EQ(*c.coolant[3],3);
	EXPECT_EQ(*c.coolant[4],4);
	EXPECT_EQ(*c.coolant[5],5);
	EXPECT_EQ(*c.coolant[6],6);
	EXPECT_EQ(*c.coolant[7],7);
	EXPECT_EQ(c.heat[0].has_value(),true);
	EXPECT_EQ(c.heat[1].has_value(),true);
	EXPECT_EQ(c.heat[2].has_value(),true);
	EXPECT_EQ(c.heat[3].has_value(),true);
	EXPECT_EQ(c.heat[4].has_value(),true);
	EXPECT_EQ(c.heat[5].has_value(),true);
	EXPECT_EQ(c.heat[6].has_value(),true);
	EXPECT_EQ(c.heat[7].has_value(),true);
	EXPECT_EQ(c.heat[0],1);
	EXPECT_EQ(c.heat[1],1);
	EXPECT_EQ(c.heat[2],1);
	EXPECT_EQ(c.heat[3],1);
	EXPECT_EQ(c.heat[4],1);
	EXPECT_EQ(c.heat[5],1);
	EXPECT_EQ(c.heat[6],1);
	EXPECT_EQ(c.heat[7],1);
	EXPECT_EQ(c.power[0].has_value(),true);
	EXPECT_EQ(c.power[1].has_value(),true);
	EXPECT_EQ(c.power[2].has_value(),true);
	EXPECT_EQ(c.power[3].has_value(),true);
	EXPECT_EQ(c.power[4].has_value(),true);
	EXPECT_EQ(c.power[5].has_value(),true);
	EXPECT_EQ(c.power[6].has_value(),true);
	EXPECT_EQ(c.power[7].has_value(),true);
	EXPECT_EQ(c.power[0],0);
	EXPECT_EQ(c.power[1],0);
	EXPECT_EQ(c.power[2],0);
	EXPECT_EQ(c.power[3],0);
	EXPECT_EQ(c.power[4],0);
	EXPECT_EQ(c.power[5],0);
	EXPECT_EQ(c.power[6],0);
	EXPECT_EQ(c.power[7],0);
}
