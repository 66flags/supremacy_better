#include "includes.h"

bool detours::init ( ) {
	if ( MH_Initialize ( ) != MH_OK )
		throw std::runtime_error ( XOR ( "Unable to initialize minhook!" ) );

	const auto _paint = pattern::find ( g_csgo.m_engine_dll, XOR ( "55 8B EC 83 EC 40 53 8B D9 8B 0D ? ? ? ? 89" ) ).as< void * > ( );
	//const auto _packet_start = util::get_method < decltype ( &PacketStart ) > ( g_csgo.m_cl, 5 );
	const auto _should_skip_animation_frame = pattern::find ( g_csgo.m_client_dll, XOR ( "57 8B F9 8B 07 8B ? ? ? ? ? FF D0 84 C0 75 02" ) ).as < void * > ( );
	const auto _check_for_sequence_change = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 51 53 8B 5D 08 56 8B F1 57 85" ) ).as < void * > ( ); // 55 8B EC 51 53 8B 5D 08 56 8B F1 57 85
	const auto _standard_blending_rules = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 8B 75 08 57 8B F9 85 F6" ) ).as < void * > ( ); //55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 8B 75 08 57 8B F9 85 F6
	const auto _modify_eye_position = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F8 83 EC 58 56 57 8B F9 83 7F 60" ) ).as < void * > ( );
	const auto _base_interpolate_part1 = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 51 8B 45 14 56 8B F1 C7 00 ? ? ? ? 8B 06 8B 80" ) ).as < void * > ( );
	const auto _animstate_update = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24" ) ).as < void *  > ( );
	const auto _do_procedural_footplant = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F0 83 EC 78 56 8B F1 57 8B 56 60 85" ) ).as < void * > ( );
	const auto _setup_bones = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 8B 0D" ) ).as < void * > ( );
	const auto _setup_movement = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 56 57 8B 3D ? ? ? ? 8B" ) ).as < void * > ( );
	const auto _update_client_side_animation = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 51 56 8B F1 80 BE ? ? ? ? ? 74" ) ).as < void * > ( );
	const auto _svcmsg_voicedata = pattern::find ( g_csgo.m_engine_dll, XOR ( "55 8B EC 83 E4 F8 A1 ? ? ? ? 81 EC ? ? ? ? 53 56 8B F1 B9 ? ? ? ? 57 FF 50 34 8B 7D 08 85 C0 74 13 8B 47 08 40 50 68 ? ? ? ? FF 15 ? ? ? ? 83 C4 08 8B 47 08 89 44 24 1C 8D 48 01 8B 86 ? ? ? ? 40 89 4C 24 0C 3B C8 75 49" ) ).as < void * > ( );
	const auto _process_movement = util::get_method < void * > ( g_csgo.m_game_movement, CGameMovement::PROCESSMOVEMENT );
	const auto _maintain_sequence_transitions = pattern::find ( g_csgo.m_client_dll, XOR ( "53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 18 56 57 8B F9 F3" ) ).as < void * > ( );
	const auto _computeposeparam_moveyaw = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F0 83 EC 48 56 8B F1 57 8B 46 14 85 C0 74 09 83 F8 01 0F 85 ? ? ? ? E8" ) ).as < void * > ( );
	const auto _teleported = pattern::find ( g_csgo.m_client_dll, XOR ( "E8 ? ? ? ? 84 C0 75 0D 8B 87 ? ? ? ? C1 E8 03 A8 01 74 2E 8B" ) ).rel32 ( 0x1 ).as < void * > ( );
	const auto _process_interp_list = pattern::find ( g_csgo.m_client_dll, XOR ( "0F ? ? ? ? ? ? 3D ? ? ? ? 74 3F" ) ).as < void * > ( );
	const auto _drawstaticproparrayfast = util::get_method < void * > ( g_csgo.m_model_render, IVModelRender::DRAWSTATICPROPARRAYFAST );
	const auto _cl_fireevents = pattern::find ( g_csgo.m_engine_dll, XOR ( "55 8B EC 83 EC 08 53 8B 1D ? ? ? ? 56 57 83 BB ? ? ? ? ? 74" ) ).as < void * > ( );
	const auto _packet_start = pattern::find ( g_csgo.m_engine_dll, XOR ( "56 8B F1 E8 ? ? ? ? 8B 8E ? ? ? ? 3B" ) ).sub ( 32 ).as < decltype ( &PacketStart ) > ( );
	const auto _run_simulation = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 EC 08 53 8B 5D 10 56 57 FF 75 0C 8B F1 F3 0F 11 55 ? 8D" ) ).as < void * > ( );
	const auto _do_animation_event = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 EC 08 53 56 57 8B F9 8B 8F ? ? ? ? 8B 01 FF 10 8B CF 8B D8 E8" ) ).as < void * > ( );

	// create detours.
	MH_CreateHook ( _packet_start, detours::PacketStart, ( void ** ) &old::PacketStart );
	MH_CreateHook ( _paint, detours::Paint, ( void ** ) &old::Paint );
	MH_CreateHook ( _should_skip_animation_frame, detours::ShouldSkipAnimationFrame, ( void ** ) &old::ShouldSkipAnimationFrame );
	MH_CreateHook ( _check_for_sequence_change, detours::CheckForSequenceChange, ( void ** ) &old::CheckForSequenceChange );
	MH_CreateHook ( _standard_blending_rules, detours::StandardBlendingRules, ( void ** ) &old::StandardBlendingRules );
	MH_CreateHook ( _modify_eye_position, detours::ModifyEyePosition, ( void ** ) &old::ModifyEyePosition );
	MH_CreateHook ( _base_interpolate_part1, detours::BaseInterpolatePart1, ( void ** ) &old::BaseInterpolatePart1 );
//	MH_CreateHook ( _animstate_update, detours::UpdateAnimationState, ( void ** ) &old::UpdateAnimationState );
	MH_CreateHook ( _do_procedural_footplant, detours::DoProceduralFootPlant, ( void ** ) &old::DoProceduralFootPlant );
	MH_CreateHook ( _setup_bones, detours::SetupBones, ( void ** ) &old::SetupBones );
	//MH_CreateHook ( _setup_movement, detours::SetupMovement, ( void ** ) &old::SetupMovement );
	//MH_CreateHook ( _svcmsg_voicedata, detours::SVCMsg_VoiceData, ( void ** ) &old::SVCMsg_VoiceData );
	MH_CreateHook ( _process_movement, detours::ProcessMovement, ( void ** ) &old::ProcessMovement );
	//	MH_CreateHook ( _maintain_sequence_transitions, detours::MaintainSequenceTransitions, ( void ** ) &old::MaintainSequenceTransitions );
	MH_CreateHook ( _update_client_side_animation, detours::UpdateClientSideAnimation, ( void ** ) &old::UpdateClientSideAnimation );
	MH_CreateHook ( _process_interp_list, detours::ProcessInterpolatedList, ( void ** ) &old::ProcessInterpolatedList );
	MH_CreateHook ( _computeposeparam_moveyaw, detours::ComputePoseParam_MoveYaw, ( void ** ) &old::ComputePoseParam_MoveYaw );
	MH_CreateHook ( _teleported, detours::Teleported, ( void ** ) &old::Teleported ); // we dont really need to do this since disabling bone interp clears ik targets already lol
	MH_CreateHook ( _cl_fireevents, detours::CL_FireEvents, ( void ** ) &old::CL_FireEvents );
	auto status = MH_CreateHook ( _do_animation_event, detours::DoAnimationEvent, ( void ** ) &old::DoAnimationEvent );
	g_cl.print_debug ( XOR ( "DoAnimationEvent: %s\n" ), MH_StatusToString ( status ) );
	//MH_CreateHook ( _run_simulation, detours::RunSimulation, ( void ** ) &old::RunSimulation );

	// enable all hooks.
	MH_EnableHook ( MH_ALL_HOOKS );

	return true;
}

