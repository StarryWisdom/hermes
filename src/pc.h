#pragma once

#include <math.h>
#include <stdint.h>
#include <string>

class pc {
public:
	//sigh these dont want to live here, but its a lot of work for hermes to do it properly
	float hermes_turn_rate{0};
	float hermes_top_speed{0};
	uint32_t hermes_visual_id{0};
	float hermes_x{0};
	float hermes_y{0};
	float hermes_z{0};
	float hermes_heading{0};
	float hermes_velocity{0};
	uint8_t hermes_neb_type{0};
	float hermes_pitch{0};
	float hermes_roll{0};
	float hermes_front_current{0};
	float hermes_front_max{0};
	float hermes_rear_current{0};
	float hermes_rear_max{0};
	std::string hermes_name{"test"};
	uint8_t hermes_coolant_or_missiles{8};
	uint8_t hermes_drive_type{0};//note this is pretty much jump_drive below
	uint8_t hermes_game_side{0};
	uint32_t hermes_map_visability{0};
	uint32_t hermes_capital_id{0};
	float hermes_combat_jump{0};
	uint8_t hermes_beacon_type{0};
	uint8_t hermes_beacon_mode{0};
	uint32_t hermes_last_docked{0};
	uint32_t hermes_captians_target{0};
	uint32_t hermes_wep_target{0};
	uint32_t hermes_science_target{0};
	uint32_t hermes_scanning_target{0};
	uint16_t hermes_shield_status{0};

	uint32_t unknown22{0};
	float unknown43{0};
	//todo total ship coolant
	float accent{0};
	uint8_t warp{0};
	std::vector<float> heat{0,0,0,0,0,0,0,0};
	std::vector<float> power{(1.0f/3.0f),(1.0f/3.0f),(1.0f/3.0f),(1.0f/3.0f),(1.0f/3.0f),(1.0f/3.0f),(1.0f/3.0f),(1.0f/3.0f)};
	std::vector<uint8_t> coolant{0,0,0,0,0,0,0,0};
	float throttle{0};
	uint32_t last_docked;
	uint32_t wep_target;
	uint32_t captians_target;
	uint32_t science_target;
	uint32_t scanning_target;
	uint32_t dock_tick_count{0};
	std::vector<uint8_t> torp_ammo_count{0,0,0,0,0,0,0,0};
	std::vector<uint8_t> torp_max_ammo{0,0,0,0,0,0,0,0};
	float energy{1000};
	float scanning_progress{0};
	bool reverse{0};
	float climb_dive{0};
	uint8_t beam_freq{0};
	float rudder{0.5f};
	uint8_t ship_num{0};
	bool shield_status{0};
	bool jump_drive;
	uint8_t mainscreen{0};
	bool auto_beams{1};
	bool red_alert{0};
	std::vector<float> tube_load_time{0,0,0,0,0,0};
	std::vector<uint8_t> tube_loaded_type{0,0,0,0,0,0};
	std::vector<uint8_t> tube_status{0,0,0,0,0,0};
};
