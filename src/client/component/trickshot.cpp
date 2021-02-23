#include "std_include.hpp"
#include "trickshot.hpp"
#include "game_console.hpp"
#include "game/game.hpp"
#include "loader/component_loader.hpp"
#include "localized_strings.hpp"
#include "utils/hook.hpp"
#include "utils/string.hpp"
#include "command.hpp"
#include "scheduler.hpp"

#define material_white game::Material_RegisterHandle("white")

using namespace game;

utils::hook::detour sv_startmap_hook;
utils::hook::detour clientspawn_hook;
utils::hook::detour bg_getdamage_hook;
utils::hook::detour sv_maprestart_hook;
utils::hook::detour bg_getsurfacepenetrationdepth_hook;

vec3_t saved_location = { -1 };
vec3_t saved_angles = { -1 };
vec3_t saved_bot_location = { -1 };
const char* current_map_name = "";

dvar_t* ts_weaponMechanics;
dvar_t* ts_spawnWithRandomClass;
dvar_t* ts_showTips;
dvar_t* ts_botFreedomDist;
dvar_t* ts_equipmentCanKill;


static std::vector<const char*> ts_weaponMech_values =
{
	"iw6",
	"iw6 (improved)",
	nullptr,
};

namespace trickshot {

	namespace mechanics {
		int lastWeapState = 0;
		void ghosted_mechanics(mp::gentity_s* ent) {
			PlayerActiveWeaponState* weapState = ent->client->ps.weapState;

			if (weapState->weaponState == 0x1 || weapState->weaponState == 0x2 || weapState->weaponState == 0x5) {
				weapState->weaponState = 0x0;
				weapState->weaponTime = 0.1f;
			}

			if (lastWeapState != weapState->weaponState) {
				if (lastWeapState != 0 && lastWeapState != 25 && lastWeapState != 24 && weapState->weaponState == 3) {
					weapState->weaponTime = 0.1f;
				}
			}

			lastWeapState = weapState->weaponState;
		}
	}

	namespace {
		float BG_GetSurfacePenetrationDepth(Weapon weap, bool p1, int p2) {
			// Penetrate through all
			return 9999.f;
		}

		void patch_bots() {
			utils::hook::nop(0x140463e52, 5); // Bot_UpdateThreat
			utils::hook::nop(0x140463e62, 5); // Bot_UpdateDistToEnemy
			utils::hook::set(0x140447d5f, "\xE9"); // Bot_CanSeeEnemy
			//utils::hook::set(0x1404494f6, "\xEB"); // Bot_CombatStateGrenade
		}

		void pregame_setup() {
			command::execute("onlinegame 1", true);
			command::execute("exec default_xboxlive.cfg", true);
			command::execute("xstartprivateparty", true);
			command::execute("xblive_rankedmatch 1", true);
			command::execute("xblive_privatematch 1", true);
			command::execute("startentitlements", true);
			command::execute("g_gametype dm", true);
			command::execute("ui_gametype dm", true);
			command::execute("scr_dm_scorelimit 1", true);
			command::execute("scr_dm_timelimit 0", true);
		}

		void SV_StartMapForParty(int p1, char* mapname, bool p3, bool p4) {
			pregame_setup();

			if (strcmp(current_map_name, mapname) != 0) {
				// reset values on map change
				saved_location[0] = -1;
				saved_angles[0] = -1;
				saved_bot_location[0] = -1;
				current_map_name = mapname;
			}

			sv_startmap_hook.invoke<void>(p1, mapname, p3, p4);
			command::execute(utils::string::va("spawnbot %d", Dvar_FindVar("party_maxplayers")->current.integer));
		}

		void SV_MapRestart(int p1, int p2) {
			sv_maprestart_hook.invoke<void>(p1, p2);
		}

		int BG_GetDamage(Weapon weap, bool p2) {
			auto weaponClass = reinterpret_cast<unsigned int(*)(Weapon, char)>(0x140240e20)(weap, p2); // BG_GetWeaponClass

			if (weaponClass == 1) {
				// Sniper
				return 9999;
			}

			if (ts_equipmentCanKill->current.enabled && weaponClass == 9) {
				// Equipment
				return 9999;
			}

			return -1;
		}

		void on_host_death(mp::gentity_s* entity) {
			game_console::print(0, "host died!");
		}