void __fastcall detours::DoAnimationEvent ( void* ecx, void* edx, int animEvent, int data ) {
	auto player = *( Player ** ) ( std::uintptr_t ( ecx ) + 328 );

	if ( player == g_cl.m_local )
		g_cl.m_events.push_back ( animEvent );

	old::DoAnimationEvent ( ecx, edx, animEvent, data );
	g_cl.print_debug ( "event: %d\n", animEvent );
}

int __fastcall detours::PacketStart ( void *ecx, void *edx, int incoming_sequence, int outgoing_acknowledged ) {
	for ( const int it : g_cl.m_outgoing_cmd_nums ) {
		if ( it == outgoing_acknowledged ) {
			old::PacketStart ( ecx, edx, incoming_sequence, outgoing_acknowledged );
			break;
		}
	}

	for ( auto it = g_cl.m_outgoing_cmd_nums.begin ( ); it != g_cl.m_outgoing_cmd_nums.end ( ); ) {
		if ( *it < outgoing_acknowledged )
			it = g_cl.m_outgoing_cmd_nums.erase ( it );
		else
			it++;
	}

	return 0;
}

void __vectorcall detours::UpdateAnimationState ( void *ecx, void *a1, float a2, float a3, float a4, void *a5 ) {
	const auto state = ( CCSGOPlayerAnimState * ) ecx;

	return old::UpdateAnimationState ( ecx, a1, a2, a3, a4, a5 );
}

