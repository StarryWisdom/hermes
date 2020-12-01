#pragma once

#include <cstddef>
#include <stdint.h>
#include <string>
#include <algorithm>
#include <iterator>
#include <queue>

#include "core-lib/packet_buffer.h"
#include "core-lib/memory_buffer.h"
#include "pc.h"
#include "npc.h"

class object_update_bitfield {
public:
	// is_new refers to is this packet new
	// if it is we want to send all fields we are given not just the ones that have changed
	object_update_bitfield(bool is_new, uint32_t bools_num, uint8_t type, uint32_t id)
		: bools_num(bools_num)
		, is_new(is_new)
	{
		buffer.write<uint8_t>(type);
		buffer.write<uint32_t>(id);
		for (int32_t a=bools_num; a>=0; a-=8) {
			buffer.write<uint8_t>(0);
		}
	}

	template <typename T> void write(T val) {
		if constexpr (std::is_same<std::decay<T>,std::decay<std::string>>::value) {
			buffer.write_artemis_string(val);
		} else {
			buffer.write<T>(val);
		}
		if (current_bool>=bools_num) {
			throw std::runtime_error("too many elements written in artemis_bitfield");
		}
		uint32_t write_offset=5+(current_bool/8);
		std::byte orig_byte=buffer.buffer[write_offset];
		uint32_t bit_num=current_bool-(8*(current_bool/8));
		std::byte new_byte=orig_byte | std::byte(1 << bit_num);
		buffer.buffer[write_offset]=new_byte;
		anything_written=true;
		current_bool++;
	}

	template <typename T> void write(T val_orig, T val_new) {
		if (is_new || val_orig!=val_new) {
			this->write(val_new);
		} else {
			current_bool++;
		}
	}

	packet_buffer finalise() {
		if (current_bool!=bools_num) {
			throw std::runtime_error("incorrect number of element in artemis_bitfield");
		}
		if (!anything_written) {
			return packet_buffer();
		}
		return buffer;
	}
private:
	uint32_t bools_num;
	bool is_new;
	uint32_t current_bool{0};
	packet_buffer buffer;
	bool anything_written{false};
};

template <const uint8_t num_bools> class bitfield_header {
public:
	constexpr bitfield_header () {
	}

	//note this consumes deque elements
	bitfield_header(std::deque<std::byte>* in) {
		bytes=buffer::pop_front_n<bitfield_size>(*in);
	}

	static constexpr size_t bitfield_size=(num_bools+8)/8;

	constexpr const std::array<std::byte,bitfield_size>& get_header() const {
		return bytes;
	}

	constexpr void set_bit(uint8_t bit) {
		const auto& [byte_num,bit_num] = calc_byte(bit);
		bytes[byte_num]|=(std::byte(1)<<bit_num);
	}

	constexpr bool is_bit_set(uint8_t bit) const {
		const auto& [byte_num,bit_num] = calc_byte(bit);
		return (bytes[byte_num]&(std::byte(1)<<bit_num))!=std::byte(0);
	}

private:
	constexpr auto calc_byte(uint8_t bit) const {
		if (bit>num_bools) {
			throw std::runtime_error("trying to use a bitfield outside of bounds");
		}
		const auto byte_num=bit/8;
		const auto bit_num=bit%8;
		return std::tuple(byte_num,bit_num);
	}

	std::array<std::byte,bitfield_size> bytes{std::byte()};
};

enum class update_dispatch_type {
	inflate, // std::deque -> update_class
	build,   // update_class -> std::deque
	merge,   // take a base class apply the update class to it
	diff,    // take 2 base classes and build the update from it
};

