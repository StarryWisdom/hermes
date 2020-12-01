#pragma once

#include <experimental/net>

#include <stdexcept>
#include <optional>
#include <iostream>
#include <fstream>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include "hermes.pb.h"

#include "core-lib/packet_buffer.h"

#include "hermes/hermes_gm_stats.h"
#include "hermes/hermes_socket.h"
#include "hermes/settings.h"

class hermes_gm_iface {
public:
	virtual ~hermes_gm_iface(){}
	virtual void tick (class hermes_gm_gui* gm,bool* close)=0;
};

class hermes_gm_gui {
public:
	class settings_window :public hermes_gm_iface {
	public:
		void tick(hermes_gm_gui* gm, bool* close) final {
			ImGui::Begin("hermes settings",close);
			for (auto [name,setting,read_only] : settings) {
				if (std::holds_alternative<bool>(setting)) {
					const bool orig_setting=std::get<bool>(setting);
					ImGui::Checkbox(name.c_str(),&std::get<bool>(setting));
					if (std::get<bool>(setting)!=orig_setting && !read_only) {
						gm->send_set_setting(name,std::get<bool>(setting));
					}
				} else if (std::holds_alternative<uint32_t>(setting)) {
					//we really should enable editing but thats later when its needed
					ImGui::Text("%s = %s",name.c_str(),std::to_string(std::get<uint32_t>(setting)).c_str());
				}
			}
			ImGui::End();
		};

		std::vector<setting_from_server> settings;
	};

	class connect_window :public hermes_gm_iface {
	public:
		virtual void tick(hermes_gm_gui* gm, bool* close) override final {
			ImGui::Begin("connection",close);
			if (gm->is_connected()) {
				ImGui::Text("Connected");
				if (ImGui::Button("disconnect")) {
					gm->socket.reset();
				}
			} else {
				ImGui::Text("Disconnected");
				std::string str;
				ImGui::InputText("host", hostname,1000);
				ImGui::InputText("port", port ,1000);
				if (ImGui::Button("connect")) {
					gm->connect(std::string(hostname),std::string(port));
					gm->reset_stats();
				}
			}
			ImGui::End();
		}
	private:
		char hostname[1024]{};
		char port[1024]{'2','0','1','5',0};
	};

	class log_window :public hermes_gm_iface {
	public:
		virtual void tick(hermes_gm_gui* gm, bool* close) override final {
			ImGui::Begin("logs",close);
			for (auto& a : log) {
				ImGui::Text("%s",a.c_str());
			}
			if (ImGui::Button("download server logs")) {
				gm->send_request_log();
			}
			ImGui::End();
		}
		void add_log_message(std::string const& str) {
			log.push_back(str);
		}
	private:
		std::vector<std::string> log;
	};

	class idle_window :public hermes_gm_iface {
	public:
		virtual void tick(hermes_gm_gui* gm, bool* close) override final {
			ImGui::Begin("idle",close);
			ImGui::InputText("from",from,1000);
			ImGui::InputText("message",message,1000);
			if (ImGui::Button("send")) {
				gm->send_idle_text(std::string(from),std::string(message));
			}
			ImGui::End();
		}
	private:
		char from[1024]{};
		char message[1024]{};
	};

	class stats_window :public hermes_gm_iface {
	public:
		virtual void tick(hermes_gm_gui* gm, bool* close) override final {
			ImGui::Begin("hermes stats",close);
			ImGui::Text("timing stats");
			for (const auto& i : gm->stats.with_sleep_buckets) {
				ImGui::Text("<=%i = %i",i.max,i.number);
			}
			ImGui::Text("------");
			for (const auto& i : gm->stats.without_sleep_buckets) {
				ImGui::Text("<=%i = %i",i.max,i.number);
			}
			ImGui::Text("------");
			for (const auto& i : gm->pc_dat) {
				ImGui::Text("pc:%i,%f,%f,%f",i.ship_num(),i.x(),i.y(),i.z());
			}
			ImGui::End();
		}
	};

