#include <string.h>
#include <thread>
#include <iostream>
#include "hermes/hermes.h"
#include "hermes/gm/hermes_gm_window.h"

uint16_t get_port_setting(const std::string& arg, const std::string& arg_name) {
	const std::string port_str(arg.substr(arg_name.length()));
	size_t offset;
	const uint16_t ret=stoi(port_str,&offset);
	if (offset!=port_str.length()) {
		throw std::runtime_error("error with commandline args");
	}
	return ret;
}

int main(int argc, char ** argv) {
//in an ideal world this would run
//GOOGLE_PROTOBUF_VERIFY_VERSION;
//around here, currently we need to fix CMake stuff to do this
//and thus we are not in an ideal world
	try {
		bool mode_hermes{true};//default
		bool mode_hermes_gm{false};
		uint16_t artemis_port{2010};
		std::optional<uint16_t> multiplexer_port;
		for (int i=1; i<argc; i++) {
			const std::string arg{argv[i]};
			if (arg=="--mode=hermes") {
				mode_hermes=true;
			} else if (arg=="--mode=hermes_gm") {
				mode_hermes_gm=true;
			} else if (arg.find("--artemis-port=")!=std::string::npos) {
				artemis_port=get_port_setting(arg,"--artemis-port=");
			} else if (arg.find("--multiplexer-port=")!=std::string::npos) {
				multiplexer_port=get_port_setting(arg,"--multiplexer-port=");
			} else {
				std::cout << "unknown argument " << argv[i] << "\n";
			}
		}
		if (mode_hermes_gm) {
			hermes_gm_gui gm;
			gm.window.window_main_loop(&gm);
			return 0;
		} else if (mode_hermes) { // mode_hermes defaults to true, thus this block needs to be last
			std::cout.setf(std::ios::unitbuf);
			std::cout << "hermes (build " << std::to_string(hermes::version) << ") - an artemis (v2.7 to v2.7.5) proxy server\n";
			std::cout << "------starting--------\nrun the server on port 2011(by default), connect to port " << artemis_port << " for artemis clients, (2015 for hermes gm clients)\n\n";
			hermes hermes{artemis_port,multiplexer_port};
			hermes.main_loop();
			return 0;
		}
	}
	catch (std::exception& error) {
		std::cout << "exception thrown - " << error.what() <<"\n";
		std::this_thread::sleep_for(std::chrono::seconds(10));
		return 1;
	}
}