template <uint8_t bits, typename update_class, typename base_class> class update_common {
private:
	uint32_t bit{0};
	update_class* update;
	base_class* merge;
	bitfield_header<bits>* bitfield;
	std::deque<std::byte>* deq;
	const base_class* preexisting_data;
	const base_class* original;
public:
	update_common(update_class* update, base_class* merge, bitfield_header<bits>* bitfield, std::deque<std::byte>* deq, const base_class* preexisting_data, const base_class* original)
		: update(update)
		, merge(merge)
		, bitfield(bitfield)
		, deq(deq)
		, preexisting_data(preexisting_data)
		, original(original)
	{
	}

	~update_common() {
		if (bit!=bits) {
			// this really makes the most sense inside of the deconstructor
			// however deconstructors are nothrow
			// so exceptions are out
			// and I dont want to always have to remeber to check it
			// so ... um ... this is non ideal but it is what it is
			// I would leave it as a throw but it shows as a warning
			// so yuk, just yuk all round
			std::cout << "error inside of update_data_common";
			std::terminate();
		}
	}

	template <typename T> void dispatch_diff(std::optional<T>* member, const T* arg1, const T* arg2=nullptr) {
		if constexpr (std::is_same<std::decay<T>,std::decay<float>>::value) {
			//NaN is != to itself
			//as such we are going to compare bit patterns
			if (arg2==nullptr || *(uint32_t*)(arg1)!=*(uint32_t*)(arg2)) {
				*member=*arg1;//TODO CHECK IF IT SHOULD BE ARG2 or ARG3
			}
		} else {
			if (arg2==nullptr || *arg1!=*arg2) {
				*member=*arg1;//TODO CHECK IF IT SHOULD BE ARG2 or ARG3
			}
		}
		++bit;
	}

	template <typename T> void dispatch_merge(std::optional<T>* member, T* dest) {
		if (member->has_value()) {
			*dest=**member;
		}
		++bit;
	}

	template <typename T> void dispatch_inflate(std::optional<T>* member) {
		if (bitfield->is_bit_set(bit)) {
			if constexpr (std::is_same<std::decay<T>,std::decay<std::string>>::value) {
				(*member)=buffer::read_artemis_string_u8(*deq);
			} else {
				(*member)=buffer::pop_front<T>(*deq);
			}
		}
		++bit;
	}

	template <typename T> void dispatch_build(std::optional<T>* member) {
		if (member->has_value()) {
			bitfield->set_bit(bit);
			if constexpr (std::is_same<std::decay<T>,std::decay<std::string>>::value) {
				buffer::write_artemis_string(*deq,**member);
			} else {
				buffer::push_back(*deq,**member);
			}
		}
		++bit;
	}

	template <update_dispatch_type type, typename T1, typename T2> void dispatch_singleton(std::optional<T1>* member, [[maybe_unused]]T2 base_class::* arg2) {
		if constexpr (type==update_dispatch_type::diff) {
			if (preexisting_data!=nullptr) {
				dispatch_diff(member,&(original->*arg2),&(preexisting_data->*arg2));
			} else {
				dispatch_diff(member,&(original->*arg2));
			}
		} else if constexpr (type==update_dispatch_type::merge) {
			dispatch_merge(member,&(merge->*arg2));
		} else if constexpr (type==update_dispatch_type::inflate) {
			dispatch_inflate(member);
		} else if constexpr (type==update_dispatch_type::build) {
			dispatch_build(member);
		}
	}

	template <update_dispatch_type type, typename T, size_t size>
		void dispatch_array(std::array<std::optional<T>,size>* member, [[maybe_unused]] std::vector<T> base_class::* arg2) {
		if constexpr (type==update_dispatch_type::diff) {
			if (preexisting_data!=nullptr && (preexisting_data->*arg2).size()!=(original->*arg2).size()) {
				throw std::runtime_error("usage error in merge");
			}
			for (auto i{0}; i!=size; ++i) {//this probably wants to be a zip iterator
				if (preexisting_data!=nullptr) {
					dispatch_diff(&((*member)[i]),&((original->*arg2)[i]),&((preexisting_data->*arg2)[i]));
				} else {
					dispatch_diff(&((*member)[i]),&((original->*arg2)[i]));
				}
			}
		} else if constexpr (type==update_dispatch_type::merge) {
			for (auto i{0}; i!=size; ++i) {
				dispatch_merge(&((*member)[i]),&((merge->*arg2)[i]));
			}
		} else if constexpr (type==update_dispatch_type::inflate) {
			for (auto& i : *member) {
				dispatch_inflate(&i);
			}
		} else if constexpr (type==update_dispatch_type::build) {
			for (auto& i : *member) {
				dispatch_build(&i);
			}
		}
	}
};

