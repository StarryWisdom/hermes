#pragma once

class npc {
public:
	//sigh these dont want to live here, but its a lot of work for hermes to do it properly
	std::string hermes_name{""};
	float hermes_top_speed{0};
	float hermes_turn_rate{0};
	uint32_t hermes_enemy{0};
	float hermes_x{0};
	float hermes_y{0};
	float hermes_z{0};
	uint32_t hermes_visual_id{0};
	float hermes_pitch{0};
	float hermes_roll{0};
	float hermes_heading{0};
	float hermes_velocity{0};
	uint8_t hermes_nebula{0};
	float hermes_front_current{0};
	float hermes_front_max{0};
	float hermes_rear_current{0};
	float hermes_rear_max{0};
	uint8_t hermes_fleet_num{0};
	uint32_t hermes_has_specials{0};
	uint32_t hermes_active_specials{0};
	uint32_t hermes_single_scan{0};
	uint32_t hermes_double_scan{0};
	uint32_t hermes_map_vis{0};
	uint8_t hermes_side{0};
	float hermes_target_x{0};
	float hermes_target_y{0};
	float hermes_target_z{0};
	uint8_t hermes_tagged{0};

	uint16_t unknown_35{0};
	uint8_t unknown_45{0};
	uint8_t unknown_46{0};
	uint8_t unknown_47{0};
	uint8_t unknown_54{0};

	std::vector<float> sys_damage{0,0,0,0,0,0,0,0};
	std::vector<float> beam_resist{1,1,1,1,1};
	float throttle=0;
	float rudder{0.5f};
	float pitch{0};
	float roll{0};
	uint8_t surrendered{0};
};
