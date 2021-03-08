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
#include <d3d11.h>

#define material_white game::Material_RegisterHandle("white")

using namespace game;

utils::hook::detour sv_startmap_hook;
utils::hook::detour clientspawn_hook;
utils::hook::detour g_damage_hook;
utils::hook::detour sv_maprestart_hook;
utils::hook::detour bg_getsurfacepenetrationdepth_hook;
utils::hook::detour bullet_endpos_hook;
utils::hook::detour pm_weap_beginweapraise_hook;
utils::hook::detour pm_weaponprocesshand_hook;

vec3_t saved_location = { -1 };
vec3_t saved_angles = { -1 };
vec3_t saved_bot_location = { -1 };
const char* current_map_name = "";

dvar_t* ts_weaponMechanics;
dvar_t* ts_spawnWithRandomClass;
dvar_t* ts_showTips;
dvar_t* ts_botFreedomDist;
dvar_t* ts_equipmentCanKill;
dvar_t* ts_aimbotEnabled;
dvar_t* ts_aimbotHitDistance;
dvar_t* ts_noSpreadEnabled;

vec3_t from_bullet = { -1 };
vec3_t to_bullet = { -1 };

/*
	TODO List:
	---------------------
	iw4 glides
	perfect iw4 mechanics
	bounces
	slides
	s&d support
*/