class object_bitstream {
public:
	void add_bitstream(const std::deque<std::byte>& in) {
		data.push_back(in);
	}

	//TODO lacking testing code
	//probably also needs code to break packets into maxium artemis size (2K?)
	std::vector<std::deque<std::byte>> get_bitstreams() {
		std::vector<std::deque<std::byte>> ret;
		std::deque<std::byte> tmp;
		for (auto& i : data) {
			if (tmp.size() + i.size() > 2000) {
				ret.push_back(tmp);
				tmp.clear();
			}
			std::copy(std::begin(i),std::end(i),std::back_inserter(tmp));
		}
		ret.push_back(tmp);
		return ret;
	}
private:
	std::vector<std::deque<std::byte>> data;
};

class update_player_data {
public:
	static constexpr uint8_t bits{45};

	uint32_t id{0};
	std::optional<uint32_t> wep_id;
	std::optional<float> throttle;
	std::optional<float> rudder;
	std::optional<float> top_speed;
	std::optional<float> turn_rate;
	std::optional<bool> auto_beams;
	std::optional<uint8_t> warp;
	std::optional<float> energy;
	//byte 2
	std::optional<uint16_t> shield_status;
	std::optional<uint32_t> unknown22;
	std::optional<uint32_t> visual_id;
	std::optional<float> x;
	std::optional<float> y;
	std::optional<float> z;
	std::optional<float> pitch;
	std::optional<float> roll;
	//byte 3
	std::optional<float> heading;
	std::optional<float> velocity;
	std::optional<uint8_t> neb_type;
	std::optional<std::string> scanning_name;
	std::optional<float> front_current;
	std::optional<float> front_max;
	std::optional<float> rear_current;
	std::optional<float> rear_max;
	//byte 4
	std::optional<uint32_t> last_docked;
	std::optional<bool> red_alert;
	std::optional<float> unknown43;
	std::optional<uint8_t> mainscreen;
	std::optional<uint8_t> beam_freq;
	std::optional<uint8_t> coolant_or_missles;
	std::optional<uint32_t> science_target;
	std::optional<uint32_t> captians_target;
	//byte 5
	std::optional<uint8_t> drive_type;
	std::optional<uint32_t> scanning_target;
	std::optional<float> scanning_progress;
	std::optional<bool> reverse;
	std::optional<float> climb_dive;
	std::optional<uint8_t> game_side;
	std::optional<uint32_t> map_visability;
	std::optional<uint8_t> ship_num;
	//byte 6
	std::optional<uint32_t> capital_id;
	std::optional<float> accent;
	std::optional<float> combat_jump;
	std::optional<uint8_t> beacon_type;
	std::optional<uint8_t> beacon_mode;

	constexpr update_player_data () {
	}

	update_player_data(std::deque<std::byte>* in) {
		if (buffer::pop_front<uint8_t>(*in)!=1) {
			throw std::runtime_error("data stream error - internal type doesnt match external");
		}
		id=uint32_t{buffer::pop_front<uint32_t>(*in)};;
		bitfield_header<bits> bitfield{in};
		inner_dispatch<update_dispatch_type::inflate>(in,nullptr,&bitfield);
	}

	update_player_data(int id, const pc& real, const pc* last_transmitted) :
		id(id)
	{
		inner_dispatch<update_dispatch_type::diff>(nullptr,nullptr,nullptr,&real,last_transmitted);
	}

	// this techincally could be const but is annoyingly hard to do
	std::deque<std::byte> build_data() {
		bitfield_header<bits> bitfield;

		std::deque<std::byte> ret;
		inner_dispatch<update_dispatch_type::build>(&ret,nullptr,&bitfield);
		if (ret.size()!=0) {
			buffer::push_front(ret,bitfield.get_header());
			buffer::push_front<uint32_t>(ret,id);
			buffer::push_front<uint8_t>(ret,1);
		}
		return ret;
	}

