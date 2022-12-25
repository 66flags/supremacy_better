#include "includes.h"

bool detours::init ( ) {
	if ( MH_Initialize ( ) != MH_OK )
		throw std::runtime_error ( XOR ( "Unable to initialize minhook!" ) );

	const auto _paint = pattern::find ( g_csgo.m_engine_dll, XOR ( "55 8B EC 83 EC 40 53 8B D9 8B 0D ? ? ? ? 89" ) ).as< void * > ( );
	const auto _packet_end = util::get_method < void * > ( g_csgo.m_cl, CClientState::PACKETEND );
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
	
	// create detours.
	MH_CreateHook ( _paint, detours::Paint, ( void ** ) &old::Paint );
	MH_CreateHook ( _packet_end, detours::PacketEnd, ( void ** ) &old::PacketEnd );
	MH_CreateHook ( _should_skip_animation_frame, detours::ShouldSkipAnimationFrame, ( void ** ) &old::ShouldSkipAnimationFrame );
	MH_CreateHook ( _check_for_sequence_change, detours::CheckForSequenceChange, ( void ** ) &old::CheckForSequenceChange );
	MH_CreateHook ( _standard_blending_rules, detours::StandardBlendingRules, ( void ** ) &old::StandardBlendingRules );
	MH_CreateHook ( _modify_eye_position, detours::ModifyEyePosition, ( void ** ) &old::ModifyEyePosition );
	MH_CreateHook ( _base_interpolate_part1, detours::BaseInterpolatePart1, ( void ** ) &old::BaseInterpolatePart1 );
	MH_CreateHook ( _animstate_update, detours::UpdateAnimationState, ( void ** ) &old::UpdateAnimationState );
	MH_CreateHook ( _do_procedural_footplant, detours::DoProceduralFootPlant, ( void ** ) &old::DoProceduralFootPlant );
	//MH_CreateHook ( _setup_bones, detours::SetupBones, ( void ** ) &old::SetupBones );
	MH_CreateHook ( _setup_movement, detours::SetupMovement, ( void ** ) &old::SetupMovement );
	//MH_CreateHook ( _svcmsg_voicedata, detours::SVCMsg_VoiceData, ( void ** ) &old::SVCMsg_VoiceData );
	MH_CreateHook ( _process_movement, detours::ProcessMovement, ( void ** ) &old::ProcessMovement );

	// enable all hooks.
	MH_EnableHook ( MH_ALL_HOOKS );

	return true;
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

void __fastcall detours::ProcessMovement ( void* ecx, void* edx, Entity *player, CMoveData *data ) {
	data->m_bGameCodeMovedPlayer = false;
	old::ProcessMovement ( ecx, edx, player, data );
}

bool __fastcall detours::SVCMsg_VoiceData ( void *ecx, void *edx, void *a2 ) {
	auto og = old::SVCMsg_VoiceData ( ecx, edx, a2 );

	return og;
}

CUtlVector <uint16_t> __forceinline rebuild_modifiers ( CCSGOGamePlayerAnimState *state ) {
	activity_modifiers_wrapper modifier_wrapper {};

	modifier_wrapper.add_modifier ( state->GetWeaponPrefix ( ) );

	if ( state->m_flSpeedAsPortionOfWalkTopSpeed > .25f )
		modifier_wrapper.add_modifier ( "moving" );

	if ( state->m_flAnimDuckAmount > .55f )
		modifier_wrapper.add_modifier ( "crouch" );

	return modifier_wrapper.get ( );
}

void __fastcall detours::SetupMovement ( void *ecx, void *edx ) {
	auto state = ( CCSGOGamePlayerAnimState * ) ecx;

	if ( !state || !state->m_pPlayer || state->m_pPlayer != g_cl.m_local )
		return old::SetupMovement ( ecx, edx );

	bool &m_bJumping = state->m_bFlashed;

	if ( !( state->m_pPlayer->m_fFlags ( ) & FL_ONGROUND )
		&& state->m_bOnGround
		&& state->m_pPlayer->m_vecVelocity ( ).z > 0.0f ) {
		m_bJumping = true;
		rebuilt::SetSequence ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, rebuilt::SelectWeightedSequence ( state, ACT_CSGO_JUMP ) );
	}

	g_cl.m_activity_modifiers.add_modifier ( state->GetWeaponPrefix ( ) );

	// update modifiers.
	if ( state->m_flSpeedAsPortionOfWalkTopSpeed > 0.25f )
		g_cl.m_activity_modifiers.add_modifier ( "moving" );

	if ( state->m_flAnimDuckAmount > 0.55f )
		g_cl.m_activity_modifiers.add_modifier ( "crouch" );

	if ( !( g_inputpred.data.m_nOldButtons & FL_ONGROUND ) ) {
		rebuilt::SetSequence ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, rebuilt::SelectWeightedSequence ( state, ACT_CSGO_FALL ) );
		m_bJumping = true;
	}
	else
		rebuild_modifiers ( state );

	old::SetupMovement ( ecx, edx );

	bool bPreviouslyOnLadder = state->m_bOnLadder;
	state->m_bOnLadder = !state->m_bOnGround && state->m_pPlayer->m_MoveType ( ) == MOVETYPE_LADDER;
	bool bStartedLadderingThisFrame = ( !bPreviouslyOnLadder && state->m_bOnLadder );
	bool bStoppedLadderingThisFrame = ( bPreviouslyOnLadder && !state->m_bOnLadder );

	if ( state->m_bOnGround ) {
		bool next_landing = false;

		if ( !state->m_bLanding && ( state->m_bLandedOnGroundThisFrame || bStoppedLadderingThisFrame ) ) {
			rebuilt::SetSequence ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, rebuilt::SelectWeightedSequence ( state, ( state->m_flDurationInAir > 1 ) ? ACT_CSGO_LAND_HEAVY : ACT_CSGO_LAND_LIGHT ) );
			//rebuilt::SetCycle ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
			next_landing = true;
		}

		state->m_flDurationInAir = 0;

		if ( next_landing && rebuilt::GetLayerActivity ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) != ACT_CSGO_CLIMB_LADDER ) {
			m_bJumping = false;

			rebuilt::IncrementLayerCycle ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, false );
			rebuilt::IncrementLayerCycle ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false );

			state->m_pPlayer->m_flPoseParameter ( ) [ POSE_JUMP_FALL ] = 0.f;

			if ( rebuilt::IsLayerSequenceCompleted ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) ) {
				state->m_bLanding = false;
				rebuilt::SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
				rebuilt::SetWeight ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, 0 );
				state->m_flLandAnimMultiplier = 1.0f;
			}
			else {
				float flLandWeight = rebuilt::GetLayerIdealWeightFromSequenceCycle ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) * state->m_flLandAnimMultiplier;
				flLandWeight *= std::clamp< float > ( ( 1.0f - state->m_flAnimDuckAmount ), 0.2f, 1.0f );

				rebuilt::SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLandWeight );

				float flCurrentJumpFallWeight = state->m_pPlayer->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_weight;
				if ( flCurrentJumpFallWeight > 0 ) {
					flCurrentJumpFallWeight = valve_math::Approach ( 0, flCurrentJumpFallWeight, state->m_flLastUpdateIncrement * 10.0f );
					rebuilt::SetWeight ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flCurrentJumpFallWeight );
				}
			}
		}

		if ( !state->m_bLanding && !m_bJumping && state->m_flLadderWeight <= 0 ) {
			rebuilt::SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
		}
	}
	else if ( !state->m_bOnLadder ) {
		state->m_bLanding = false;

		// we're in the air
		if ( state->m_bLeftTheGroundThisFrame || bStoppedLadderingThisFrame ) {
			// If entered the air by jumping, then we already set the jump activity.
			// But if we're in the air because we strolled off a ledge or the floor collapsed or something,
			// we need to set the fall activity here.
			if ( !m_bJumping ) {
				rebuilt::SetSequence ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, rebuilt::SelectWeightedSequence ( state, ACT_CSGO_FALL ) );
			}

			state->m_flDurationInAir = 0;
		}

		state->m_flDurationInAir += state->m_flLastUpdateIncrement;

		rebuilt::IncrementLayerCycle ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false );

		// increase jump weight
		float flJumpWeight = state->m_pPlayer->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_weight;
		float flNextJumpWeight = rebuilt::GetLayerIdealWeightFromSequenceCycle ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL );

		if ( flNextJumpWeight > flJumpWeight )
			rebuilt::SetWeight ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flNextJumpWeight );

		auto smoothstep_bounds = [ ] ( float edge0, float edge1, float x ) {
			x = std::clamp< float > ( ( x - edge0 ) / ( edge1 - edge0 ), 0, 1 );
			return x * x * ( 3 - 2 * x );
		};

		float flLingeringLandWeight = state->m_pPlayer->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ].m_weight;

		if ( flLingeringLandWeight > 0 ) {
			flLingeringLandWeight *= smoothstep_bounds ( 0.2f, 0.0f, state->m_flDurationInAir );
			rebuilt::SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLingeringLandWeight );
		}

		// blend jump into fall. This is a no-op if we're playing a fall anim.
		state->m_pPlayer->m_flPoseParameter ( ) [ POSE_JUMP_FALL ] = std::clamp ( smoothstep_bounds ( 0.72f, 1.52f, state->m_flDurationInAir ), 0.0f, 1.0f );
	}
}