		void show_tips() {
			const auto x = 35;
			const auto y = 35;
			const auto scale = 0.7f;
			float bg[4] = { 0, 0, 0, 0.7f };
			float color[4] = { 1, 1, 1, 1 };

			auto* normalFont = game::R_RegisterFont("fonts/normalfont");
			auto* objFont = game::R_RegisterFont("fonts/objectivefont");
			auto* consoleFont = game::R_RegisterFont("fonts/consolefont");

			if (!normalFont || !objFont || !consoleFont) return;

			game::R_AddCmdDrawStretchPic(x-5, y-5, 460, y + 210, 0.0f, 0.0f, 0.0f, 0.0f, bg, material_white);

			game::R_AddCmdDrawText("^:IW6TS^7 by @hc20k", 0x7FFFFFFF, objFont, x, y + static_cast<float>(objFont->pixelHeight)* scale, scale, scale, 0.0f, color, 0);

			game::R_AddCmdDrawText("^3Commands", 0x7FFFFFFF, objFont, x, y + 35 + static_cast<float>(objFont->pixelHeight)* 0.5f, 0.5f, 0.5f, 0.0f, color, 0);

			game::R_AddCmdDrawText("^2Save Location^7 - ^6ts_saveLocation", 0x7FFFFFFF, normalFont, x, y + 60 + static_cast<float>(normalFont->pixelHeight)* scale, scale, scale, 0.0f, color, 0);
			game::R_AddCmdDrawText("^2Load Location^7 - ^6ts_loadLocation", 0x7FFFFFFF, normalFont, x, y + 80 + static_cast<float>(normalFont->pixelHeight)* scale, scale, scale, 0.0f, color, 0);
			game::R_AddCmdDrawText("^2Save Bot Location^7 - ^6ts_botSaveLocation", 0x7FFFFFFF, normalFont, x, y + 100 + static_cast<float>(normalFont->pixelHeight) * scale, scale, scale, 0.0f, color, 0);
			game::R_AddCmdDrawText("^2Random Class^7 - ^6ts_randomClass", 0x7FFFFFFF, normalFont, x, y + 120 + static_cast<float>(normalFont->pixelHeight) * scale, scale, scale, 0.0f, color, 0);

			game::R_AddCmdDrawText("Use ^2bind ^5[key] ^6[command]^7 for easy use.", 0x7FFFFFFF, normalFont, x, y + 150 + static_cast<float>(normalFont->pixelHeight)* scale, scale, scale, 0.0f, color, 0);

			game::R_AddCmdDrawText("Set ^3ts_showTips^7 to ^10^7 to hide this message.", 0x7FFFFFFF, normalFont, x, y + 190 + static_cast<float>(normalFont->pixelHeight)* scale, scale, scale, 0.0f, color, 0);
		}

		void on_host_spawned(mp::gentity_s* entity) {
			entity->client->flags ^= 1; // god mode
			
			// Give class 50ms after spawn
			scheduler::once([]() {
				if (ts_spawnWithRandomClass->current.enabled)
					randomize_class();
			}, scheduler::pipeline::main, std::chrono::duration<int, std::milli>(50ms));

			// Main loop
			scheduler::schedule([entity]() {
				if (entity->health < 1) {
					on_host_death(entity);
					return scheduler::cond_end;
				}

				// Unlimited ammo
				game::Add_Ammo(&entity->client->ps, entity->client->ps.weapon, false, 1, 0); //Weapon ammo
				game::Add_Ammo(&entity->client->ps, entity->client->ps.offHand, false, 1, 1); //Offhand ammo

				switch (ts_weaponMechanics->current.integer) {
					case 1: {
						// improved
						mechanics::ghosted_mechanics(entity);
					}

					default: {
						break;
					}
				}

				return scheduler::cond_continue;

			}, scheduler::pipeline::main, std::chrono::duration<int, std::milli>(50ms));
		}

		void on_bot_spawned(mp::gentity_s* entity) {
			// Main loop
			scheduler::schedule([entity]() {
				if (entity->health < 1) {
					return scheduler::cond_end;
				}

				if (saved_bot_location[0] != -1) {
					if (vecdist(entity->r.origin, saved_bot_location) > ts_botFreedomDist->current.value) {
						veccpy(saved_bot_location, entity->client->ps.origin);
					}
				}
				return scheduler::cond_continue;
			}, scheduler::pipeline::main, std::chrono::duration<int, std::milli>(50ms));
		}

		void ClientSpawn(mp::gentity_s* entity, float* origin, float* angles) {

			if (entity->s.clientNum == 0) {
				// is host
				if (saved_location[0] != -1 && saved_angles[0] != -1) {
					veccpy(saved_location, origin);
					veccpy(saved_angles, angles);
				}
				on_host_spawned(entity);
			}

			if (game::SV_BotIsBot(entity->s.clientNum)) {
				// is bot
				if (saved_bot_location[0] != -1) {
					veccpy(saved_bot_location, origin);
				}
				on_bot_spawned(entity);
			}

			clientspawn_hook.invoke<void>(entity, origin, angles);
		}

		void teleport_bots() {
			if (saved_bot_location[0] == -1) {
				return;
			}

			// TP all bots
			for (int i = 0; i < 32; i++) {
				mp::gentity_s ent = mp::g_entities[i];
				if (ent.client && SV_BotIsBot(i)) {
					veccpy(saved_bot_location, ent.client->ps.origin);
				}
			}
		}

		void save_location() {
			mp::gentity_s self = mp::g_entities[0];

			if (self.client) {
				game::SV_GameSendServerCommand(self.s.clientNum, 1, "e \"^2Location saved.\"");

				veccpy(self.r.origin, saved_location);
				veccpy(self.r.angles, saved_angles);
			}
		}