	void merge_into(class pc* dest) {
		inner_dispatch<update_dispatch_type::merge>(nullptr,dest);
	}
private:
	template <const update_dispatch_type type> void inner_dispatch (std::deque<std::byte>* deq, pc* merge, bitfield_header<bits>* bitfield=nullptr, const pc* original=nullptr, const pc* preexisiting_data=nullptr) {
		//TODO check if an error in here calls terminate (it seemed to when name was being read wrongly)
		static_assert(bitfield_header<bits>::bitfield_size==6);
		update_common<bits,update_player_data,pc> header{this,merge,bitfield,deq,preexisiting_data,original};
		header.dispatch_singleton<type>(&wep_id,&pc::hermes_wep_target);
		header.dispatch_singleton<type>(&throttle,&pc::throttle);
		header.dispatch_singleton<type>(&rudder,&pc::rudder);
		header.dispatch_singleton<type>(&top_speed,&pc::hermes_top_speed);
		header.dispatch_singleton<type>(&turn_rate,&pc::hermes_turn_rate);
		header.dispatch_singleton<type>(&auto_beams,&pc::auto_beams);
		header.dispatch_singleton<type>(&warp,&pc::warp);
		header.dispatch_singleton<type>(&energy,&pc::energy);
		//byte 2
		header.dispatch_singleton<type>(&shield_status,&pc::hermes_shield_status);
		header.dispatch_singleton<type>(&unknown22,&pc::unknown22);
		header.dispatch_singleton<type>(&visual_id,&pc::hermes_visual_id);
		header.dispatch_singleton<type>(&x,&pc::hermes_x);
		header.dispatch_singleton<type>(&y,&pc::hermes_y);
		header.dispatch_singleton<type>(&z,&pc::hermes_z);
		header.dispatch_singleton<type>(&pitch,&pc::hermes_pitch);
		header.dispatch_singleton<type>(&roll,&pc::hermes_roll);
		//byte 3
		header.dispatch_singleton<type>(&heading,&pc::hermes_heading);
		header.dispatch_singleton<type>(&velocity,&pc::hermes_velocity);
		header.dispatch_singleton<type>(&neb_type,&pc::hermes_neb_type);
		header.dispatch_singleton<type>(&scanning_name,&pc::hermes_name);
		header.dispatch_singleton<type>(&front_current,&pc::hermes_front_current);
		header.dispatch_singleton<type>(&front_max,&pc::hermes_front_max);
		header.dispatch_singleton<type>(&rear_current,&pc::hermes_rear_current);
		header.dispatch_singleton<type>(&rear_max,&pc::hermes_rear_max);
		//byte 4
		header.dispatch_singleton<type>(&last_docked,&pc::hermes_last_docked);
		header.dispatch_singleton<type>(&red_alert,&pc::red_alert);
		header.dispatch_singleton<type>(&unknown43,&pc::unknown43);
		header.dispatch_singleton<type>(&mainscreen,&pc::mainscreen);
		header.dispatch_singleton<type>(&beam_freq,&pc::beam_freq);
		header.dispatch_singleton<type>(&coolant_or_missles,&pc::hermes_coolant_or_missiles);
		header.dispatch_singleton<type>(&science_target,&pc::hermes_science_target);
		header.dispatch_singleton<type>(&captians_target,&pc::hermes_captians_target);
		//byte 5
		header.dispatch_singleton<type>(&drive_type,&pc::hermes_drive_type);
		header.dispatch_singleton<type>(&scanning_target,&pc::hermes_scanning_target);
		header.dispatch_singleton<type>(&scanning_progress,&pc::scanning_progress);
		header.dispatch_singleton<type>(&reverse,&pc::reverse);
		header.dispatch_singleton<type>(&climb_dive,&pc::climb_dive);
		header.dispatch_singleton<type>(&game_side,&pc::hermes_game_side);
		header.dispatch_singleton<type>(&map_visability,&pc::hermes_map_visability);
		header.dispatch_singleton<type>(&ship_num,&pc::ship_num);
		//byte 6
		header.dispatch_singleton<type>(&capital_id,&pc::hermes_capital_id);
		header.dispatch_singleton<type>(&accent,&pc::accent);
		header.dispatch_singleton<type>(&combat_jump,&pc::hermes_combat_jump);
		header.dispatch_singleton<type>(&beacon_type,&pc::hermes_beacon_type);
		header.dispatch_singleton<type>(&beacon_mode,&pc::hermes_beacon_mode);
	}
};

