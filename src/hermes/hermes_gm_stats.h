#pragma once

#include "hermes.pb.h"

#include <vector>

class timing_bucket {
public:
	timing_bucket(uint32_t max, uint32_t number=0)
		: max(max)
		, number(number)
	{}

	uint32_t max;
	uint32_t number;
};

class hermes_gm_stats {
public:
	void deserialize(const hermes_proto::hermes_stats& buf) {
		with_sleep_buckets.clear();
		without_sleep_buckets.clear();
		for (const auto& i : buf.timing_with_sleep()) {
			with_sleep_buckets.push_back(timing_bucket(i.max(),i.number()));
		}
		for (const auto& i : buf.timing_without_sleep()) {
			without_sleep_buckets.push_back(timing_bucket(i.max(),i.number()));
		}
	}

	hermes_proto::hermes_stats serialize() const {
		hermes_proto::hermes_stats buf;
		for (const auto& i : with_sleep_buckets) {
			auto t=buf.add_timing_with_sleep();
			t->set_max(i.max);
			t->set_number(i.number);
		}
		for (const auto& i : without_sleep_buckets) {
			auto t=buf.add_timing_without_sleep();
			t->set_max(i.max);
			t->set_number(i.number);
		}
		return buf;
	}

	void increment_timing_stat_with_sleep(uint32_t cur) {
		for (auto& i : with_sleep_buckets) {
			if (cur<=i.max) {
				++i.number;
				return;
			}
		}
	}

	void increment_timing_stat_without_sleep(uint32_t cur) {
		for (auto& i : without_sleep_buckets) {
			if (cur<=i.max) {
				++i.number;
				return;
			}
		}
	}

	std::vector<timing_bucket> with_sleep_buckets{1,5,10,50,100,200,500,UINT32_MAX};
	std::vector<timing_bucket> without_sleep_buckets{1,5,10,50,100,200,500,UINT32_MAX};
};
