#include "memory_buffer.h"
#include <iostream>

namespace buffer {
//TODO needs test
void write_artemis_string(std::deque<std::byte>& queue, const std::string& string) {
	push_back<uint32_t>(queue,string.size()+1);
	for (const auto& t : string) {
		push_back<uint16_t>(queue,t);
	}
	push_back<uint16_t>(queue,0);
}

//TODO needs test
std::string read_artemis_string_u8(std::deque<std::byte>& queue) {
	std::string str;
	auto count=pop_front<uint32_t>(queue);
	bool skip{false};
	for (uint32_t i=0; i!=count; i++) {
		uint8_t chr=pop_front<uint16_t>(queue);
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
}