class update_weapons_data {
public:
	static constexpr uint8_t bits{26};

	uint32_t id{0};
	std::array<std::optional<uint8_t>,8> max_ammo;
	std::array<std::optional<float>,6> unload_time;
	std::array<std::optional<uint8_t>,6> tube_status;
	std::array<std::optional<uint8_t>,6> tube_loaded_type;

	constexpr update_weapons_data () {
	}

	update_weapons_data(std::deque<std::byte>* in) {
		if (buffer::pop_front<uint8_t>(*in)!=2) {
			throw std::runtime_error("data stream error - internal type doesnt match external");
		}
		id=uint32_t{buffer::pop_front<uint32_t>(*in)};;
		bitfield_header<bits> bitfield{in};
		inner_dispatch<update_dispatch_type::inflate>(in,nullptr,&bitfield);
	}

	update_weapons_data(int id, const pc& real, const pc* last_transmitted) :
		id(id)
	{
		inner_dispatch<update_dispatch_type::diff>(nullptr,nullptr,nullptr,&real,last_transmitted);
	}

	// this techincally could be const but is annoyingly hard to do
	std::deque<std::byte> build_data() {
		bitfield_header<bits> bitfield;

		std::deque<std::byte> ret;
		inner_dispatch<update_dispatch_type::build>(&ret,nullptr,&bitfield);
		if (ret.size()!=0) {
			buffer::push_front(ret,bitfield.get_header());
			buffer::push_front<uint32_t>(ret,id);
			buffer::push_front<uint8_t>(ret,2);
		}
		return ret;
	}

	void merge_into(class pc* dest) {
		inner_dispatch<update_dispatch_type::merge>(nullptr,dest);
	}
private:
	template <const update_dispatch_type type> void inner_dispatch (std::deque<std::byte>* deq, pc* merge, bitfield_header<bits>* bitfield=nullptr, const pc* original=nullptr, const pc* preexisiting_data=nullptr) {
		static_assert(bitfield_header<bits>::bitfield_size==4);
		update_common<bits,update_weapons_data,pc> header{this,merge,bitfield,deq,preexisiting_data,original};
		header.dispatch_array<type>(&max_ammo,&pc::torp_ammo_count);
		header.dispatch_array<type>(&unload_time,&pc::tube_load_time);
		header.dispatch_array<type>(&tube_status,&pc::tube_status);
		header.dispatch_array<type>(&tube_loaded_type,&pc::tube_loaded_type);
	}
};

class update_engineering_data {
public:
	static constexpr uint8_t bits{24};

	uint32_t id{0};
	std::array<std::optional<float>,8> heat;
	std::array<std::optional<float>,8> power;
	std::array<std::optional<uint8_t>,8> coolant;

	constexpr update_engineering_data () {
	}

	update_engineering_data(std::deque<std::byte>* in) {
		if (buffer::pop_front<uint8_t>(*in)!=3) {
			throw std::runtime_error("data stream error - internal type doesnt match external");
		}
		id=uint32_t{buffer::pop_front<uint32_t>(*in)};;
		bitfield_header<bits> bitfield{in};
		inner_dispatch<update_dispatch_type::inflate>(in,nullptr,&bitfield);
	}

	update_engineering_data(int id, const pc& real, const pc* last_transmitted) :
		id(id)
	{
		inner_dispatch<update_dispatch_type::diff>(nullptr,nullptr,nullptr,&real,last_transmitted);
	}

