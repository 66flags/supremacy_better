#include "includes.h"

bool detours::init ( ) {
	if ( MH_Initialize ( ) != MH_OK )
		throw std::runtime_error ( XOR ( "Unable to initialize minhook!" ) );

	const auto _paint = pattern::find ( g_csgo.m_engine_dll, XOR ( "55 8B EC 83 EC 40 53 8B D9 8B 0D ? ? ? ? 89" ) ).as< void * > ( );
	const auto _packet_end = util::get_method < void * > ( g_csgo.m_cl, CClientState::PACKETEND );
	const auto _should_skip_animation_frame = pattern::find ( g_csgo.m_client_dll, XOR ( "57 8B F9 8B 07 8B ? ? ? ? ? FF D0 84 C0 75 02" ) ).as < void * > ( );
	const auto _check_for_sequence_change = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 51 53 8B 5D 08 56 8B F1 57 85" ) ).as < void * > ( ); // 55 8B EC 51 53 8B 5D 08 56 8B F1 57 85
	const auto _standard_blending_rules = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 8B 75 08 57 8B F9 85 F6" ) ).as < void * > ( ); //55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 8B 75 08 57 8B F9 85 F6

	// create detours.
	MH_CreateHook ( _paint, detours::Paint, ( void ** ) &old::Paint );
	MH_CreateHook ( _packet_end, detours::PacketEnd, ( void ** ) &old::PacketEnd );
	MH_CreateHook ( _should_skip_animation_frame, detours::ShouldSkipAnimationFrame, ( void ** ) &old::ShouldSkipAnimationFrame );
	MH_CreateHook ( _check_for_sequence_change, detours::CheckForSequenceChange, ( void ** ) &old::CheckForSequenceChange );
	MH_CreateHook ( _standard_blending_rules, detours::StandardBlendingRules, ( void ** ) &old::StandardBlendingRules );

	// enable all hooks.
	MH_EnableHook ( MH_ALL_HOOKS );

	return true;
}

void __fastcall detours::Paint ( void *ecx, void *edx, PaintModes_t mode ) {
	old::Paint ( ecx, edx, mode );

	const auto vgui = reinterpret_cast< IEngineVGui * >( ecx );

	static auto StartDrawing = pattern::find ( g_csgo.m_surface_dll, XOR ( "55 8B EC 83 E4 C0 83 EC 38 80 3D ? ? ? ? ? 56 57 8B F9 75 53" ) ).as< void ( __thiscall * )( void * ) > ( );
	static auto FinishDrawing = pattern::find ( g_csgo.m_surface_dll, XOR ( "8B 0D ? ? ? ? 56 C6 05 ? ? ? ? ? 8B 01 FF 90" ) ).as<  void ( __thiscall * )( void * ) > ( );

	if ( vgui->m_static_transition_panel && ( mode & PAINT_UIPANELS ) ) {
		StartDrawing ( g_csgo.m_surface );
		g_cl.OnPaint ( );
		FinishDrawing ( g_csgo.m_surface );
	}
}

void __fastcall detours::PacketEnd ( void *ecx, void *edx ) {
	old::PacketEnd ( ecx, edx );

	// [COLLAPSED LOCAL DECLARATIONS. PRESS KEYPAD CTRL-"+" TO EXPAND]

	auto nc = g_csgo.m_engine->GetNetChannelInfo ( );

	if ( nc ) {
		auto out_sequence_nr = *( int * ) ( std::uintptr_t ( nc ) + 0x18 );
		nc->m_choked_packets = 0;
		nc->SendDatagram ( 0 );

		-- *( int * ) ( std::uintptr_t ( nc ) + 0x2C );
		-- *( int * ) ( std::uintptr_t ( nc ) + 0x18 );
	}
}

bool __fastcall detours::ShouldSkipAnimationFrame ( void *ecx, void *edx, float time ) {
	return false;
}

void __fastcall detours::CheckForSequenceChange ( void *ecx, void *edx, void *hdr, int sequence, bool force_new_sequence, bool interpolate ) {
	return old::CheckForSequenceChange ( ecx, edx, hdr, sequence, force_new_sequence, false );
}

void __fastcall detours::StandardBlendingRules ( void *ecx, void *edx, void *hdr, void *pos, void *q, float current_time, int bone_mask ) {
	const auto player = reinterpret_cast< Player * >( ecx );

	if ( !( player->m_fEffects ( ) & EF_NOINTERP ) )
		player->m_fEffects ( ) |= EF_NOINTERP;

	old::StandardBlendingRules ( ecx, edx, hdr, pos, q, current_time, bone_mask );

	player->m_fEffects ( ) &= ~EF_NOINTERP;
}