static std::vector<const char*> ts_weaponMech_values =
{
	"default",
	"default (improved)",
	"mw2",
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


		bool pm_isadsallowed(mp::playerState_s** state) {
			mp::playerState_s* ppVar2 = *state;
			return (~(*(byte*)&ppVar2->pm_flags >> 2) & 1);
		}

		void PM_Weapon_BeginWeaponRaise(mp::playerState_s** ps_, unsigned int or , unsigned int p2, unsigned int p4, unsigned int p5, PlayerHandIndex handIdx) {

			auto* ps = *ps_;

			if (ts_weaponMechanics->current.integer == 2) {
				if (or != 0x12 && or != 0x13) {
					or = 0x01;
				}
				//ps->weapState[handIdx].weapAnim = ~ps->weapState[handIdx].weapAnim & 0x800 | 0x01;
			}
			pm_weap_beginweapraise_hook.invoke<void>(ps_, or , p2, p4, p5, handIdx);


			if (ts_weaponMechanics->current.integer == 2) {
				ps->weapState[handIdx].weaponDelay = 0;
			}
		}

		void print_debug_info(mp::playerState_s* ps) {
			game_console::print(0, "************ PM ***********");
			game_console::print(0, "pm_type: %d", ps->pm_type);
			game_console::print(0, "pm_flags: %d | 0x%X", ps->pm_flags, ps->pm_flags);
			game_console::print(0, "pm_time: %d", ps->pm_time);
			game_console::print(0, "************ WEAP ***********");
			for (int i = 0; i < 2; i++) {
				game_console::print(0, "weapState[%d].weapAnim: %d | 0x%X", i, ps->weapState[i].weapAnim, ps->weapState[i].weapAnim);
				game_console::print(0, "weapState[%d].weaponState: %d", i, ps->weapState[i].weaponState);
				game_console::print(0, "weapState[%d].weaponTime: %d", i, ps->weapState[i].weaponTime);
				game_console::print(0, "weapState[%d].weaponDelay: %d", i, ps->weapState[i].weaponDelay);
			}
		}

		void PM_WeaponProcessHand(mp::playerState_s** ps_, long long p2, unsigned int p3, PlayerHandIndex handIdx) {

			pm_weaponprocesshand_hook.invoke<void>(ps_, p2, p3, handIdx);

			if (ts_weaponMechanics->current.integer == 2) {
				auto ps = *ps_;

				// YY cancel -> shoot
				if (ps->weapState[handIdx].weapAnim == 0x1 || ps->weapState[handIdx].weaponState == 1) {
					ps->weapState[handIdx].weaponTime = 0.1f;
				}
			}

		}
	}

	namespace {

		float BG_GetSurfacePenetrationDepth(Weapon weap, bool p1, int p2) {
			// Penetrate through all
			return 9999.f;
		}

		bool call_Bot_CombatStateGrenade(unsigned int* p1, usercmd_s* cmd) {
			return 0;
		}

		void patch_bots() {
			utils::hook::nop(0x140463e52, 5); // Bot_UpdateThreat
			utils::hook::nop(0x140463e62, 5); // Bot_UpdateDistToEnemy
			utils::hook::set(0x140447d5f, "\xE9"); // Bot_CanSeeEnemy
			utils::hook::call(0x140454fed, call_Bot_CombatStateGrenade);
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
			command::execute(utils::string::va("spawnbot %d", Dvar_FindVar("sv_maxclients")->current.integer));
		}

		void SV_MapRestart(int p1, int p2) {
			sv_maprestart_hook.invoke<void>(p1, p2);
		}

		void G_Damage(mp::gentity_s* target, mp::gentity_s* inflictor, mp::gentity_s* attacker, vec3_t* dir, vec3_t* pt, int damage, int dflags, int meansOfDeath, Weapon weap, bool p10, enum hitLocation_t hitLoc, unsigned int p12, int p13, int p14) {
			if (BG_GetWeaponClass(weap, true) == 1 || (ts_equipmentCanKill->current.enabled && BG_GetWeaponClass(weap, true) == 9)) {
				damage = 9999;
			}
			else { return; }

			g_damage_hook.invoke<void>(target, inflictor, attacker, dir, pt, damage, dflags, meansOfDeath, weap, p10, hitLoc, p12, p13, p14);
		}

		void Bullet_Endpos(unsigned int* randSeed, float spread, float p3, float* endpoint, float* dir, float p6, float p7, weaponParms parms, float p9) {
			if (ts_noSpreadEnabled->current.enabled) {
				spread = 0.f;
				*randSeed = 0;
			}

			bullet_endpos_hook.invoke<void>(randSeed, spread, p3, endpoint, dir, p6, p7, parms, p9);
			
			if (ts_aimbotEnabled->current.enabled && BG_GetWeaponClass(parms.weapon, true) == 1) {
				float closest_distance = INFINITE;

				if (ts_aimbotHitDistance->current.value > 1) {
					closest_distance = ts_aimbotHitDistance->current.value;
				}

				mp::gentity_s closest_ent;
				bool has_closest = false;

				for (int i = 1; i < 32; i++) {
					mp::gentity_s ent = mp::g_entities[i];

					if (ent.client && ent.health > 0) {
						if (vecdist(endpoint, ent.r.origin) < closest_distance) {
							closest_distance = vecdist(endpoint, ent.r.origin);
							closest_ent = ent;
							has_closest = true;
						}
					}
				}

				if (has_closest) {
					veccpy(closest_ent.r.origin, endpoint);
				}
			}

			veccpy(endpoint, to_bullet);
			veccpy(parms.muzzleTrace, from_bullet);

			game_console::print(0, "From: %f, %f, %f", from_bullet[0], from_bullet[1], from_bullet[2]);
			game_console::print(0, "To: %f, %f, %f", to_bullet[0], to_bullet[1], to_bullet[2]);
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
			
			// Give class 50ms after spawn
			scheduler::once([entity]() {
				if (ts_spawnWithRandomClass->current.enabled && entity->health > 1)
					randomize_class();
			}, scheduler::pipeline::main, std::chrono::duration<int, std::milli>(50ms));

			if (entity->health > 0) {
				entity->client->flags ^= game::FL_GODMODE; // god mode
			}

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

				veczero(self.client->ps.velocity);
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
				const char* secondary = utils::string::va("%s_camo%d", random_secondary[rand() % 8], rand() % 18);
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
			g_damage_hook.create(0x140394df0, G_Damage);
			bg_getsurfacepenetrationdepth_hook.create(0x140238fd0, BG_GetSurfacePenetrationDepth);
			bullet_endpos_hook.create(0x1403762c0, Bullet_Endpos);
			// iw4 mechanics
			pm_weap_beginweapraise_hook.create(0x140231500, mechanics::PM_Weapon_BeginWeaponRaise);
			pm_weaponprocesshand_hook.create(0x140230be0, mechanics::PM_WeaponProcessHand);
			utils::hook::call(0x14022fc2f, mechanics::pm_isadsallowed);

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
			ts_aimbotEnabled = Dvar_RegisterBool("ts_aimbotEnabled", false, DVAR_FLAG_SAVED, "Enable trickshot aimbot (silent aim)");
			ts_aimbotHitDistance = Dvar_RegisterInt("ts_aimbotHitDistance", 0, 0, 5000, DVAR_FLAG_SAVED, "The distance the shot has to miss by to be taken over by the aimbot. 0 = disabled");
			ts_noSpreadEnabled = Dvar_RegisterBool("ts_noSpreadEnabled", false, DVAR_FLAG_SAVED, "Enable 100% accuracy.");

			localized_strings::override("LUA_MENU_PRIVATE_MATCH_LOBBY", "^:TRICKSHOT");
			localized_strings::override("LUA_MENU_PRIVATE_MATCH_CAPS", "^:TRICKSHOT");
			localized_strings::override("LUA_MENU_DESC_PRIVATE_MATCH", "Spin around in circles");

			// replace tips
			const char* tips[6] = {
				"Don't forget to bind ^3ts_saveLocation^7 and ^3ts_loadLocation^7 to a key!",
				"Get a random class with ^3ts_randomClass^7!",
				"Change the weapon mechanics with ^3ts_weaponMechanics^7!",
				"No luck? Try ^3ts_aimbotEnabled^7 ;)",
				"To make the bots stay in a certain place, use ^3ts_botSaveLocation^7!",
				"Remember, bullets can penetrate through pretty much everything."
			};

			for (int i = 0; i < 55; i++) {
				const char* tip_local = utils::string::va("PLATFORM_DYK_IW5_MSG%d", i);
				localized_strings::override(tip_local, tips[rand() % 6]);
			}

			// UI loop
			scheduler::loop([]() {
				if (game::CL_IsCgameInitialized()) {
					if (ts_showTips->current.enabled) {
						show_tips();
					}
				}

			}, scheduler::pipeline::renderer);
		}
	};
}

REGISTER_COMPONENT(trickshot::component)	