void detours::CL_FireEvents ( ) {
	//  55 8B EC 83 EC 08 53 8B 1D ? ? ? ? 56 57 83 BB ? ? ? ? ? 74
	CEventInfo *ei = g_csgo.m_cl->m_events;
	CEventInfo *next = nullptr;

	if ( !ei ) {
		return old::CL_FireEvents ( );
	}

	do {
		next = *reinterpret_cast< CEventInfo ** >( reinterpret_cast< uintptr_t >( ei ) + 0x38 );

		uint16_t classID = ei->m_class_id - 1;

		auto m_pCreateEventFn = ei->m_client_class->m_pCreateEvent;
		if ( !m_pCreateEventFn ) {
			continue;
		}

		void *pCE = m_pCreateEventFn ( );
		if ( !pCE ) {
			continue;
		}

		if ( classID == 170 ) {
			ei->m_fire_delay = 0.0f;
		}
		ei = next;
	} while ( next != nullptr );

	old::CL_FireEvents ( );
}

void detours::ProcessInterpolatedList ( ) {
	// extrapolate - 80 3d ? ? ? ? ? 8d 57 + 0x2
	static auto s_bAllowExtrapolation = pattern::find ( g_csgo.m_client_dll, XOR ( "80 3D ? ? ? ? ? 8D 57" ) ).add ( 0x2 ).to ( ).as < bool * > ( );
	*s_bAllowExtrapolation = false;
	old::ProcessInterpolatedList ( );
}

int __fastcall detours::DrawStaticPropArrayFast ( void *ecx, void *edx, void *props, int count, bool shadow_deph ) {
	if ( g_menu.main.visuals.transparent_props.get ( ) )
		g_csgo.m_studio_render->SetAlphaModulation ( g_menu.main.visuals.transparent_props_opacity.get ( ) / 100.f );

	return old::DrawStaticPropArrayFast ( ecx, edx, props, count, shadow_deph );
}

bool __fastcall detours::Teleported ( void *ecx, void *edx ) {
	if ( g_bones.m_running )
		return true; // force uninterpolated bones and ik targets to be cleared.
	return old::Teleported ( ecx, edx );
}

void __fastcall detours::ComputePoseParam_MoveYaw ( void *ecx, void *edx, void *hdr ) {
	//SetLocalAngles ( QAngle ( 0, m_flCurrentFeetYaw, 0 ) );
	old::ComputePoseParam_MoveYaw ( ecx, edx, hdr );
}

