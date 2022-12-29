#include "includes.h"

enum animtag_indices {
	ANIMTAG_INVALID = -1,
	ANIMTAG_UNINITIALIZED = 0,
	ANIMTAG_STARTCYCLE_N,
	ANIMTAG_STARTCYCLE_NE,
	ANIMTAG_STARTCYCLE_E,
	ANIMTAG_STARTCYCLE_SE,
	ANIMTAG_STARTCYCLE_S,
	ANIMTAG_STARTCYCLE_SW,
	ANIMTAG_STARTCYCLE_W,
	ANIMTAG_STARTCYCLE_NW,
	ANIMTAG_AIMLIMIT_YAWMIN_IDLE,
	ANIMTAG_AIMLIMIT_YAWMAX_IDLE,
	ANIMTAG_AIMLIMIT_YAWMIN_WALK,
	ANIMTAG_AIMLIMIT_YAWMAX_WALK,
	ANIMTAG_AIMLIMIT_YAWMIN_RUN,
	ANIMTAG_AIMLIMIT_YAWMAX_RUN,
	ANIMTAG_AIMLIMIT_YAWMIN_CROUCHIDLE,
	ANIMTAG_AIMLIMIT_YAWMAX_CROUCHIDLE,
	ANIMTAG_AIMLIMIT_YAWMIN_CROUCHWALK,
	ANIMTAG_AIMLIMIT_YAWMAX_CROUCHWALK,
	ANIMTAG_AIMLIMIT_PITCHMIN_IDLE,
	ANIMTAG_AIMLIMIT_PITCHMAX_IDLE,
	ANIMTAG_AIMLIMIT_PITCHMIN_WALKRUN,
	ANIMTAG_AIMLIMIT_PITCHMAX_WALKRUN,
	ANIMTAG_AIMLIMIT_PITCHMIN_CROUCH,
	ANIMTAG_AIMLIMIT_PITCHMAX_CROUCH,
	ANIMTAG_AIMLIMIT_PITCHMIN_CROUCHWALK,
	ANIMTAG_AIMLIMIT_PITCHMAX_CROUCHWALK,
	ANIMTAG_WEAPON_POSTLAYER,
	ANIMTAG_FLASHBANG_PASSABLE,
	ANIMTAG_COUNT
};

void rebuilt::SetSequence ( CCSGOGamePlayerAnimState *state, int layer_idx, int sequence ) {
	const auto player = state->m_pPlayer;

	if ( !player || !state )
		return;

	static auto CCSGOPlayerAnimState__UpdateLayerOrderPreset = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 51 53 56 57 8B F9 83 7F 60 00 0F 84 ? ? ? ? 83" ) ).as< void ( __thiscall * )( CCSGOGamePlayerAnimState *, int, int ) > ( );

	if ( sequence > 1 ) {
		g_csgo.m_model_cache->BeginLock ( );

		const auto layer = player->m_AnimOverlay ( ) + layer_idx;

		if ( !layer )
			return;

		if ( layer->m_owner && layer->m_sequence != sequence )
			InvalidatePhysicsRecursive ( state->m_pPlayer, 16 );

		layer->m_sequence = sequence;
		layer->m_playback_rate = player->GetSequenceCycleRate ( sequence );

		SetCycle ( state, layer_idx, 0 );
		SetWeight ( state, layer_idx, 0 );

		CCSGOPlayerAnimState__UpdateLayerOrderPreset ( state, layer_idx, sequence );

		g_csgo.m_model_cache->EndLock ( );
	}
}

