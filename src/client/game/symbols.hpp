#pragma once

#define WEAK __declspec(selectany)

namespace game
{
	/***************************************************************
	 * Functions
	 **************************************************************/

	WEAK symbol<void(int type, VariableUnion u)> AddRefToValue{0x1403D7740, 0x1404326E0};
	WEAK symbol<void(int type, VariableUnion u)> RemoveRefToValue{0x1403D90F0, 0x1404340C0};

	WEAK symbol<void(void*, void*)> AimAssist_AddToTargetList{0, 0x140139D80};

	WEAK symbol<void ()> Com_Frame_Try_Block_Function{0x1403BC980, 0x1404131A0};
	WEAK symbol<const char*(char const**)> Com_Parse{0x1404313E0, 0x1404F50E0};
	WEAK symbol<void (errorParm code, const char* message, ...)> Com_Error{0x1403BBFF0, 0x140412740};
	WEAK symbol<void ()> Com_Quit{0x1403BDDD0, 0x140414920};
	WEAK symbol<CodPlayMode ()> Com_GetCurrentCoDPlayMode{0, 0x1404f6140};
	WEAK symbol<void (float, float, int)> Com_SetSlowMotion{0, 0x1404158C0};
	WEAK symbol<void(const char* text_in)> Com_TokenizeString{0x1403B4150, 0x1403F7CC0};
	WEAK symbol<void()> Com_EndTokenizeString{0x1403B37C0, 0x1403F7330};

	WEAK symbol<void (const char* message)> Conbuf_AppendText{0x14043DDE0, 0x1405028C0};

	WEAK symbol<void (int localClientNum, const char* text)> Cbuf_AddText{0x1403B3050, 0x1403F6B50};
	WEAK symbol<void (int localClientNum, int controllerIndex, const char* buffer,
	                  void (int, int, const char*))> Cbuf_ExecuteBufferInternal{0x1403B3160, 0x1403F6C60};

	WEAK symbol<bool ()> CL_IsCgameInitialized{0x140234DA0, 0x1402B9A70};

	WEAK symbol<void (int localClientNum, const char* message)> CG_GameMessage{0x1401F2E20, 0x140271320};
	WEAK symbol<void (int localClientNum, mp::cg_s* cg, const char* dvar, const char* value)> CG_SetClientDvarFromServer
		{0x0, 0x14028A2C0};

	WEAK symbol<void (const char* cmdName, void (), cmd_function_s* allocedCmd)> Cmd_AddCommandInternal{
		0x1403B3570, 0x1403F7070
	};
	WEAK symbol<void (int localClientNum, int controllerIndex, const char* text)> Cmd_ExecuteSingleCommand{
		0x1403B3B10, 0x1403F7680
	};

	WEAK symbol<void (XAssetType type, void (__cdecl *func)(XAssetHeader, void*), void* inData, bool includeOverride)>
	DB_EnumXAssets_FastFile{0x140271F50, 0x14031EF90};
	WEAK symbol<int (XAssetType type)> DB_GetXAssetTypeSize{0x14024FB30, 0x1402FB180};
	WEAK symbol<void(XZoneInfo* zoneInfo, unsigned int zoneCount, DBSyncMode syncMode)> DB_LoadXAssets{
		0x140273FD0, 0x140320F20
	};
	WEAK symbol<void(void* buffer, int size)> DB_ReadXFileUncompressed{0x140250FB0, 0x1402FC9C0};

