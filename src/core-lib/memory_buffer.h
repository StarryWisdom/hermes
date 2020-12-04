#pragma once

#include <type_traits>
#include <deque>
#include <cstring>
#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>

namespace buffer {

template <size_t size> void push_front_n(std::deque<std::byte>& queue,const std::array<std::byte,size>& data) {
	queue.insert(queue.begin(),data.begin(),data.end());
}

template <size_t size> void push_back_n(std::deque<std::byte>& queue,const std::array<std::byte,size>& data) {
	queue.insert(queue.end(),data.begin(),data.end());
}

template <typename T> void push_back(std::deque<std::byte>& queue, T val) {
	static_assert(std::is_standard_layout<T>::value);
	static_assert(std::is_trivial<T>::value);
	std::array<std::byte,sizeof(T)> arr;
	std::memcpy(&arr,&val,sizeof(T));
	push_back_n(queue,arr);
}

template <typename T> void push_front(std::deque<std::byte>& queue, T val) {
	static_assert(std::is_standard_layout<T>::value);
	static_assert(std::is_trivial<T>::value);
	std::array<std::byte,sizeof(T)> arr;
	std::memcpy(&arr,&val,sizeof(T));
	push_front_n(queue,arr);
}

template <size_t size> std::array<std::byte,size> read_at_n(const std::deque<std::byte>& queue, size_t start) {
	if (queue.size()<start+size) {
		throw std::runtime_error("invalid_call to read_at_n");
	}
	std::array<std::byte,size> arr;
	std::copy_n(queue.begin()+start,size,arr.begin());
	return arr;
}

template <size_t size> std::array<std::byte,size> peek_back_n(const std::deque<std::byte>& queue) {
	if (queue.size()<size) {
		throw std::runtime_error("invalid call to peek_back_n (not enough buffer)");
	}
	return read_at_n<size>(queue,queue.size()-size);
}

template <size_t size> std::array<std::byte,size> peek_front_n(const std::deque<std::byte>& queue) {
	return read_at_n<size>(queue,0);
}

template <typename T> T read_at(const std::deque<std::byte>& queue, size_t start) {
	static_assert(std::is_standard_layout<T>::value);
	static_assert(std::is_trivial<T>::value);
	std::array<std::byte,sizeof(T)> arr{read_at_n<sizeof(T)>(queue,start)};
	T val;
	std::memcpy(&val,&arr,sizeof(T));
	return val;
}

template <typename T> T peek_back(const std::deque<std::byte>& queue) {
	if (queue.size()<sizeof(T)) {
		throw std::runtime_error("invalid call to peek_back (not enough buffer)");
	}
	return read_at<T>(queue,queue.size()-sizeof(T));
}

template <typename T> T peek_front(const std::deque<std::byte>& queue) {
	return read_at<T>(queue,0);
}

template <typename T> T pop_back(std::deque<std::byte>& queue) {
	T tmp=peek_back<T>(queue);
	queue.erase(queue.end()-sizeof(T),queue.end());
	return tmp;
}

template <typename T> T pop_front(std::deque<std::byte>& queue) {
	T tmp=peek_front<T>(queue);
	queue.erase(queue.begin(),queue.begin()+sizeof(T));
	return tmp;
}

template <size_t size> std::array<std::byte,size> pop_front_n(std::deque<std::byte>& queue) {
	std::array<std::byte,size> tmp=pop_front<std::array<std::byte,size>>(queue);
	return tmp;
}

//TODO needs test
inline void write_artemis_string(std::deque<std::byte>& queue, const std::string& string) {
	push_back<uint32_t>(queue,string.size()+1);
	for (const auto& t : string) {
		push_back<uint16_t>(queue,t);
	}
	push_back<uint16_t>(queue,0);
}

//TODO needs test
inline std::string read_artemis_string_u8(std::deque<std::byte>& queue) {
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