int rebuilt::SelectWeightedSequence ( CCSGOGamePlayerAnimState *state, int act ) {
	int seq = -1;

	const bool crouching = state->m_flAnimDuckAmount > 0.55f;
	const bool moving = state->m_flSpeedAsPortionOfWalkTopSpeed > 0.25f;

	switch ( act ) {
	case ACT_CSGO_LAND_HEAVY:
		seq = 23;
		if ( crouching )
			seq = 24;
		break;
	case ACT_CSGO_FALL:
		seq = 14;
		break;
	case ACT_CSGO_JUMP:
		seq = moving + 17;
		if ( !crouching )
			seq = moving + 15;
		break;
	case ACT_CSGO_CLIMB_LADDER:
		seq = 13;
		break;
	case ACT_CSGO_LAND_LIGHT:
		seq = 2 * moving + 20;
		if ( crouching ) {
			seq = 21;
			if ( moving )
				seq = 19;
		}
		break;
	case ACT_CSGO_IDLE_TURN_BALANCEADJUST:
		seq = 4;
		break;
	case ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING:
		seq = 5;
		break;
	case ACT_CSGO_FLASHBANG_REACTION:
		seq = 224;
		if ( crouching )
			seq = 225;
		break;
	case ACT_CSGO_ALIVE_LOOP:
		seq = 8;
		if ( state->m_pWeapon
			&& state->m_pWeapon->GetWpnData ( )
			&& state->m_pWeapon->GetWpnData ( )->m_weapon_type == WEAPONTYPE_KNIFE )
			seq = 9;
		break;
	default:
		return -1;
	}

	if ( seq < 2 )
		return -1;

	return seq;
}

void rebuilt::SetCycle ( CCSGOGamePlayerAnimState *state, int layer_idx, float cycle ) {
	auto layer = state->m_pPlayer->m_AnimOverlay ( ) + layer_idx;

	auto clamp_cycle = [ ] ( float in ) {
		in -= int ( in );

		if ( in < 0.0f )
			in += 1.0f;
		else if ( in > 1.0f )
			in -= 1.0f;

		return in;
	};

	const auto clamped_cycle = clamp_cycle ( cycle );

	if ( layer->m_owner && layer->m_cycle != clamped_cycle ) {
		InvalidatePhysicsRecursive ( state->m_pPlayer, 8 );
	}

	layer->m_cycle = clamped_cycle;
}

const float CS_PLAYER_SPEED_RUN = 260.0f;
const float CS_PLAYER_SPEED_VIP = 227.0f;
const float CS_PLAYER_SPEED_SHIELD = 160.0f;
const float CS_PLAYER_SPEED_HAS_HOSTAGE = 200.0f;
const float CS_PLAYER_SPEED_STOPPED = 1.0f;
const float CS_PLAYER_SPEED_OBSERVER = 900.0f;
const float CS_PLAYER_SPEED_DUCK_MODIFIER = 0.34f;
const float CS_PLAYER_SPEED_WALK_MODIFIER = 0.52f;
const float CS_PLAYER_SPEED_CLIMB_MODIFIER = 0.34f;
const float CS_PLAYER_HEAVYARMOR_FLINCH_MODIFIER = 0.5f;
const float CS_PLAYER_DUCK_SPEED_IDEAL = 8.0f;
//enum AnimationLayer_t {
//	ANIMATION_LAYER_AIMMATRIX = 0,
//	ANIMATION_LAYER_WEAPON_ACTION,
//	ANIMATION_LAYER_WEAPON_ACTION_RECROUCH,
//	ANIMATION_LAYER_ADJUST,
//	ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL,
//	ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB,
//	ANIMATION_LAYER_MOVEMENT_MOVE,
//	ANIMATION_LAYER_MOVEMENT_STRAFECHANGE,
//	ANIMATION_LAYER_WHOLE_BODY,
//	ANIMATION_LAYER_FLASHED,
//	ANIMATION_LAYER_FLINCH,
//	ANIMATION_LAYER_ALIVELOOP,
//	ANIMATION_LAYER_LEAN,
//	ANIMATION_LAYER_COUNT,
//};

void rebuilt::SetWeightDeltaRate ( CCSGOGamePlayerAnimState *state, int layer_idx, float old_weight ) {
	if ( state->m_flLastUpdateIncrement != 0.0f ) {
		const auto layer = state->m_pPlayer->m_AnimOverlay ( ) + layer_idx;

		float weight_delta_rate = ( layer->m_weight - old_weight ) / state->m_flLastUpdateIncrement;

		if ( layer->m_weight_delta_rate != weight_delta_rate )
			layer->m_weight_delta_rate = weight_delta_rate;
	}
}

