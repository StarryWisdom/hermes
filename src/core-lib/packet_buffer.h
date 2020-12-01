#pragma once

#include <string>
#include <cstring>
#include <stdint.h>
#include <vector>
#include <optional>
#include <type_traits>
#include <stdexcept>

class packet_buffer {
public:
	void write_artemis_string(std::string);
	void write_artemis_string(std::u16string);

	std::u16string read_artemis_string() {
		std::u16string str;
		auto count=read<uint32_t>();
		bool skip{false};
		for (uint32_t i=0; i!=count; i++) {
			uint16_t chr=read<uint16_t>();
			if (!skip) {
				if (chr!=0) {
					str+=chr;
				} else {
					skip=true;
				}
			}
		}
		return str;
	}

	//I am slightly concerned over having any type being writable
	//esp int - without specifying size
	//also the possiblity of sending pointers
	//I think this is better in code length than manually allowing
	//but may end up having to switch
	template <typename T> void write(const T& t) {
		if constexpr (std::is_same<std::decay<T>,std::decay<std::string>>::value) {
			copy_bytes(t.c_str(),t.size());
			write<uint8_t>(0);
		} else if constexpr (std::is_same<std::decay<T>,std::decay<class packet_buffer>>::value) {
			if (t.read_offset!=0) {
				throw std::runtime_error("attempted to copy a packet buffer mid buffer, currently this is undefined");
			}
			copy_bytes(t.buffer.data(),t.write_offset);
		} else {
			static_assert(std::is_standard_layout<T>::value);
			realloc_if_needed(sizeof(T));
			std::memcpy(buffer.data()+write_offset,&t,sizeof(T));
			write_offset += sizeof(T);
		}
	}

	void copy_bytes(const void*, uint32_t);

	template <typename T> std::optional<T> attempt_read() {
		if constexpr (std::is_same<std::decay<T>,std::decay<std::string>>::value) {
			std::string str;
			for (;;) {
				if (write_offset==read_offset) {
					return std::optional<T>();
				}
				char code=read<uint8_t>();
				if (code==0) {
					return str;
				}
				str+=code;
			}
		} else {
			static_assert(!std::is_same<std::decay<T>,std::decay<packet_buffer>>::value,"invalid trying to read packet_buffer from packet_buffer");
			if (read_offset+sizeof(T)>write_offset) {
				return std::optional<T>();
			}
			T ret;
			std::memcpy(&ret,buffer.data()+read_offset,sizeof(T));
			read_offset+=sizeof(T);
			return ret;
		}
	}

	template <typename T> T read() {
		const auto ret=attempt_read<T>();
		if (!ret) {
			throw std::runtime_error("insufficent data in packet buffer - malformed packet?");
		} else {
			return *ret;
		}
	}

	uint32_t write_offset{0};
	uint32_t read_offset{0};
	std::vector<std::byte> buffer;
	void realloc_if_needed(uint32_t size) {
		// this if really wants to go away, but its supprisingly hard to make go away
		if ((write_offset+size)>buffer.size()) {
			buffer.resize(write_offset+size);
		}
	}
};
