#pragma once

#include <string_view>
#include "compile_time_crc.hpp"

class jam32 {
public:
	constexpr static uint32_t value(std::string_view str) {
		return ~crcdetail::compute(str.data(),str.length());
	};
};
