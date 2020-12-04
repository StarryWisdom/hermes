#include "settings.h"

#include "hermes.h"

void settings::serialize_into (hermes_proto::hermes_server_msg* msg, artemis_server_info& server_info) const {
	serialize_single(msg,"give gm enabled",!disable_give_me_gm,false); // this probably wants to be folded into other features
	serialize_single(msg,"engineering data fix",engineering_data_fix,false);
	serialize_single(msg,"num threads",num_threads,true);
	serialize_single(msg,"version",hermes::version,true);
	serialize_single(msg,"num pc",static_cast<uint32_t>(server_info.server_data.pc_data.size()),true);
	serialize_single(msg,"num npc",static_cast<uint32_t>(server_info.server_data.npc_data.size()),true);
}