	WEAK symbol<dvar_t*(const char* name)> Dvar_FindVar{0x140429E70, 0x1404ECB60};
	WEAK symbol<void (char* buffer, int index)> Dvar_GetCombinedString{0x1403BFD80, 0x140416B30};
	WEAK symbol<const char*(const char* dvarName, const char* defaultValue)> Dvar_GetVariantStringWithDefault{
		0x14042A240, 0x1404ECF90
	};
	WEAK symbol<dvar_t*(const char* dvarName, bool value, unsigned int flags, const char* description)>
	Dvar_RegisterBool{0x14042AF10, 0x1404EDD60};
	WEAK symbol<dvar_t*(const char* dvarName, const char** valueList, int defaultIndex, unsigned int flags,
	                    const char* description)> Dvar_RegisterEnum{0x14042B220, 0x1404EE070};
	WEAK symbol<dvar_t*(const char* dvarName, float value, float min, float max, unsigned int flags,
	                    const char* description)> Dvar_RegisterFloat{0x14042B330, 0x1404EE180};
	WEAK symbol<dvar_t*(const char* dvarName, int value, int min, int max, unsigned int flags, const char* desc)>
	Dvar_RegisterInt{0x14042B420, 0x1404EE270};
	WEAK symbol<dvar_t*(const char* dvarName, const char* value, unsigned int flags, const char* description)>
	Dvar_RegisterString{0x14042B7A0, 0x1404EE660};
	WEAK symbol<dvar_t*(const char* dvarName, float x, float y, float min, float max, unsigned int flags,
	                    const char* description)> Dvar_RegisterVec2{0x14042B880, 0x1404EE740};
	WEAK symbol<dvar_t*(const char* dvarName, float x, float y, float z, float w, float min, float max,
	                    unsigned int flags, const char* description)> Dvar_RegisterVec4{0x14042BC10, 0x1404EEA50};
	WEAK symbol<void (dvar_t* dvar, DvarSetSource source)> Dvar_Reset{0x14042C150, 0x1404EF020};
	WEAK symbol<void (dvar_t* dvar, bool value)> Dvar_SetBool{0x14042C370, 0x1404EF1A0};
	WEAK symbol<void (const char* dvar, const char* buffer)> Dvar_SetCommand{0x14042C8E0, 0x1404EF790};
	WEAK symbol<void (dvar_t* dvar, const char* string)> Dvar_SetString{0x14042D6E0, 0x1404F08E0};
	WEAK symbol<void (const char*, const char*, DvarSetSource)> Dvar_SetFromStringByNameFromSource{
		0x14042D000, 0x1404F00B0
	};
	WEAK symbol<void ()> Dvar_Sort{0x14042DEF0, 0x1404F1210};
	WEAK symbol<const char*(dvar_t* dvar, dvar_value value)> Dvar_ValueToString{0x14042E710, 0x1404F1A30};

	WEAK symbol<long long (const char* qpath, char** buffer)> FS_ReadFile{0x14041D0B0, 0x1404DE900};
	WEAK symbol<void (void* buffer)> FS_FreeFile{0x14041D0A0, 0x1404DE8F0};

	WEAK symbol<Weapon(const char* pickupName, int model)> G_FindItem{0x140462490, 0x14021B7E0};
	WEAK symbol<int (playerState_s* ps, Weapon weapon, int dualWield, int startInAltMode, int usedBefore)>
	G_GivePlayerWeapon{0x140359E20, 0x1403DA5E0};
	WEAK symbol<Weapon(const char* name)> G_GetWeaponForName{0x140359890, 0x1403DA060};
	WEAK symbol<void()> G_Glass_Update{0x14030E680, 0x140397450};
	WEAK symbol<void (playerState_s* ps, Weapon weapon, int hadWeapon)> G_InitializeAmmo{0x140311F00, 0x14039AEA0};
	WEAK symbol<void(int clientNum, Weapon weapon)> G_SelectWeapon{0x14035A200, 0x1403DA880};
	WEAK symbol<int(playerState_s* ps, Weapon weapon)> G_TakePlayerWeapon{0x14035A350, 0x1403DA9C0};
	WEAK symbol<unsigned int (const char* name, /*ConfigString*/ unsigned int start, unsigned int max, int create,
	                          const char* errormsg)> G_FindConfigstringIndex{0x0, 0x140161F90};
	WEAK symbol<int (int server_time)> G_RunFrame{0x0, 0x1403A05E0};

	WEAK symbol<game_hudelem_s*(int clientNum, int teamNum)> HudElem_Alloc{0x0, 0x1403997E0};

	WEAK symbol<char*(char* string)> I_CleanStr{0x140432460, 0x1404F63C0};

	WEAK symbol<const char*(int, int, int)> Key_KeynumToString{0x14023D9A0, 0x1402C40E0};

	WEAK symbol<unsigned int (int)> Live_SyncOnlineDataFlags{0, 0x1405ABF70};

	WEAK symbol<void (int clientNum, const char* menu, int a3, int a4, unsigned int a5)> LUI_OpenMenu{
		0x0, 0x1404B3610
	};

	WEAK symbol<bool (int clientNum, const char* menu)> Menu_IsMenuOpenAndVisible{0x0, 0x1404B38A0};

	WEAK symbol<Material*(const char* material)> Material_RegisterHandle{0x140523D90, 0x1405F0E20};

	WEAK symbol<void (netsrc_t, netadr_s*, const char*)> NET_OutOfBandPrint{0, 0x14041D5C0};
	WEAK symbol<void (netsrc_t sock, int length, const void* data, const netadr_s* to)> NET_SendLoopPacket{
		0, 0x14041D780
	};
	WEAK symbol<bool (const char* s, game::netadr_s* a)> NET_StringToAdr{0, 0x14041D870};