void rebuilt::IncrementLayerCycleWeightRateGeneric ( CCSGOGamePlayerAnimState *state, int layer_idx ) {
	float prev_weight = state->m_pPlayer->m_AnimOverlay ( ) [ layer_idx ].m_weight;
	IncrementLayerCycle ( state, layer_idx, false );
	SetWeight ( state, layer_idx, GetLayerIdealWeightFromSequenceCycle ( state, layer_idx ) );
	SetWeightDeltaRate ( state, layer_idx, prev_weight );
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

void rebuilt::Update ( CCSGOPlayerAnimState *animstate, const ang_t &angles ) {
	auto game_state = ( CCSGOGamePlayerAnimState * ) animstate;
	bool &m_bJumping = game_state->m_bFlashed;

	if ( !( game_state->m_pPlayer->m_fFlags ( ) & FL_ONGROUND )
		&& game_state->m_bOnGround
		&& game_state->m_pPlayer->m_vecVelocity ( ).z > 0.0f ) {
		m_bJumping = true;
		rebuilt::SetSequence ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, rebuilt::SelectWeightedSequence ( game_state, ACT_CSGO_JUMP ) );
	}

	if ( !( g_inputpred.data.m_nOldButtons & FL_ONGROUND ) ) {
		rebuilt::SetSequence ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, rebuilt::SelectWeightedSequence ( game_state, ACT_CSGO_FALL ) );
		m_bJumping = true;
	}
	else
		rebuild_modifiers ( game_state );

	bool bPreviouslyOnLadder = game_state->m_bOnLadder;
	game_state->m_bOnLadder = !game_state->m_bOnGround && game_state->m_pPlayer->m_MoveType ( ) == MOVETYPE_LADDER;
	bool bStartedLadderingThisFrame = ( !bPreviouslyOnLadder && game_state->m_bOnLadder );
	bool bStoppedLadderingThisFrame = ( bPreviouslyOnLadder && !game_state->m_bOnLadder );

	if ( game_state->m_bOnGround ) {
		bool next_landing = false;

		if ( !game_state->m_bLanding && ( game_state->m_bLandedOnGroundThisFrame || bStoppedLadderingThisFrame ) ) {
			rebuilt::SetSequence ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, rebuilt::SelectWeightedSequence ( game_state, ( game_state->m_flDurationInAir > 1 ) ? ACT_CSGO_LAND_HEAVY : ACT_CSGO_LAND_LIGHT ) );
			//rebuilt::SetCycle ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
			next_landing = true;
		}

		game_state->m_flDurationInAir = 0;

		if ( next_landing && rebuilt::GetLayerActivity ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) != ACT_CSGO_CLIMB_LADDER ) {
			m_bJumping = false;

			rebuilt::IncrementLayerCycle ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, false );
			rebuilt::IncrementLayerCycle ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false );

			game_state->m_pPlayer->m_flPoseParameter ( ) [ POSE_JUMP_FALL ] = 0.f;

			if ( rebuilt::IsLayerSequenceCompleted ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) ) {
				game_state->m_bLanding = false;
				rebuilt::SetWeight ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
				rebuilt::SetWeight ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, 0 );
				game_state->m_flLandAnimMultiplier = 1.0f;
			}
			else {
				float flLandWeight = rebuilt::GetLayerIdealWeightFromSequenceCycle ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) * game_state->m_flLandAnimMultiplier;
				flLandWeight *= std::clamp< float > ( ( 1.0f - game_state->m_flAnimDuckAmount ), 0.2f, 1.0f );

				rebuilt::SetWeight ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLandWeight );

				float flCurrentJumpFallWeight = game_state->m_pPlayer->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_weight;
				if ( flCurrentJumpFallWeight > 0 ) {
					flCurrentJumpFallWeight = valve_math::Approach ( 0, flCurrentJumpFallWeight, game_state->m_flLastUpdateIncrement * 10.0f );
					rebuilt::SetWeight ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flCurrentJumpFallWeight );
				}
			}
		}

		if ( !game_state->m_bLanding && !m_bJumping && game_state->m_flLadderWeight <= 0 ) {
			rebuilt::SetWeight ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
		}
	}
	else if ( !game_state->m_bOnLadder ) {
		game_state->m_bLanding = false;

		// we're in the air
		if ( game_state->m_bLeftTheGroundThisFrame || bStoppedLadderingThisFrame ) {
			// If entered the air by jumping, then we already set the jump activity.
			// But if we're in the air because we strolled off a ledge or the floor collapsed or something,
			// we need to set the fall activity here.
			if ( !m_bJumping ) {
				rebuilt::SetSequence ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, rebuilt::SelectWeightedSequence ( game_state, ACT_CSGO_FALL ) );
			}

			game_state->m_flDurationInAir = 0;
		}

		game_state->m_flDurationInAir += game_state->m_flLastUpdateIncrement;

		rebuilt::IncrementLayerCycle ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false );

		// increase jump weight
		float flJumpWeight = game_state->m_pPlayer->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_weight;
		float flNextJumpWeight = rebuilt::GetLayerIdealWeightFromSequenceCycle ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL );

		if ( flNextJumpWeight > flJumpWeight )
			rebuilt::SetWeight ( game_state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flNextJumpWeight );

		auto smoothstep_bounds = [ ] ( float edge0, float edge1, float x ) {
			x = std::clamp< float > ( ( x - edge0 ) / ( edge1 - edge0 ), 0, 1 );
			return x * x * ( 3 - 2 * x );
		};

		float flLingeringLandWeight = game_state->m_pPlayer->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ].m_weight;

		if ( flLingeringLandWeight > 0 ) {
			flLingeringLandWeight *= smoothstep_bounds ( 0.2f, 0.0f, game_state->m_flDurationInAir );
			rebuilt::SetWeight ( game_state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLingeringLandWeight );
		}

		// blend jump into fall. This is a no-op if we're playing a fall anim.
		game_state->m_pPlayer->m_flPoseParameter ( ) [ POSE_JUMP_FALL ] = std::clamp ( smoothstep_bounds ( 0.72f, 1.52f, game_state->m_flDurationInAir ), 0.0f, 1.0f );
	}

	game::UpdateAnimationState ( animstate, angles );
}