	// this techincally could be const but is annoyingly hard to do
	std::deque<std::byte> build_data() {
		bitfield_header<bits> bitfield;

		std::deque<std::byte> ret;
		inner_dispatch<update_dispatch_type::build>(&ret,nullptr,&bitfield);
		if (ret.size()!=0) {
			buffer::push_front(ret,bitfield.get_header());
			buffer::push_front<uint32_t>(ret,id);
			buffer::push_front<uint8_t>(ret,3);
		}
		return ret;
	}

	void merge_into(class pc* dest) {
		inner_dispatch<update_dispatch_type::merge>(nullptr,dest);
	}
private:
	template <const update_dispatch_type type> void inner_dispatch (std::deque<std::byte>* deq, pc* merge, bitfield_header<bits>* bitfield=nullptr, const pc* original=nullptr, const pc* preexisiting_data=nullptr) {
		static_assert(bitfield_header<bits>::bitfield_size==4);
		update_common<bits,update_engineering_data,pc> header{this,merge,bitfield,deq,preexisiting_data,original};
		header.dispatch_array<type>(&heat,&pc::heat);
		header.dispatch_array<type>(&power,&pc::power);
		header.dispatch_array<type>(&coolant,&pc::coolant);
	}
};

class update_npc_data {
public:
	static constexpr uint8_t bits{49};

	uint32_t id{0};

	//byte 1
	std::optional<std::string> scanning_name;
	std::optional<float> throttle;
	std::optional<float> rudder;
	std::optional<float> top_speed;
	std::optional<float> turn_rate;
	std::optional<uint32_t> enemy;
	std::optional<uint32_t> visual_id;
	std::optional<float> x;
	//byte 2
	std::optional<float> y;
	std::optional<float> z;
	std::optional<float> pitch;
	std::optional<float> roll;
	std::optional<float> heading;
	std::optional<float> velocity;
	std::optional<uint8_t> surrendered;
	std::optional<uint8_t> nebula;
	//byte 3
	std::optional<float> front_current;
	std::optional<float> front_max;
	std::optional<float> rear_current;
	std::optional<float> rear_max;
	std::optional<uint16_t> unknown35;
	std::optional<uint8_t> fleet_num;
	std::optional<uint32_t> has_specials;
	std::optional<uint32_t> active_specials;
	//byte 4
	std::optional<uint32_t> single_scan;
	std::optional<uint32_t> double_scan;
	std::optional<uint32_t> map_vis;
	std::optional<uint8_t> side;
	std::optional<uint8_t> unknown45;
	std::optional<uint8_t> unknown46;
	std::optional<uint8_t> unknown47;
	std::optional<float> target_x;
	//byte 5
	std::optional<float> target_y;
	std::optional<float> target_z;
	std::optional<uint8_t> tagged;
	std::optional<uint8_t> unknown54;
	std::array<std::optional<float>,8> sys_damage;
	//byte 6
	std::array<std::optional<float>,5> beam_resist;
	//byte 7

	constexpr update_npc_data () {
	}

	update_npc_data(std::deque<std::byte>* in) {
		if (buffer::pop_front<uint8_t>(*in)!=5) {
			throw std::runtime_error("data stream error - internal type doesnt match external");
		}
		id=uint32_t{buffer::pop_front<uint32_t>(*in)};
		bitfield_header<bits> bitfield{in};
		inner_dispatch<update_dispatch_type::inflate>(in,nullptr,&bitfield);
	}

	update_npc_data(int id, const npc& real, const npc* last_transmitted) :
		id(id)
	{
		inner_dispatch<update_dispatch_type::diff>(nullptr,nullptr,nullptr,&real,last_transmitted);
	}

	// this techincally could be const but is annoyingly hard to do
	std::deque<std::byte> build_data() {
		bitfield_header<bits> bitfield;

		std::deque<std::byte> ret;
		inner_dispatch<update_dispatch_type::build>(&ret,nullptr,&bitfield);
		if (ret.size()!=0) {
			buffer::push_front(ret,bitfield.get_header());
			buffer::push_front<uint32_t>(ret,id);
			buffer::push_front<uint8_t>(ret,5);
		}
		return ret;
	}