	class popup_window :public hermes_gm_iface {
	public:
		virtual void tick(hermes_gm_gui* gm, bool* close) override final {
			ImGui::Begin("popup",close);

			std::string ship_cmd;
			std::string console_cmd;

			for (int i=1; i<=8; i++) {
				ImGui::Checkbox(std::string("ship"+std::to_string(i)).c_str(),&ship_bool[i-1]);
				if (ship_bool[i-1]) {
					ship_cmd+="popupShip"+std::to_string(i)+"|";
				}
			}

			for (auto& i : consoles) {
				ImGui::Checkbox(i.display.c_str(),&i.checked);
				if (i.checked) {
					console_cmd+=i.hermes+"|";
				}
			}

			ImGui::InputText("message",message,1000);

			if (ImGui::Button("send")) {
				ship_cmd+="0";
				console_cmd+="0";
				gm->send_popup(console_cmd,ship_cmd,message);
			}

			ImGui::End();
		}
	private:
		class console {
		public:
			const std::string display;
			const std::string hermes;
			bool checked;
		};
		std::array<bool,8> ship_bool{};
		char message[1024]{};
		std::vector<console> consoles{
			{"main screen","popupMainScreen",false},
			{"helm","popupHelm",false},
			{"weapons","popupWeapons",false},
			{"engineering","popupEngineering",false},
			{"science","popupScience",false},
			{"communications","popupCommunications",false},
			{"fighter","popupFighter",false},
			{"data","popupData",false},
			{"observer","popupObserver",false},
			{"captains","popupCaptainsMap",false},
			{"game master","popupGameMaster",false}};
	};

	class coms_window :public hermes_gm_iface {
	public:
		virtual void tick(hermes_gm_gui* gm, bool* close) override final {
			ImGui::Begin("coms",close);

			ImGui::Checkbox("alert (red)",&commsFilterAlert);
			ImGui::Checkbox("side (blue)",&commsFilterSide);
			ImGui::Checkbox("status (yellow)",&commsFilterStatus);
			ImGui::Checkbox("player (white)",&commsFilterPlayer);
			ImGui::Checkbox("station (green)",&commsFilterStation);
			ImGui::Checkbox("enemy (green)",&commsFilterEnemy);

			ImGui::Checkbox("friend",&commsFilterFriend);
			ImGui::InputText("from",from,1000);
			ImGui::InputText("message",message,1000);

			if (ImGui::Button("send")) {
				//lazy way to build string - we are going to or 0 into it (even though uneeded
				std::string filters;
				if (commsFilterAlert) {
					filters+="commsFilterAlert|";
				}
				if (commsFilterSide) {
					filters+="commsFilterSide|";
				}
				if (commsFilterStatus) {
					filters+="commsFilterStatus|";
				}
				if (commsFilterPlayer) {
					filters+="commsFilterPlayer|";
				}
				if (commsFilterStation) {
					filters+="commsFilterStation|";
				}
				if (commsFilterEnemy) {
					filters+="commsFilterEnemy|";
				}
				if (commsFilterFriend) {
					filters+="commsFilterFriend|";
				}
				filters+="0";
				gm->send_coms_text(filters,std::string(from),std::string(message));
			}
			ImGui::End();
		}
	private:
		bool commsFilterAlert{false};
		bool commsFilterSide{false};
		bool commsFilterStatus{false};
		bool commsFilterPlayer{false};
		bool commsFilterStation{false};
		bool commsFilterEnemy{false};
		bool commsFilterFriend{false};
		char from[1024]{};
		char message[1024]{};
	};

	class kick_window :public hermes_gm_iface {
	public:
		virtual void tick(hermes_gm_gui* gm, bool* close) override final {
			ImGui::Begin("kick", close);
			for (auto ship_num=1; ship_num<=8; ++ship_num) {
				std::string ship_name="ship "+std::to_string(ship_num);
				if (ImGui::CollapsingHeader(ship_name.c_str())) {
					if (ImGui::Button(std::string(ship_name+" helm").c_str())) {
						gm->send_kick_client(ship_num,"kickHelm");
					}
					if (ImGui::Button(std::string(ship_name+" weapons").c_str())) {
						gm->send_kick_client(ship_num,"kickWeapons");
					}
					if (ImGui::Button(std::string(ship_name+" engineering").c_str())) {
						gm->send_kick_client(ship_num,"kickEngineering");
					}
				}
			}
			if (ImGui::CollapsingHeader("other")) {
				if (ImGui::Button("gm")) {
					for (auto ship_num=1; ship_num<=8; ++ship_num) {
						gm->send_kick_client(ship_num,"kickGameMaster");
					}
				}
			}
			ImGui::End();
		}
	};