	WEAK symbol<void (float x, float y, float width, float height, float s0, float t0, float s1, float t1,
	                  float* color, Material* material)> R_AddCmdDrawStretchPic{0x140234460, 0x140600BE0};
	WEAK symbol<void (const char*, int, Font_s*, float, float, float, float, float, float*, int)> R_AddCmdDrawText{
		0x140533E40, 0x140601070
	};
	WEAK symbol<void (const char*, int, Font_s*, float, float, float, float, float, const float*, int, int, char)>
	R_AddCmdDrawTextWithCursor{0x140534170, 0x1406013A0};
	WEAK symbol<Font_s*(const char* font)> R_RegisterFont{0x1405130B0, 0x1405DFAC0};
	WEAK symbol<void()> R_SyncRenderThread{0x140535AF0, 0x140602D30};
	WEAK symbol<int (const char* text, int maxChars, Font_s* font)> R_TextWidth{0x140513390, 0x1405DFDB0};

	WEAK symbol<ScreenPlacement*()> ScrPlace_GetViewPlacement{0x14024D150, 0x1402F6D40};

	WEAK symbol<void ()> GScr_LoadConsts{0x140367AA0, 0x1403E0420};
	WEAK symbol<unsigned int (unsigned int parentId, unsigned int name)> FindVariable{0x1403D84F0, 0x1404334A0};
	WEAK symbol<scr_string_t (unsigned int parentId, unsigned int id)> GetVariableName{0x1403D8E90, 0x140433E60};
	WEAK symbol<void (VariableValue* result, unsigned int classnum, int entnum, int offset)> GetEntityFieldValue{
		0x1403DC810, 0x140437860
	};
	WEAK symbol<const float *(const float* v)> Scr_AllocVector{0x1403D9AF0, 0x140434A10};
	WEAK symbol<const char*(int index)> Scr_GetString{0, 0x140439160};
	WEAK symbol<float (int index)> Scr_GetFloat{0, 0x140438D60};
	WEAK symbol<int ()> Scr_GetNumParam{0x1403DDF60, 0x140438EC0};
	WEAK symbol<void ()> Scr_ClearOutParams{0x1403DD500, 0x140438600};
	WEAK symbol<scr_entref_t (unsigned int entId)> Scr_GetEntityIdRef{0x1403DBDC0, 0x140436E10};
	WEAK symbol<int (unsigned int classnum, int entnum, int offset)> Scr_SetObjectField{0x140350E70, 0x1403D3FE0};
	WEAK symbol<void (unsigned int id, scr_string_t stringValue, unsigned int paramcount)> Scr_NotifyId{
		0x1403DE730, 0x140439700
	};

	WEAK symbol<const char*(const char*)> SEH_StringEd_GetString{0x0, 0x1404A5F60};

	WEAK symbol<const char*(scr_string_t stringValue)> SL_ConvertToString{0x1403D6870, 0x1404317F0};
	WEAK symbol<scr_string_t(const char* str, unsigned int user)> SL_GetString{0x1403D6CD0, 0x140431C70};


	WEAK symbol<void (const char* text_in)> SV_Cmd_TokenizeString{0, 0x1403F8150};
	WEAK symbol<void ()> SV_Cmd_EndTokenizedString{0, 0x1403F8110};

	WEAK symbol<void (netadr_s* from)> SV_DirectConnect{0, 0x140471390};
	WEAK symbol<void (int, int, const char*)> SV_GameSendServerCommand{0x140490F40, 0x1404758C0};
	WEAK symbol<bool ()> SV_Loaded{0x140491820, 0x1404770C0};
	WEAK symbol<void (int localClientNum, const char* map, bool mapIsPreloaded)> SV_StartMap{0, 0x140470170};
	WEAK symbol<void (int localClientNum, const char* map, bool mapIsPreloaded, bool migrate)> SV_StartMapForParty{
		0, 0x1404702F0
	};
	WEAK symbol<mp::gentity_s*(const char*, unsigned int, unsigned int, unsigned int)> SV_AddBot{0, 0x140470920};
	WEAK symbol<bool (int clientNum)> SV_BotIsBot{0, 0x140461340};
	WEAK symbol<void (mp::client_t*, const char*, int)> SV_ExecuteClientCommand{0, 0x140472430};
	WEAK symbol<void ()> SV_FastRestart{0x14048B890, 0x14046F440};
	WEAK symbol<playerState_s*(int num)> SV_GetPlayerstateForClientNum{0x140490F80, 0x140475A10};
	WEAK symbol<const char*(int clientNum)> SV_GetGuid{0, 0x140475990};
	WEAK symbol<void (int clientNum, const char* reason)> SV_KickClientNum{0, 0x14046F730};
	WEAK symbol<void (int index, const char* string)> SV_SetConfigstring{0, 0x140477450};
	WEAK symbol<void (mp::gentity_s*)> SV_SpawnTestClient{0, 0x1404740A0};