void __fastcall detours::MaintainSequenceTransitions ( void *ecx, void *edx, void *bone_setup, float cycle, vec3_t pos [ ], quaternion_t q [ ] ) {
	if ( !g_bones.m_running )
		return old::MaintainSequenceTransitions ( ecx, edx, bone_setup, cycle, pos, q );
}

int __fastcall detours::BaseInterpolatePart1 ( void *ecx, void *edx, float &curtime, vec3_t &old_origin, ang_t &old_angs, int &no_more_changes ) {
	const auto player = reinterpret_cast< Player * >( ecx );

	if ( !player )
		return old::BaseInterpolatePart1 ( ecx, edx, curtime, old_origin, old_angs, no_more_changes );

	static auto CBaseEntity__MoveToLastReceivedPos = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 80 7D 08 00 56 8B F1 75 0D 80 BE ? ? ? ? ? 0F" ) ).as < void ( __thiscall * ) ( void *, bool ) > ( );

	if ( player->IsPlayer ( ) && player != g_cl.m_local ) {
		no_more_changes = 1;
		CBaseEntity__MoveToLastReceivedPos ( player, false );
		return 0;
	}

	return old::BaseInterpolatePart1 ( ecx, edx, curtime, old_origin, old_angs, no_more_changes );
}

void __fastcall detours::ProcessMovement ( void *ecx, void *edx, Entity *player, CMoveData *data ) {
	data->m_bGameCodeMovedPlayer = false;
	old::ProcessMovement ( ecx, edx, player, data );
}

bool __fastcall detours::SVCMsg_VoiceData ( void *ecx, void *edx, void *a2 ) {
	auto og = old::SVCMsg_VoiceData ( ecx, edx, a2 );

	return og;
}

void __fastcall detours::UpdateClientSideAnimation ( void *ecx, void *edx ) {
	auto player = ( Player * ) ecx;

	if ( !player )
		return old::UpdateClientSideAnimation ( ecx, edx );

	if ( player->index ( ) == g_cl.m_local->index ( ) || g_cl.m_local == player ) {
		auto state = ( CCSGOGamePlayerAnimState * ) g_cl.m_local->m_PlayerAnimState ( );

		if ( g_cl.m_animate )
			old::UpdateClientSideAnimation ( ecx, edx );
		else {
			// fix viewmodel addons not updating.
			g_cl.m_local->m_PlayerAnimState ( )->m_player = nullptr;
			old::UpdateClientSideAnimation ( g_cl.m_local, 0 );
			g_cl.m_local->m_PlayerAnimState ( )->m_player = g_cl.m_local;

			// update animation data for the local player.
			//g_cl.OnRenderStart ( );

			// this will update the attachments origin.
			static auto SetupBones_AttachmentHelper = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4 56 57 85 DB 0F 84" ) ).as < void ( __thiscall * ) ( void *, void * ) > ( );
			SetupBones_AttachmentHelper ( g_cl.m_local, g_cl.m_local->GetModelPtr ( ) );

			// force setupbones rebuild.
			g_bones.Build ( player, nullptr, g_csgo.m_globals->m_curtime );
			//rebuilt::InvalidatePhysicsRecursive ( player, 8 );
		}
	}
	else
		old::UpdateClientSideAnimation ( ecx, edx );

	if ( player->m_iTeamNum ( ) != g_cl.m_local->m_iTeamNum ( ) ) {
		player->SetAbsOrigin ( player->m_vecOrigin ( ) );

		if ( g_cl.m_update_ent ) {
			// @onetap
			if ( player->m_AnimOverlay ( ) ) {
				for ( int i = 0; i < ANIMATION_LAYER_COUNT; i++ ) {
					player->m_AnimOverlay ( ) [ i ].m_owner = player;
				}
			}

			int iEFlags = player->m_iEFlags ( );

			player->m_iEFlags ( ) &= ~( 1 << 12 );

			old::UpdateClientSideAnimation ( ecx, edx );

			player->m_iEFlags ( ) = iEFlags;
		}
		else
			old::UpdateClientSideAnimation ( ecx, edx );
	}

	else
		old::UpdateClientSideAnimation ( ecx, edx );
}