void __vectorcall detours::UpdateAnimationState ( void *ecx, void *a1, float a2, float a3, float a4, void *a5 ) {
	const auto state = ( CCSGOPlayerAnimState * ) ecx;

	if ( state->m_player != g_cl.m_local )
		return old::UpdateAnimationState ( ecx, a1, a2, a3, a4, a5 );

	auto game_state = ( CCSGOGamePlayerAnimState * ) ecx;

	if ( state->m_frame == g_csgo.m_globals->m_frame )
		state->m_frame -= 1;

	//auto angle = g_cl.m_angle;

	old::UpdateAnimationState ( ecx, a1, a2, a3, a4, a5 );

	//if ( game_state->m_flTimeOfLastKnownInjury < game_state->m_pPlayer->m_flTimeOfLastInjury ( ) ) {
	//	game_state->m_flTimeOfLastKnownInjury = game_state->m_pPlayer->m_flTimeOfLastInjury ( );

	//	// flinches override flinches of their own priority
	//	bool bNoFlinchIsPlaying = ( rebuilt::IsLayerSequenceCompleted ( game_state, ANIMATION_LAYER_FLINCH ) || game_state->m_pPlayer->m_AnimOverlay ( ) [ ANIMATION_LAYER_FLINCH ].m_weight <= 0 );
	//	bool bHeadshotIsPlaying = ( !bNoFlinchIsPlaying && rebuilt::GetLayerActivity ( game_state, ANIMATION_LAYER_FLINCH ) == ACT_CSGO_FLINCH_HEAD );

	//	//if ( game_state->m_pPlayer->GetLastDamageTypeFlags ( ) & DMG_BURN ) {
	//	//	if ( bNoFlinchIsPlaying ) {
	//	//		rebuilt::SetSequence ( game_state, ANIMATION_LAYER_FLINCH, rebuilt::SelectWeightedSequence ( game_state, ACT_CSGO_FLINCH_MOLOTOV ) );

	//	//		// clear out all the flinch-related actmods now we selected a sequence
	//	//		rebuild_modifiers ( game_state );
	//	//	}
	//	//}
	//	if ( bNoFlinchIsPlaying || !bHeadshotIsPlaying || game_state->m_pPlayer->m_LastHitGroup ( ) == HITGROUP_HEAD ) {
	//		auto damageDir = game_state->m_pPlayer->m_nRelativeDirectionOfLastInjury ( );
	//		bool bLeft = false;
	//		bool bRight = false;
	//		switch ( damageDir ) {
	//		case DAMAGED_DIR_NONE:
	//		case DAMAGED_DIR_FRONT:
	//		{
	//			g_cl.m_activity_modifiers.add_modifier ( "front" );
	//			break;
	//		}
	//		case DAMAGED_DIR_BACK:
	//		{
	//			g_cl.m_activity_modifiers.add_modifier ( "back" );
	//			break;
	//		}
	//		case DAMAGED_DIR_LEFT:
	//		{
	//			g_cl.m_activity_modifiers.add_modifier ( "left" );
	//			bLeft = true;
	//			break;
	//		}
	//		case DAMAGED_DIR_RIGHT:
	//		{
	//			g_cl.m_activity_modifiers.add_modifier ( "right" );
	//			bRight = true;
	//			break;
	//		}
	//		}
	//		switch ( game_state->m_pPlayer->m_LastHitGroup ( ) ) {
	//		case HITGROUP_HEAD:
	//		{
	//			g_cl.m_activity_modifiers.add_modifier ( "head" );
	//			break;
	//		}
	//		case HITGROUP_CHEST:
	//		{
	//			g_cl.m_activity_modifiers.add_modifier ( "chest" );
	//			break;
	//		}
	//		case HITGROUP_LEFTARM:
	//		{
	//			if ( !bLeft ) { g_cl.m_activity_modifiers.add_modifier ( "left" ); }
	//			g_cl.m_activity_modifiers.add_modifier ( "arm" );
	//			break;
	//		}
	//		case HITGROUP_RIGHTARM:
	//		{
	//			if ( !bRight ) { g_cl.m_activity_modifiers.add_modifier ( "right" ); }
	//			g_cl.m_activity_modifiers.add_modifier ( "arm" );
	//			break;
	//		}
	//		case HITGROUP_GENERIC:
	//		case HITGROUP_STOMACH:
	//		{
	//			g_cl.m_activity_modifiers.add_modifier ( "gut" );
	//			break;
	//		}
	//		case HITGROUP_LEFTLEG:
	//		{
	//			if ( !bLeft ) { g_cl.m_activity_modifiers.add_modifier ( "left" ); }
	//			g_cl.m_activity_modifiers.add_modifier ( "leg" );
	//			break;
	//		}
	//		case HITGROUP_RIGHTLEG:
	//		{
	//			if ( !bRight ) { g_cl.m_activity_modifiers.add_modifier ( "right" ); }
	//			g_cl.m_activity_modifiers.add_modifier ( "leg" );
	//			break;
	//		}
	//		}
	//		rebuilt::SetSequence ( game_state, ANIMATION_LAYER_FLINCH, rebuilt::SelectWeightedSequence ( game_state, ( game_state->m_pPlayer->m_LastHitGroup ( ) == HITGROUP_HEAD ) ? ACT_CSGO_FLINCH_HEAD : ACT_CSGO_FLINCH ) );

	//		// clear out all the flinch-related actmods now we selected a sequence
	//		rebuild_modifiers ( game_state );
	//	}

	//}

}