	WEAK symbol<void (const char* error, ...)> Sys_Error{0x14043AC20, 0x1404FF510};
	WEAK symbol<bool ()> Sys_IsDatabaseReady2{0x1403C2D40, 0x140423920};
	WEAK symbol<int ()> Sys_Milliseconds{0x14043D2A0, 0x140501CA0};
	WEAK symbol<void()> Sys_ShowConsole{0x14043E650, 0x140503130};
	WEAK symbol<bool (int, void const*, const netadr_s*)> Sys_SendPacket{0x14043D000, 0x140501A00};
	WEAK symbol<void*(int valueIndex)> Sys_GetValue{0x1403C2C30, 0x1404237D0};

	WEAK symbol<void ()> SwitchToCoreMode{0, 0x1401FA4A0};
	WEAK symbol<void ()> SwitchToAliensMode{0, 0x1401FA4D0};
	WEAK symbol<void ()> SwitchToSquadVsSquadMode{0, 0x1401FA500};

	WEAK symbol<const char*(const char*)> UI_LocalizeMapname{0, 0x1404B96D0};
	WEAK symbol<const char*(const char*)> UI_LocalizeGametype{0, 0x1404B90F0};

	WEAK symbol<DWOnlineStatus (int)> dwGetLogOnStatus{0, 0x140589490};

	WEAK symbol<void*(jmp_buf* Buf, int Value)> longjmp{0x14062E030, 0x140738060};
	WEAK symbol<int (jmp_buf* Buf)> _setjmp{0x14062F030, 0x140739060};

	WEAK symbol<int (mp::playerState_s*, int, bool, int stock, int clip)> Add_Ammo{ 0, 0x140399e50 };
	WEAK symbol<void(mp::gentity_s*, mp::gentity_s*, mp::gentity_s*, int, int, unsigned int, bool, float*, int, int)> player_die{ 0, 0x140396920 };
	WEAK symbol<unsigned int(Weapon, char)> BG_GetWeaponClass{ 0, 0x140240e20 };

	/***************************************************************
	 * Variables
	 **************************************************************/

	WEAK symbol<int> keyCatchers{0x1417CF6E0, 0x1419E1ADC};

	WEAK symbol<CmdArgs> cmd_args{0x144CE7F70, 0x144518480};
	WEAK symbol<CmdArgs> sv_cmd_args{0x144CE8020, 0x144518530};
	WEAK symbol<cmd_function_s*> cmd_functions{0x144CE80C8, 0x1445185D8};

	WEAK symbol<int> dvarCount{0x1458CBA3C, 0x1478EADF4};
	WEAK symbol<dvar_t*> sortedDvars{0x1458CBA60, 0x1478EAE10};

	WEAK symbol<PlayerKeyState> playerKeys{0x14164138C, 0x1419DEABC};

	WEAK symbol<SOCKET> query_socket{0, 0x147AD1A78};

	WEAK symbol<const char*> command_whitelist{0x14086AA70, 0x1409E3AB0};

	WEAK symbol<unsigned int> levelEntityId{0x1452A9F30, 0x144A43020};
	WEAK symbol<int> g_script_error_level{0x1455B1F98, 0x144D535C4};
	WEAK symbol<jmp_buf> g_script_error{0x1455BA5E0, 0x144D536E0};
	WEAK symbol<scr_classStruct_t> g_classMap{0x140873E20, 0x1409EBFC0};

	WEAK symbol<void*> DB_XAssetPool{0x14086DCB0, 0x1409E4F20};
	WEAK symbol<int> g_poolSize{0x14086DBB0, 0x1409E4E20};

	WEAK symbol<scrVarGlob_t> scr_VarGlob{0x1452CDF80, 0x144A67080};
	WEAK symbol<scrVmPub_t> scr_VmPub{0x1455B1FA0, 0x144D4B090};
	WEAK symbol<unsigned int> scr_levelEntityId{0x1452A9F30, 0x144A43020};

	WEAK symbol<DWORD> threadIds{0x144DE6640, 0x1446B4960};

	WEAK symbol<GfxDrawMethod_s> gfxDrawMethod{0x145F525A8, 0x1480350D8};

	namespace sp
	{
		WEAK symbol<gentity_s> g_entities{0x143C91600, 0};
	}

	namespace mp
	{
		WEAK symbol<cg_s> cgArray{0, 0x14176EC00};

		WEAK symbol<gentity_s> g_entities{0, 0x14427A0E0};

		WEAK symbol<int> svs_numclients{0, 0x14647B28C};
		WEAK symbol<client_t> svs_clients{0, 0x14647B290};

		WEAK symbol<std::uint32_t> sv_serverId_value{0, 0x144DF9478};

		WEAK symbol<int> gameTime{0, 0x1443F4B6C};
		WEAK symbol<int> serverTime{0, 0x14647B280};
	}
}