void rebuilt::InvalidatePhysicsRecursive ( void *player, int change_flags ) {
	static auto CBaseEntity__InvalidatePhysicsRecursive = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56" ) ).as< void ( __thiscall * )( void *, int ) > ( );
	CBaseEntity__InvalidatePhysicsRecursive ( player, change_flags );
}

void rebuilt::SetWeight ( CCSGOGamePlayerAnimState *state, int layer_idx, float weight ) {
	auto layer = state->m_pPlayer->m_AnimOverlay ( ) + layer_idx;

	if ( layer->m_owner && layer->m_weight != weight ) {
		if ( weight == 0.0f || weight == 0.0f ) {
			InvalidatePhysicsRecursive ( state->m_pPlayer, 10 );
		}
	}

	layer->m_weight = weight;
}

float rebuilt::GetLayerIdealWeightFromSequenceCycle ( CCSGOGamePlayerAnimState *state, int layer_idx ) {
	static auto CCSGOPlayerAnimState__GetLayerIdealWeightFromSeqCycle = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 EC 08 53 56 8B 35 ? ? ? ? 57 8B F9 8B CE 8B 06 FF 90" ) ).as< float ( __thiscall * )( CCSGOGamePlayerAnimState *, int ) > ( );
	return CCSGOPlayerAnimState__GetLayerIdealWeightFromSeqCycle ( state, layer_idx );
}

bool rebuilt::IsLayerSequenceCompleted ( CCSGOGamePlayerAnimState *state, int layer_idx ) {
	const auto layer = state->m_pPlayer->m_AnimOverlay ( ) + layer_idx;

	if ( layer )
		return ( ( layer->m_cycle + ( state->m_flLastUpdateIncrement * layer->m_playback_rate ) ) >= 1 );

	return false;
}

