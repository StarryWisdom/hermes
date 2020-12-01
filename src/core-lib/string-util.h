#pragma once

#include <codecvt>

class string_util {
public:
	static std::u16string get_u16_string(const std::string str) {
#ifdef _MSC_VER
//may be able to get rid of this when the windows stdlib is updated
//it seems to be a known bug in visual studio
//currently it will compile but wont link
//see https://stackoverflow.com/questions/32055357/visual-studio-c-2015-stdcodecvt-with-char16-t-or-char32-t
		std::wstring_convert<std::codecvt_utf8_utf16<int16_t>,int16_t> conversion;
		auto tmp_windows_str = conversion.from_bytes(str);
		std::u16string ret = reinterpret_cast<const char16_t *>(tmp_windows_str.data());
#else
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> conversion;
		std::u16string ret=conversion.from_bytes(str);
#endif
		return ret;
	}

	static std::string get_u8_string(const std::u16string str) {
#ifdef _MSC_VER
		std::wstring tmp_str;// = reinterpret_cast<const wchar_t *>(str.data());
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conversion;
		auto ret = conversion.to_bytes(tmp_str);
#else
		std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> conversion;
		std::string ret=conversion.to_bytes(str);
#endif
		return ret;
	}
};
