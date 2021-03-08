#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "command.hpp"
#include "game_console.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"
#include "filesystem.hpp"
#include "scheduler.hpp"

#include <utils/hook.hpp>
#include <utils/nt.hpp>

namespace patches
{
	namespace
	{
		utils::hook::detour live_get_local_client_name_hook;

		const char* live_get_local_client_name()
		{
			return game::Dvar_FindVar("name")->current.string;
		}

		utils::hook::detour sv_kick_client_num_hook;

		void sv_kick_client_num(const int clientNum, const char* reason)
		{
			// Don't kick bot to equalize team balance.
			if (reason == "EXE_PLAYERKICKED_BOT_BALANCE"s)
			{
				return;
			}
			return sv_kick_client_num_hook.invoke<void>(clientNum, reason);
		}

		utils::hook::detour dvar_register_int_hook;

		game::dvar_t* dvar_register_int(const char* name, int value, const int min, const int max,
		                                const unsigned int flags,
		                                const char* description)
		{
			// enable map selection in extinction
			if (!strcmp(name, "extinction_map_selection_enabled"))
			{
				value = true;
			}

				// enable extra loadouts
			else if (!strcmp(name, "extendedLoadoutsEnable"))
			{
				value = true;
			}

				// show all in-game store items
			else if (strstr(name, "igs_"))
			{
				value = true;
			}

			return dvar_register_int_hook.invoke<game::dvar_t*>(name, value, min, max, flags, description);
		}

		game::dvar_t* register_fovscale_stub(const char* name, float /*value*/, float /*min*/, float /*max*/,
		                                     unsigned int /*flags*/,
		                                     const char* desc)
		{
			// changed max value from 2.0f -> 5.0f and min value from 0.5f -> 0.1f
			return game::Dvar_RegisterFloat(name, 1.0f, 0.1f, 5.0f, 0x1, desc);
		}

		game::dvar_t* register_cg_gun_dvars(const char* name, float /*value*/, float /*min*/, float /*max*/,
		                                    unsigned int /*flags*/, const char* desc)
		{
			if (name == "cg_gun_x"s)
			{
				return game::Dvar_RegisterFloat(name, 0.0f, -1.0f, 2.0f, game::DvarFlags::DVAR_FLAG_SAVED, desc);
			}
			else
			{
				return game::Dvar_RegisterFloat(name, 0.0f, 0.0f, 0.0f, 0, desc);
			}
		}

		game::dvar_t* register_network_fps_stub(const char* name, int, int, int, unsigned int flags,
		                                        const char* desc)
		{
			return game::Dvar_RegisterInt(name, 1000, 20, 1000, flags, desc);
		}

		bool cmd_exec_patch()
		{
			const command::params exec_params{};
			if (exec_params.size() == 2)
			{
				std::string file_name = exec_params.get(1);
				if (file_name.find(".cfg") == std::string::npos)
					file_name.append(".cfg");

				const auto file = filesystem::file(file_name);
				if (file.exists())
				{
					game::Cbuf_ExecuteBufferInternal(0, 0, file.get_buffer().data(), game::Cmd_ExecuteSingleCommand);
					return true;
				}
			}

			return false;
		}

		auto cmd_exec_stub_mp = utils::hook::assemble([](utils::hook::assembler& a)
		{
			const auto success = a.newLabel();

			a.pushad64();
			a.call_aligned(cmd_exec_patch);
			a.test(al, al);
			a.popad64();

			a.jz(success);
			a.mov(edx, 0x18000);
			a.jmp(0x1403F7530);

			a.bind(success);
			a.jmp(0x1403F7574);
		});

		auto cmd_exec_stub_sp = utils::hook::assemble([](utils::hook::assembler& a)
		{
			const auto success = a.newLabel();

			a.pushad64();
			a.call_aligned(cmd_exec_patch);
			a.test(al, al);
			a.popad64();

			a.jz(success);
			a.mov(edx, 0x18000);
			a.jmp(0x1403B39C0);

			a.bind(success);
			a.jmp(0x1403B3A04);
		});