void __fastcall detours::DoProceduralFootPlant ( void *ecx, void *edx, int a1, int a2, int a3, int a4 ) {
	return;
}

bool __fastcall detours::SetupBones ( void *ecx, void *edx, BoneArray *out, int max, int mask, float curtime ) {
	static auto SetupBones_AttachmentHelper = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4 56 57 85 DB 0F 84" ) ).as < void ( __thiscall * ) ( void *, void * ) > ( );

	auto player = ( Player * ) ecx - 4;

	if ( g_bones.m_running )
		return old::SetupBones ( ecx, edx, out, max, mask, curtime );

	if ( player && player->alive ( ) && player == g_cl.m_local && player->IsPlayer ( ) ) {
		//SetupBones_AttachmentHelper ( player, *( void ** ) ( std::uintptr_t ( player ) + 0x2938 ) ); // fix attachments on local

		if ( out ) {
			if ( max >= player->m_iBoneCount ( ) )
				memcpy ( out, player->m_pBoneCache ( ), sizeof ( BoneArray ) * player->m_iBoneCount ( ) );
			else
				return false;
		}
		
		return true;
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

void __fastcall detours::PacketEnd ( void *ecx, void *edx ) {
	old::PacketEnd ( ecx, edx );

	// [COLLAPSED LOCAL DECLARATIONS. PRESS KEYPAD CTRL-"+" TO EXPAND]

	auto nc = g_csgo.m_engine->GetNetChannelInfo ( );

	if ( nc && nc->m_choked_packets > 0 ) {
		const auto backup_packets = nc->m_choked_packets;

		auto out_sequence_nr = *( int * ) ( std::uintptr_t ( nc ) + 0x18 );

		nc->m_choked_packets = 0;
		nc->SendDatagram ( 0 );
		nc->m_choked_packets = backup_packets;

		-- *( int * ) ( std::uintptr_t ( nc ) + 0x2C );
		-- *( int * ) ( std::uintptr_t ( nc ) + 0x18 );
	}
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