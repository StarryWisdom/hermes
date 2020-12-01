#pragma once

#include "hermes.pb.h"

#include "artemis_server_info.h"

#include <variant>
#include <vector>
#include <string>
#include <array>

class setting_from_server {
public:
	template <typename T> setting_from_server(std::string name, T val, bool read_only)
		: name(name)
		, val(val)
		, read_only(read_only)
	{}

	std::string name;
	std::variant<bool,uint32_t> val;
	bool read_only;
};

class settings {
public:
//	std::array<bool,8> ship_locked = { false,false,false,false,false,false,false,false };
//	std::array<std::string,8> default_ship_name{ "disconnected","disconnected", "disconnected", "disconnected", "disconnected", "disconnected", "disconnected", "disconnected" };
	bool disable_give_me_gm{false};
	// do we fix up missing engineering data, this will probably merge into a larger feature in time
	bool engineering_data_fix{true};
	uint32_t num_threads{0};

	void serialize_into (hermes_proto::hermes_server_msg* msg, artemis_server_info&) const;

	static auto deserialize_from (const hermes_proto::hermes_server_msg& msg) {
		std::vector<setting_from_server> ret;
		for (const auto& i : msg.settings()) {
			if (i.data().has_boolean()) {
				ret.push_back(setting_from_server(i.name(),i.data().boolean(),i.read_only()));
			} else if (i.data().has_u32()) {
				ret.push_back(setting_from_server(i.name(),i.data().u32(),i.read_only()));
			}
		}
		return ret;
	}

	void set_setting(const std::string name, const bool enabled) {
		if (name=="give gm enabled") {
			disable_give_me_gm=!enabled;
		} else if (name=="engineering data fix") {
			engineering_data_fix=enabled;
		} else {
			throw std::runtime_error(std::string("unknown command")+name);
		}
	}

private:
	void serialize_single (hermes_proto::hermes_server_msg* msg, const std::string& name, const bool enabled, const bool read_only) const {
		auto entry=msg->add_settings();
		entry->set_name(name);
		entry->mutable_data()->set_boolean(enabled);
		entry->set_read_only(read_only);
	}

	void serialize_single (hermes_proto::hermes_server_msg* msg, const std::string& name, const uint32_t val, const bool read_only) const {
		auto entry=msg->add_settings();
		entry->set_name(name);
		entry->mutable_data()->set_u32(val);
		entry->set_read_only(read_only);
	}
};