	class root_menu :public hermes_gm_iface {
	public:
		void tick(hermes_gm_gui* gm) {
			bool junk;
			tick(gm, &junk);
		}

	private:
		virtual void tick(hermes_gm_gui* gm, bool*) override final {
			ImGui::Begin("menus");//we dont display a close button as its bad to disable this
			ImGui::Checkbox("connect window",&show_connect_window);
			ImGui::Checkbox("settings window",&show_settings_window);
			ImGui::Checkbox("kick window",&show_kick_window);
			ImGui::Checkbox("idle window",&show_idle_window);
			ImGui::Checkbox("coms window",&show_coms_window);
			ImGui::Checkbox("popup window",&show_popup_window);
			ImGui::Checkbox("log",&show_log_window);
			ImGui::Checkbox("stats",&show_stats_window);
			ImGui::Checkbox("demo",&show_demo_window);
			ImGui::End();

			if (show_settings_window) {
				gm->settings_window.tick(gm, &show_settings_window);
			}
			if (show_connect_window) {
				gm->connect_window.tick(gm, &show_connect_window);
			}
			if (show_kick_window) {
				gm->kick_window.tick(gm, &show_kick_window);
			}
			if (show_idle_window) {
				gm->idle_window.tick(gm, &show_idle_window);
			}
			if (show_coms_window) {
				gm->coms_window.tick(gm, &show_coms_window);
			}
			if (show_popup_window) {
				gm->popup_window.tick(gm, &show_popup_window);
			}
			if (show_log_window) {
				gm->log_window.tick(gm, &show_log_window);
			}
			if (show_stats_window) {
				gm->stats_window.tick(gm, &show_stats_window);
			}
			if (show_demo_window) {
				ImGui::ShowDemoWindow(&show_demo_window);
			}
		}

		bool show_settings_window{false};
		bool show_connect_window{true};
		bool show_kick_window{false};
		bool show_coms_window{false};
		bool show_popup_window{false};
		bool show_log_window{false};
		bool show_demo_window{false};
		bool show_idle_window{false};
		bool show_stats_window{false};
	};

	class hermes_gm_window {
	public:
		hermes_gm_window() {
			glfwSetErrorCallback([](int error, char const * const description){
				throw std::runtime_error(std::string{"glfw error "+std::to_string(error)+description});
			});
			if (!glfwInit()) {
				throw std::runtime_error("glfw init failed");
			}
			window = glfwCreateWindow(1280, 720, "test window", nullptr, nullptr);
			glfwMakeContextCurrent(window);
			glfwSwapInterval(1); // Enable vsync
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			ImGui::StyleColorsDark();
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL2_Init();
		}

		~hermes_gm_window() {
			ImGui_ImplOpenGL2_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			glfwDestroyWindow(window);
			glfwTerminate();
		}

		void tick(hermes_gm_gui* gm) {
			gm->root.tick(gm);
		}

		void window_main_loop(hermes_gm_gui* gm) {
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
				ImGui_ImplOpenGL2_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				tick(gm);

				ImGui::Render();
				int display_w, display_h;
				glfwGetFramebufferSize(window, &display_w, &display_h);
				glViewport(0, 0, display_w, display_h);
				glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
				glClear(GL_COLOR_BUFFER_BIT);
				ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
				glfwMakeContextCurrent(window);
				glfwSwapBuffers(window);

				gm->network_tick();
			}
		}

		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	private:
		GLFWwindow* window;
	};

	bool is_connected () const {
		return socket.has_value();
	}

	void connect(std::string const& host, std::string const& port) {
		std::experimental::net::ip::tcp::resolver resolver(io_context);
		try {
			auto tmp = resolver.resolve(host,port);
			std::experimental::net::ip::tcp::socket sock(io_context);;
			std::experimental::net::connect(sock,tmp);
			socket=std::move(sock);
		} catch (std::exception& e) {
			log_window.add_log_message(std::string(e.what())+"\n");
		}
	}

	void send_command(const std::string& cmd) {
		try {
			if (!socket) {
				log_window.add_log_message("unable to send command - not connected");
				return;
			}
			socket->enqueue_write(cmd);
		} catch (std::exception& e) {
			log_window.add_log_message(std::string(e.what())+"\n");
		}
	}

	void send_kick_client(const uint8_t ship_num, const std::string& console) {
		std::string cmd="kickClients(kickShip"+std::to_string(ship_num)+","+console+");\n";
		send_command(cmd);
	}

	static std::string bool_to_lua(const bool val) {
		return (val) ? "true" : "false";
	}

	void send_set_setting(const std::string& setting_name, const bool setting_value) {
		std::string cmd="setSetting(\""+setting_name+"\","+bool_to_lua(setting_value)+");\n";
		send_command(cmd);
	}

	void send_request_log() {
		send_command("requestLog();\n");
	}