	void merge_into(class npc* dest) {
		inner_dispatch<update_dispatch_type::merge>(nullptr,dest);
	}
private:
	template <const update_dispatch_type type> void inner_dispatch (std::deque<std::byte>* deq, npc* merge, bitfield_header<bits>* bitfield=nullptr, const npc* original=nullptr, const npc* preexisiting_data=nullptr) {
		static_assert(bitfield_header<bits>::bitfield_size==7);
		update_common<bits,update_npc_data,npc> header{this,merge,bitfield,deq,preexisiting_data,original};
		//byte 1
		header.dispatch_singleton<type>(&scanning_name,&npc::hermes_name);
		header.dispatch_singleton<type>(&throttle,&npc::throttle);
		header.dispatch_singleton<type>(&rudder,&npc::rudder);
		header.dispatch_singleton<type>(&top_speed,&npc::hermes_top_speed);//TODO early tests implied that if the hullid was changed then we needed to resend this as well (this included original creation)
		header.dispatch_singleton<type>(&turn_rate,&npc::hermes_turn_rate);//TODO see above
		header.dispatch_singleton<type>(&enemy,&npc::hermes_enemy);
		header.dispatch_singleton<type>(&visual_id,&npc::hermes_visual_id);
		header.dispatch_singleton<type>(&x,&npc::hermes_x);
		//byte 2
		header.dispatch_singleton<type>(&y,&npc::hermes_y);
		header.dispatch_singleton<type>(&z,&npc::hermes_z);
		header.dispatch_singleton<type>(&pitch,&npc::hermes_pitch);
		header.dispatch_singleton<type>(&roll,&npc::hermes_roll);
		header.dispatch_singleton<type>(&heading,&npc::hermes_heading);
		header.dispatch_singleton<type>(&velocity,&npc::hermes_velocity);
		header.dispatch_singleton<type>(&surrendered,&npc::surrendered);
		header.dispatch_singleton<type>(&nebula,&npc::hermes_nebula);
		//byte 3
		header.dispatch_singleton<type>(&front_current,&npc::hermes_front_current);
		header.dispatch_singleton<type>(&front_max,&npc::hermes_front_max);
		header.dispatch_singleton<type>(&rear_current,&npc::hermes_rear_current);
		header.dispatch_singleton<type>(&rear_max,&npc::hermes_rear_max);
		header.dispatch_singleton<type>(&unknown35,&npc::unknown_35);
		header.dispatch_singleton<type>(&fleet_num,&npc::hermes_fleet_num);
		header.dispatch_singleton<type>(&has_specials,&npc::hermes_has_specials);
		header.dispatch_singleton<type>(&active_specials,&npc::hermes_active_specials);
		//byte 4
		header.dispatch_singleton<type>(&single_scan,&npc::hermes_single_scan);
		header.dispatch_singleton<type>(&double_scan,&npc::hermes_double_scan);
		header.dispatch_singleton<type>(&map_vis,&npc::hermes_map_vis);
		header.dispatch_singleton<type>(&side,&npc::hermes_side);
		header.dispatch_singleton<type>(&unknown45,&npc::unknown_45);
		header.dispatch_singleton<type>(&unknown46,&npc::unknown_46);
		header.dispatch_singleton<type>(&unknown47,&npc::unknown_47);
		header.dispatch_singleton<type>(&target_x,&npc::hermes_target_x);
		//byte 5
		header.dispatch_singleton<type>(&target_y,&npc::hermes_target_y);
		header.dispatch_singleton<type>(&target_z,&npc::hermes_target_z);
		header.dispatch_singleton<type>(&tagged,&npc::hermes_tagged);
		header.dispatch_singleton<type>(&unknown54,&npc::unknown_54);
		header.dispatch_array<type>(&sys_damage,&npc::sys_damage);
		//byte 6
		header.dispatch_array<type>(&beam_resist,&npc::beam_resist);
		//byte 7
	}
};