		void load_location() {
			mp::gentity_s self = mp::g_entities[0];

			if (self.client) {

				if (saved_location[0] == -1) {
					game::SV_GameSendServerCommand(self.s.clientNum, 1, "e \"^1No location saved!\"");
					return;
				}

				veccpy(saved_location, self.client->ps.origin);
				veccpy(saved_angles, self.client->ps.viewangles);
			}
		}

		void save_bot_location() {
			mp::gentity_s self = mp::g_entities[0];

			if (self.client) {
				game::SV_GameSendServerCommand(self.s.clientNum, 1, "e \"^2Bot location saved.\"");
				veccpy(self.r.origin, saved_bot_location);
				teleport_bots();
			}
		}

		void randomize_class() {
			mp::gentity_s self = mp::g_entities[0];

			if (self.client) {
				playerState_s* ps = game::SV_GetPlayerstateForClientNum(0);

				// Take all weapons
				for (int i = 0; i < 15; i++) {
					Weapon weap = self.client->ps.weaponsEquipped[i];
					if (weap.data) {
						game::G_TakePlayerWeapon(ps, weap);
					}
				}

				const char* primary = utils::string::va("%s_camo%d",random_primary[rand() % 4], rand() % 18);
				const char* secondary = utils::string::va("%s_camo%d", random_secondary[rand() % 7], rand() % 18);
				const char* offhand = "throwingknife_mp";

				game_console::print(0, "Giving primary: %s | sec: %s", primary, secondary);

				Weapon loadout[3] = {
					game::G_GetWeaponForName(primary),
					game::G_GetWeaponForName(secondary),
					game::G_GetWeaponForName(offhand)
				};

				for (int i = 0; i < 3; i++) {
					Weapon weap = loadout[i];
					game::G_GivePlayerWeapon(ps, weap, 0, 0, 0);
					game::G_InitializeAmmo(ps, weap, 0);
				}

				game::G_SelectWeapon(0, loadout[0]);
			}
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			sv_startmap_hook.create(0x1404702F0, SV_StartMapForParty);
			sv_maprestart_hook.create(0x14046F3B0, SV_MapRestart);
			clientspawn_hook.create(0x140387b20, ClientSpawn);
			bg_getdamage_hook.create(0x14023e260, BG_GetDamage);
			bg_getsurfacepenetrationdepth_hook.create(0x140238fd0, BG_GetSurfacePenetrationDepth);

			patch_bots();

			command::add("ts_saveLocation", []() {
				save_location();
			});

			command::add("ts_loadLocation", []() {
				load_location();
			});

			command::add("ts_clearLocation", []() {
				saved_location[0] = -1;
			});

			command::add("ts_randomClass", []() {
				randomize_class();
			});

			command::add("ts_botSaveLocation", []() {
				save_bot_location();
			});

			command::add("ts_botClearLocation", []() {
				saved_bot_location[0] = -1;
			});

			command::add("suicide", []() {
				mp::gentity_s* ent = &mp::g_entities[0];

				// TODO: fix crash

				if (ent->client) {
					game::player_die(ent,ent,ent, 100000, 0xe, 0, false, (float*)0x0, HITLOC_NONE, 0);
				}
			});

			ts_weaponMechanics = Dvar_RegisterEnum("ts_weaponMechanics", ts_weaponMech_values.data(), 0, DVAR_FLAG_SAVED, "Change the weapon movement mechanics.");
			ts_spawnWithRandomClass = Dvar_RegisterBool("ts_spawnWithRandomClass", false, DVAR_FLAG_SAVED, "Spawn with a random trickshotting class.");
			ts_showTips = Dvar_RegisterBool("ts_showTips", true, DVAR_FLAG_SAVED, "Show tutorial.");
			ts_botFreedomDist = Dvar_RegisterFloat("ts_botFreedomDist", 300.f, 0.f, 1500.f, DVAR_FLAG_SAVED, "The distance the bots can stray away from the saved location without being teleported.");
			ts_equipmentCanKill = Dvar_RegisterBool("ts_equipmentCanKill", true, DVAR_FLAG_SAVED, "Equipment (e.g. throwing knife) will kill enemies.");

			localized_strings::override("LUA_MENU_PRIVATE_MATCH_LOBBY", "^:TRICKSHOT");
			localized_strings::override("LUA_MENU_PRIVATE_MATCH_CAPS", "^:TRICKSHOT");
			localized_strings::override("LUA_MENU_DESC_PRIVATE_MATCH", "Spin around in circles");

			// replace tips
			for (int i = 0; i < 55; i++) {
				const char* tip_local = utils::string::va("PLATFORM_DYK_IW5_MSG%d", i);
				localized_strings::override(tip_local, "Don't forget to bind ^2ts_saveLocation^7 and ^2ts_loadLocation^7 to a key!");
			}

			// UI loop
			scheduler::loop([]() {
				if (game::CL_IsCgameInitialized() && ts_showTips->current.enabled) {
					show_tips();
				}
			}, scheduler::pipeline::renderer);
		}
	};
}

REGISTER_COMPONENT(trickshot::component)	
