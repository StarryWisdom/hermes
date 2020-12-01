#include "packet_buffer.h"

#include <string.h>
#include <locale>
#include <assert.h>
#include "string-util.h"

void packet_buffer::write_artemis_string(std::u16string string) {
	write<uint32_t>(string.size()+1);
	copy_bytes((char *)string.c_str(),string.size()*2);
	write<uint16_t>(0);
}

void packet_buffer::write_artemis_string(std::string u8str) {
	std::u16string u16str=string_util::get_u16_string(u8str);
	write_artemis_string(u16str);
}

void packet_buffer::copy_bytes(const void* src, uint32_t len) {
	realloc_if_needed(len);
	if (len!=0) {
		memcpy((buffer.data()+write_offset),src,len);
	}
	write_offset+=len;
}