void rebuilt::IncrementLayerCycle ( CCSGOGamePlayerAnimState *state, int layer_idx, bool allow_loop ) {
	const auto layer = state->m_pPlayer->m_AnimOverlay ( ) + layer_idx;

	if ( !layer )
		return;

	if ( abs ( layer->m_playback_rate ) <= 0 )
		return;

	float flCurrentCycle = state->m_pPlayer->m_AnimOverlay ( ) [ layer_idx ].m_cycle;
	flCurrentCycle += state->m_flLastUpdateIncrement * state->m_pPlayer->m_AnimOverlay ( ) [ layer_idx ].m_playback_rate;

	if ( !allow_loop && flCurrentCycle >= 1 ) {
		flCurrentCycle = 0.999f;
	}

	SetCycle ( state, layer_idx, valve_math::ClampCycle ( flCurrentCycle ) );
}

int rebuilt::GetLayerActivity ( CCSGOGamePlayerAnimState *state, AnimationLayer_t layer_idx ) {
	static auto CCSGOPlayerAnimState__GetLayerActivity = pattern::find ( g_csgo.m_client_dll, XOR ( "51 53 56 8B 35 ? ? ? ? 57 8B F9 8B CE 8B 06 FF 90 ? ? ? ? 8B 7F 60 83 BF" ) ).as< int ( __thiscall * )( CCSGOGamePlayerAnimState *, int ) > ( );
	return CCSGOPlayerAnimState__GetLayerActivity ( state, layer_idx );
}