		int dvar_command_patch() // game makes this return an int and compares with eax instead of al -_-
		{
			const command::params args{};

			if (args.size() <= 0)
				return 0;

			auto* dvar = game::Dvar_FindVar(args.get(0));
			if (dvar)
			{
				if (args.size() == 1)
				{
					const auto current = game::Dvar_ValueToString(dvar, dvar->current);
					const auto reset = game::Dvar_ValueToString(dvar, dvar->reset);
					game_console::print(game_console::con_type_info, "\"%s\" is: \"%s^7\" default: \"%s^7\"",
					                    dvar->name, current, reset);
					game_console::print(game_console::con_type_info, "   %s\n",
					                    dvars::dvar_get_domain(dvar->type, dvar->domain).data());
				}
				else
				{
					char command[0x1000] = {0};
					game::Dvar_GetCombinedString(command, 1);
					game::Dvar_SetCommand(args.get(0), command);
				}

				return 1;
			}

			return 0;
		}

		game::Font_s* get_chat_font_handle()
		{
			return game::R_RegisterFont("fonts/bigFont");
		}

		void aim_assist_add_to_target_list(void* a1, void* a2)
		{
			if (!dvars::aimassist_enabled->current.enabled)
				return;

			game::AimAssist_AddToTargetList(a1, a2);
		}

		game::dvar_t* register_cg_fov_stub(const char* name, float value, float min, float /*max*/,
		                                   const unsigned int flags,
		                                   const char* description)
		{
			return game::Dvar_RegisterFloat(name, value, min, 160, flags | 1, description);
		}

		void bsp_sys_error_stub(const char* error, const char* arg1)
		{
			if (game::environment::is_dedi() || game::environment::is_linker())
			{
				game::Sys_Error(error, arg1);
			}
			else
			{
				game::Com_Error(game::ERR_DROP, error, arg1);
			}
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			// Increment ref-count on these
			LoadLibraryA("PhysXDevice64.dll");
			LoadLibraryA("PhysXUpdateLoader64.dll");

			command::add("quit", []()
			{
				game::Com_Quit();
			});

			command::add("crash", []()
			{
				*reinterpret_cast<int*>(1) = 0;
			});

			command::add("quit_hard", []()
			{
				utils::nt::raise_hard_exception();
			});

			// Unlock fps in main menu
			utils::hook::set<BYTE>(SELECT_VALUE(0x140242DDB, 0x1402CF58B), 0xEB);

			// Unlock cg_fov
			utils::hook::call(SELECT_VALUE(0x1401F3E96, 0x14027273C), register_cg_fov_stub);
			if (game::environment::is_sp())
			{
				utils::hook::call(0x1401F3EC7, register_cg_fov_stub);
			}

			// set it to 3 to display both voice dlc announcers did only show 1
			game::Dvar_RegisterInt("igs_announcer", 3, 3, 3, 0x0,
			                       "Show Announcer Packs. (Bitfield representing which announcer paks to show)");

			// changed max value from 85 -> 1000
			if (!game::environment::is_dedi() && !game::environment::is_linker())
			{
				game::Dvar_RegisterInt("com_maxfps", 85, 0, 1000, 0x1, "Cap frames per second");
			}

			if (!game::environment::is_sp())
			{
				//increased max limit for sv_network_fps, the lower limit is the default one. Original range is from 20 to 200 times a second.
				utils::hook::call(0x140476F4F, register_network_fps_stub);
			}

			// register cg_gun_ dvars with new values and flags
			// maybe _x can stay usable within a reasonable range? it can make scoped weapons DRASTICALLY better on high FOVs
			utils::hook::call(SELECT_VALUE(0x140228DDE, 0x1402AB04C), register_cg_gun_dvars);
			utils::hook::call(SELECT_VALUE(0x140228E0E, 0x1402AB07C), register_cg_gun_dvars);
			utils::hook::call(SELECT_VALUE(0x140228E3E, 0x1402AB0AC), register_cg_gun_dvars);

			// Register cg_fovscale with new params
			utils::hook::call(SELECT_VALUE(0x140317079, 0x140272777), register_fovscale_stub);

			// Patch Dvar_Command to print out values how CoD4 does it
			utils::hook::jump(SELECT_VALUE(0x1403BFCB0, 0x140416A60), dvar_command_patch);

			command::add("dvarDump", []()
			{
				game_console::print(game_console::con_type_info,
				                    "================================ DVAR DUMP ========================================\n");
				for (auto i = 0; i < *game::dvarCount; i++)
				{
					const auto dvar = game::sortedDvars[i];
					if (dvar)
					{
						game_console::print(game_console::con_type_info, "%s \"%s\"\n", dvar->name,
						                    game::Dvar_ValueToString(dvar, dvar->current));
					}
				}
				game_console::print(game_console::con_type_info, "\n%i dvar indexes\n", *game::dvarCount);
				game_console::print(game_console::con_type_info,
				                    "================================ END DVAR DUMP ====================================\n");
			});

			command::add("commandDump", []()
			{
				game_console::print(game_console::con_type_info,
				                    "================================ COMMAND DUMP =====================================\n");
				game::cmd_function_s* cmd = (*game::cmd_functions);
				int i = 0;
				while (cmd)
				{
					if (cmd->name)
					{
						game_console::print(game_console::con_type_info, "%s\n", cmd->name);
						i++;
					}
					cmd = cmd->next;
				}
				game_console::print(game_console::con_type_info, "\n%i command indexes\n", i);
				game_console::print(game_console::con_type_info,
				                    "================================ END COMMAND DUMP =================================\n");
			});

			command::add("weapDump", []() {
				game_console::print(game_console::con_type_info,
					"================================ WEAP DUMP =====================================\n");

				// 141734de0 = bg_weaponCompleteDefs
				// size = 296

				for (int i = 0; i < 512; i++) {
					game::WeaponCompleteDef* weap	= game::mp::bg_weaponCompleteDefs[i];
					if (weap != nullptr && weap->szInternalName != nullptr) {
						game_console::print(game_console::con_type_info, "%s\n", weap->szInternalName);
					}
				}

				game_console::print(game_console::con_type_info,
					"================================ END WEAP DUMP =====================================\n");
			});

			// Allow executing custom cfg files with the "exec" command
			utils::hook::jump(SELECT_VALUE(0x1403B39BB, 0x1403F752B), SELECT_VALUE(0x1403B3A12, 0x1403F7582));
			//Use a relative jump to empty memory first
			utils::hook::jump(SELECT_VALUE(0x1403B3A12, 0x1403F7582), SELECT_VALUE(cmd_exec_stub_sp, cmd_exec_stub_mp),
			                  true);
			//Use empty memory to go to our stub first (can't do close jump, so need space for 12 bytes)

			// Fix mouse lag
			utils::hook::nop(SELECT_VALUE(0x14043E6CB, 0x140504A2B), 6);
			scheduler::loop([]()
			{
				SetThreadExecutionState(ES_DISPLAY_REQUIRED);
			}, scheduler::pipeline::main);

			// Allow kbam input when gamepad is enabled
			utils::hook::nop(SELECT_VALUE(0x14023D490, 0x1402C3099), 2);
			utils::hook::nop(SELECT_VALUE(0x14023B3AC, 0x1402C0CE0), 6);

			if (game::environment::is_sp())
			{
				patch_sp();
			}
			else
			{
				patch_mp();
			}
		}