void __fastcall detours::SetupMovement ( void *ecx, void *edx ) {
	auto state = ( CCSGOGamePlayerAnimState * ) ecx;
	old::SetupMovement ( ecx, edx );
}

void __fastcall detours::DoProceduralFootPlant ( void *ecx, void *edx, int a1, int a2, int a3, int a4 ) {
	return;
}

bool __fastcall detours::SetupBones ( void *ecx, void *edx, matrix3x4_t *out, int max, int mask, float curtime ) {
	auto player = ( Player * ) ecx - 4;

	static auto &g_iModelBoneCounter = *pattern::find ( g_csgo.m_client_dll, XOR ( "3B 05 ? ? ? ? 0F 84 ? ? ? ? 8B 47" ) ).add ( 2 ).to ( ).as< int * > ( );

	if ( player && player != g_cl.m_local && player->alive ( ) && player->IsPlayer ( ) ) {
		AimPlayer *data = &g_aimbot.m_players [ player->index ( ) - 1 ];

		if ( data && data->m_records.size ( ) ) {
			auto cache = &player->m_BoneCache ( );

			if ( cache ) {
				LagRecord *current = data->m_records.front ( ).get ( );

				if ( current && current->m_setup ) {
					if ( *( int * ) ( std::uintptr_t ( player ) + 0x267C ) != g_iModelBoneCounter ) {
						auto cache = &player->m_BoneCache ( );

						memcpy ( cache->m_pCachedBones, current->m_bones, sizeof ( BoneArray ) * cache->m_CachedBoneCount );

						for ( auto i = 0; i < cache->m_CachedBoneCount; i++ )
							cache->m_pCachedBones [ i ].SetOrigin ( cache->m_pCachedBones [ i ].GetOrigin ( ) - current->m_origin + player->GetRenderOrigin ( ) );

						static auto SetupBones_AttachmentHelper = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4 56 57 85 DB 0F 84" ) ).as < void ( __thiscall * ) ( void *, void * ) > ( );
						SetupBones_AttachmentHelper ( player, player->GetModelPtr ( ) );
						*( int * ) ( std::uintptr_t ( player ) + 0x267C ) = g_iModelBoneCounter;
					}

					if ( out ) {
						if ( max >= cache->m_CachedBoneCount )
							memcpy ( out, cache->m_pCachedBones, sizeof ( BoneArray ) * cache->m_CachedBoneCount );
						else
							return false;
					}

					return true;
				}
			}	
		}
	}

	return old::SetupBones ( ecx, edx, out, max, mask, curtime );
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

void __fastcall detours::ModifyEyePosition ( void *ecx, void *edx, vec3_t &eye_pos ) {
	auto state = reinterpret_cast < CCSGOPlayerAnimState * > ( ecx );

	if ( !state )
		return old::ModifyEyePosition ( ecx, edx, eye_pos );

	*( bool * ) ( std::uintptr_t ( state ) + 0x328 ) = false;

	return old::ModifyEyePosition ( ecx, edx, eye_pos );
}

bool __fastcall detours::ShouldSkipAnimationFrame ( void *ecx, void *edx ) {
	return false;
}

void __fastcall detours::CheckForSequenceChange ( void *ecx, void *edx, void *hdr, int sequence, bool force_new_sequence, bool interpolate ) {
	old::CheckForSequenceChange ( ecx, edx, hdr, sequence, force_new_sequence, false );
}

void __fastcall detours::StandardBlendingRules ( void *ecx, void *edx, void *hdr, void *pos, void *q, float current_time, int bone_mask ) {
	const auto player = reinterpret_cast< Player * >( ecx );

	if ( !player || player != g_cl.m_local )
		return old::StandardBlendingRules ( ecx, edx, hdr, pos, q, current_time, bone_mask );

	player->m_fEffects ( ) |= EF_NOINTERP;

	old::StandardBlendingRules ( ecx, edx, hdr, pos, q, current_time, bone_mask );

	player->m_fEffects ( ) &= ~EF_NOINTERP;
}