private:
	void replace_all(std::string& str,const std::string from, const std::string to) {
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
	}
public:

	std::string prepare_artemis_string(const std::string& in) {
		std::string ret=in;
		replace_all(ret,"\\","\\\\");
		replace_all(ret,"\n","^");
		replace_all(ret,"\"","\\\"");
		replace_all(ret,"\'","\\\'");
		ret="\""+ret+"\"";
		return ret;
	}

	void send_coms_text(const std::string& filters, const std::string& from, const std::string& msg) {
		std::string cmd="sendCommsTextAllClients("+filters+","+prepare_artemis_string(from)+","+prepare_artemis_string(msg)+");\n;";
		send_command(cmd);
	}

	void send_idle_text(const std::string& from, const std::string& msg) {
		// artemis strings of size 0 tend to crash artemis clients
		// I am not 100% sure that idle text with a size 0 will
		// I am also not 100% sure that hermes wont filter it
		// and ideally GM's wouldnt send that in the first place
		// but vs mass client crashes there is little harm in adding another filter in
		if (from.size()!=0 && msg.size()!=0) {
			std::string cmd="sendIdleTextAllClients("+prepare_artemis_string(from)+","+prepare_artemis_string(msg)+");\n";
			send_command(cmd);
			send_coms_text("0",from,msg);
		}
	}

	void send_popup(const std::string& console, const std::string& ship, const std::string& msg) {
		if (console.size()!=0 && ship.size()!=0 && msg.size()!=0) {
			std::string cmd="sendPopup("+console+","+ship+","+prepare_artemis_string(msg)+");\n";
			send_command(cmd);
		}
	}

	void network_tick() {
		try {
			if (socket.has_value()) {
				socket->attempt_write_buffer();
				if (!next_read_len) {
					auto tmp=socket->attempt_read(4);
					if (tmp.has_value()) {
						uint32_t val=*(reinterpret_cast<uint32_t*>(tmp->data()));
						next_read_len=val;
					}
				}
				if (next_read_len.has_value()) {
					auto tmp=socket->attempt_read(*next_read_len);
					if (tmp.has_value()) {
						next_read_len.reset();
						std::string str;
						str.resize(tmp->size());
						std::transform(tmp->begin(),tmp->end(),str.begin(),[] (std::byte c) {return char(c);});
						hermes_proto::hermes_server_msg protobuf;
						protobuf.ParseFromString(str);
						if (protobuf.has_stats()) {
							const auto server_stats=protobuf.stats();
							stats.deserialize(server_stats);
						}
						if (protobuf.has_log()) {
							try {
								const std::string filename{"hermes.server.log.txt"};
								std::ofstream log(filename);
								log << protobuf.log();
								log_window.add_log_message(std::string("server log saved to")+filename+"\n");
							} catch (std::runtime_error& e) {
								log_window.add_log_message(std::string("error in retriving log")+e.what()+"\n");
							}
						}
						pc_dat.clear();
						for (auto& i : protobuf.pc_data()) {
							pc_dat.push_back(i);
						}
						if (protobuf.settings_size()!=0) {
							settings_window.settings=settings::deserialize_from(protobuf);
						}
					}
				}
			}
		} catch (std::exception& e) {
			log_window.add_log_message(std::string("network tick error ")+e.what());
			socket.reset();
			reset_stats();
		}
	}

	void reset_stats() {
		stats=hermes_gm_stats();
		pc_dat.clear();
	}

	hermes_gm_window window;
	settings_window settings_window;
	root_menu root;
	connect_window connect_window;
	kick_window kick_window;
	coms_window coms_window;
	idle_window idle_window;
	log_window log_window;
	stats_window stats_window;
	popup_window popup_window;
	std::experimental::net::io_context io_context;
	std::optional<queued_nonblocking_socket> socket;
private:
	hermes_gm_stats stats;
	std::vector<hermes_proto::pc_data_v1> pc_dat;
	std::optional<uint32_t> next_read_len;
};