		static void patch_mp()
		{
			// Use name dvar and add "saved" flags to it
			utils::hook::set<uint8_t>(0x1402C836D, 0x01);
			live_get_local_client_name_hook.create(0x1404FDAA0, &live_get_local_client_name);

			// Patch SV_KickClientNum
			sv_kick_client_num_hook.create(0x14046F730, &sv_kick_client_num);

			// block changing name in-game
			utils::hook::set<uint8_t>(0x140470300, 0xC3);

			// Unlock all patches/cardtitles and exclusive items/camos
			utils::hook::set(0x140402B10, 0xC301B0); // LiveStorage_IsItemUnlockedFromTable_LocalClient
			utils::hook::set(0x140402360, 0xC301B0); // LiveStorage_IsItemUnlockedFromTable
			utils::hook::set(0x1404A94E0, 0xC301B0); // GetIsCardTitleUnlocked

			// Enable DLC items, extra loadouts and map selection in extinction
			dvar_register_int_hook.create(0x1404EE270, &dvar_register_int);

			// patch game chat on resolutions higher than 1080p to use the right font
			utils::hook::call(0x14025C825, get_chat_font_handle);
			utils::hook::call(0x1402BC42F, get_chat_font_handle);
			utils::hook::call(0x1402C3699, get_chat_font_handle);

			dvars::aimassist_enabled = game::Dvar_RegisterBool("aimassist_enabled", true,
			                                                   game::DvarFlags::DVAR_FLAG_SAVED,
			                                                   "Enables aim assist for controllers");
			//client side aim assist dvar
			utils::hook::call(0x14013B9AC, aim_assist_add_to_target_list);

			// patch "Couldn't find the bsp for this map." error to not be fatal in mp
			utils::hook::call(0x14031E8AB, bsp_sys_error_stub);
		}

		static void patch_sp()
		{
			// SP doesn't initialize WSA
			WSADATA wsa_data;
			WSAStartup(MAKEWORD(2, 2), &wsa_data);
		}
	};
}

REGISTER_COMPONENT(patches::component)
