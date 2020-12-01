#pragma once

#include "artemis_sent_data.h"

#include <string>

class artemis_server_info {
public:
	artemis_server_info(const std::string& server_name, const std::string& port)
		: server_name(server_name)
		, port(port)
	{}

	artemis_server_info() : artemis_server_info("127.0.0.1","2011") {};

	std::string server_name;
	std::string port;

	artemis_sent_data server_data;
};
