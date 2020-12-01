#pragma once

#include <unordered_map>

#include "artemis-lib/bitstream.h"

class artemis_sent_data {
public:
	const pc* get_eng_data(uint32_t id) {
		if (pc_data.count(id)==0) {
			return nullptr;
		} else {
			return &pc_data.at(id);
		}
	}

	const npc* get_npc_data(uint32_t id) {
		if (npc_data.count(id)==0) {
			return nullptr;
		} else {
			return &npc_data.at(id);
		}
	}

	void update_player(update_player_data& update) {
		const auto id{update.id};
		if (!pc_data.count(id)) {
			pc_data.emplace(id,pc());
		}
		update.merge_into(&pc_data.at(id));
	}

	void update_eng(update_engineering_data& update) {
		const auto id{update.id};
		if (!pc_data.count(id)) {
			pc_data.emplace(id,pc());
		}
		update.merge_into(&pc_data.at(id));
	}

	void update_wep(update_weapons_data& update) {
		const auto id{update.id};
		if (!pc_data.count(id)) {
			pc_data.emplace(id,pc());
		}
		update.merge_into(&pc_data.at(id));
	}

	void update_npc(update_npc_data& update) {
		const auto id{update.id};
		if (!npc_data.count(id)) {
			npc_data.emplace(id,npc());
		}
		update.merge_into(&npc_data.at(id));
	}

	template <typename fn> void for_each_npc(fn f) const {
		for (const auto& [id, npc] : npc_data) {
			f(id,npc);
		}
	}

	template <typename fn> void for_each_pc(fn f) const {
		for (const auto& [id, pc] : pc_data) {
			f(id,pc);
		}
	}

	void clear_all_data() {
		pc_data.clear();
		npc_data.clear();
	}

	void delete_player_data(uint32_t id) {
		if (pc_data.count(id)!=0) {
			pc_data.erase(id);
		}
	}

	void delete_npc_data(uint32_t id) {
		if (npc_data.count(id)!=0) {
			npc_data.erase(id);
		}
	}

	std::unordered_map<uint32_t,pc> pc_data;//this probably wants to become private....
	std::unordered_map<uint32_t,npc> npc_data;//this probably wants to become private....
};