void rebuilt::CalculatePoses ( CCSGOPlayerAnimState *state, Player *player, float *poses, float feet_yaw ) {
	auto state_game = ( CCSGOGamePlayerAnimState * ) state;

	memcpy ( poses, player->m_flPoseParameter ( ), sizeof ( poses ) );

	//m_tPoseParamMappings [ PLAYER_POSE_PARAM_MOVE_BLEND_WALK ].SetValue ( m_pPlayer, ( 1.0f - m_flWalkToRunTransition ) * ( 1.0f - m_flAnimDuckAmount ) );
	//m_tPoseParamMappings [ PLAYER_POSE_PARAM_MOVE_BLEND_RUN ].SetValue ( m_pPlayer, ( m_flWalkToRunTransition ) * ( 1.0f - m_flAnimDuckAmount ) );
	//m_tPoseParamMappings [ PLAYER_POSE_PARAM_MOVE_BLEND_CROUCH_WALK ].SetValue ( m_pPlayer, m_flAnimDuckAmount );

	const uint32_t buttons = *reinterpret_cast< uint32_t * >( reinterpret_cast< uintptr_t >( player ) + 0x31E8 );

	bool moveRight = ( buttons & ( IN_MOVERIGHT ) ) != 0;
	bool moveLeft = ( buttons & ( IN_MOVELEFT ) ) != 0;
	bool moveForward = ( buttons & ( IN_FORWARD ) ) != 0;
	bool moveBackward = ( buttons & ( IN_BACK ) ) != 0;

	vec3_t vecForward;
	vec3_t vecRight;

	valve_math::AngleVectors ( { 0, state_game->m_flFootYaw, 0 }, &vecForward, &vecRight, NULL );
	vecRight.normalize_place ( );

	float flVelToRightDot = state_game->m_vecVelocityNormalizedNonZero.dot ( vecRight );
	float flVelToForwardDot = state_game->m_vecVelocityNormalizedNonZero.dot ( vecForward );

	bool bStrafeRight = ( state_game->m_flSpeedAsPortionOfWalkTopSpeed >= 0.73f && moveRight && !moveLeft && flVelToRightDot < -0.63f );
	bool bStrafeLeft = ( state_game->m_flSpeedAsPortionOfWalkTopSpeed >= 0.73f && moveLeft && !moveRight && flVelToRightDot > 0.63f );
	bool bStrafeForward = ( state_game->m_flSpeedAsPortionOfWalkTopSpeed >= 0.65f && moveForward && !moveBackward && flVelToForwardDot < -0.55f );
	bool bStrafeBackward = ( state_game->m_flSpeedAsPortionOfWalkTopSpeed >= 0.65f && moveBackward && !moveForward && flVelToForwardDot > 0.55f );

	*reinterpret_cast< bool * >( reinterpret_cast< uintptr_t >( player ) + 0x39E0 ) = ( bStrafeRight || bStrafeLeft || bStrafeForward || bStrafeBackward );

	if ( *reinterpret_cast< bool * >( reinterpret_cast< uintptr_t >( player ) + 0x39E0 ) ) {
		if ( !state_game->m_bStrafeChanging ) {
			state_game->m_flDurationStrafing = 0;
		}

		state_game->m_bStrafeChanging = true;

		state_game->m_flStrafeChangeWeight = valve_math::Approach ( 1, state_game->m_flStrafeChangeWeight, state_game->m_flLastUpdateIncrement * 20 );
		state_game->m_flStrafeChangeCycle = valve_math::Approach ( 0, state_game->m_flStrafeChangeCycle, state_game->m_flLastUpdateIncrement * 10 );

		poses [ POSE_STRAFE_YAW ] = std::clamp ( valve_math::AngleNormalize ( state_game->m_flMoveYaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;
	}
	else if ( state_game->m_flStrafeChangeWeight > 0 ) {
		state_game->m_flDurationStrafing += state_game->m_flLastUpdateIncrement;

		if ( state_game->m_flDurationStrafing > 0.08f )
			state_game->m_flStrafeChangeWeight = valve_math::Approach ( 0, state_game->m_flStrafeChangeWeight, state_game->m_flLastUpdateIncrement * 5 );

		state_game->m_nStrafeSequence = player->LookupSequence ( "strafe" );
		float flRate = player->GetSequenceCycleRate ( state_game->m_nStrafeSequence );
		state_game->m_flStrafeChangeCycle = std::clamp< float > ( state_game->m_flStrafeChangeCycle + state_game->m_flLastUpdateIncrement * flRate, 0, 1 );
	}

	poses [ POSE_MOVE_BLEND_WALK ] = ( 1.0f - state_game->m_flWalkToRunTransition ) * ( 1.0f - state_game->m_flAnimDuckAmount );
	poses [ POSE_MOVE_BLEND_RUN ] = ( state_game->m_flWalkToRunTransition ) * ( 1.0f - state_game->m_flAnimDuckAmount );
	poses [ POSE_MOVE_BLEND_CROUCH ] = state_game->m_flAnimDuckAmount;

	poses [ POSE_BODY_YAW ] = std::clamp ( valve_math::AngleNormalize ( state_game->m_flMoveYaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;

	float flAimYaw = valve_math::AngleDiff ( state_game->m_flEyeYaw, state_game->m_flFootYaw );

	if ( flAimYaw >= 0 && state_game->m_flAimYawMax != 0 )
		flAimYaw = ( flAimYaw / state_game->m_flAimYawMax ) * 60.0f;
	else if ( state_game->m_flAimYawMin != 0 )
		flAimYaw = ( flAimYaw / state_game->m_flAimYawMin ) * -60.0f;

	poses [ POSE_BODY_YAW ] = std::clamp ( valve_math::AngleNormalize ( flAimYaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;

	float flPitch = valve_math::AngleDiff ( state_game->m_flEyePitch, 0 );

	if ( flPitch > 0 ) {
		flPitch = ( flPitch / state_game->m_flAimPitchMax ) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MAX;
	}
	else {
		flPitch = ( flPitch / state_game->m_flAimPitchMin ) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MIN;
	}

	poses [ POSE_BODY_PITCH ] = std::clamp ( flPitch, -90.f, 90.f ) / 180.0f + 0.5f;
	poses [ POSE_SPEED ] = std::clamp ( state_game->m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f );
	poses [ POSE_STAND ] = std::clamp ( 1.0f - ( state_game->m_flAnimDuckAmount * state_game->m_flInAirSmoothValue ), 0.f, 1.f );
}