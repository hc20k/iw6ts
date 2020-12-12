#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/scripting/entity.hpp"
#include "game/scripting/execution.hpp"

#include <utils/hook.hpp>

#include "command.hpp"

namespace logfile
{
	namespace
	{
		bool evaluate_say(char* text, game::mp::gentity_s* ent)
		{
			auto hidden = false;

			const auto level = scripting::entity(*game::levelEntityId);
			const auto player = scripting::call("getEntByNum", {ent->client->ps.clientNum}).as<scripting::entity>();

			++text;

			if (text[0] == '/')
			{
				hidden = true;
				++text;
			}

			scripting::notify(level, "say", {player, text});
			scripting::notify(player, "say", {text});

			return hidden;
		}
	}

	const auto say_stub = utils::hook::assemble([](utils::hook::assembler& a)
	{
		const auto hidden = a.newLabel();

		a.call_aligned(0x1404F63C0);

		a.mov(rdx, rdi);
		a.mov(rcx, rbx);

		a.call_aligned(evaluate_say);

		a.cmp(rax, 0);
		a.jne(hidden);

		a.lea(rcx, byte_ptr(rsp, 0x80));
		a.jmp(0x140392A92);

		a.bind(hidden);
		a.jmp(0x140392C6E);
	});

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp())
			{
				return;
			}

			utils::hook::jump(0x140392A85, say_stub, true);
		}
	};
}

REGISTER_COMPONENT(logfile::component)