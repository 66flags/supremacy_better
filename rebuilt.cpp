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

// ty ses ^-^
void rebuilt::Update ( CCSGOGamePlayerAnimState *animstate, const ang_t &angles, int tick ) {
	const float backup_cur_time = g_csgo.m_globals->m_curtime;
	const float backup_frametime = g_csgo.m_globals->m_frametime;
	const int backup_framecount = g_csgo.m_globals->m_frame;

	/* update with "correct" information */
	g_csgo.m_globals->m_curtime = static_cast< float >( tick ) * g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_frame = tick;

	/* force game to use non-abs velocity */
	animstate->m_pPlayer->m_iEFlags ( ) |= 0x1000;
	animstate->m_pPlayer->m_vecAbsVelocity ( ) = animstate->m_pPlayer->m_vecVelocity ( );

	auto animlayers = animstate->m_pPlayer->m_AnimOverlay ( );
	const auto backup_animlayers = *animlayers;
	const CCSGOGamePlayerAnimState backup_animstate = *animstate;

	const bool on_ground = ( g_cl.m_local->m_fFlags ( ) & FL_ONGROUND ) != 0;
	const bool landed_on_ground_this_frame = on_ground && !animstate->m_bOnGround;

	/*
	*	LMAO f*k it, we will use flashed as a place to hold the jumping var (that only exists on the server)
	*	I dont want to have to create a separate structure to hold just a few server-specific animstate members
	*	This doesnt get used in client anim code anyways (afaik)...
	*/
	bool &m_bJumping = animstate->m_bFlashed;

	/*
	case PLAYERANIMEVENT_JUMP: // 6
		{
			// note: this event means a jump is definitely happening, not just that a jump is desired
			m_bJumping = true;
			SetLayerSequence( ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, SelectSequenceFromActMods( ACT_CSGO_JUMP ) );
			break;
		}
	*	Very ghetto fix:
	*/
	if ( !on_ground
		&& animstate->m_bOnGround
		/* make sure we are moving UP and not DOWN (so change in flags doesnt trigger PLAYERANIMEVENT_JUMP when just falling off a platform) */
		&& animstate->m_pPlayer->m_vecVelocity ( ).z > 0.0f ) {
		m_bJumping = true;
		SetSequence ( animstate, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL,SelectWeightedSequence ( animstate, ACT_CSGO_JUMP ) );
	}

	/* rebuild some missing anim code */
	const bool stopped_moving_this_frame = animstate->m_flVelocityLengthXY <= 0.0f && animstate->m_flDurationStill <= 0.0f;

	const bool previously_on_ladder = animstate->m_bOnLadder;
	const bool on_ladder = animstate->m_pPlayer->m_MoveType ( ) == MOVETYPE_LADDER;
	const bool started_laddering_this_frame = ( !previously_on_ladder && on_ladder );
	const bool stopped_laddering_this_frame = ( previously_on_ladder && !on_ladder );

	/* CCSGOPlayerAnimState::SetupVelocity() ANIMATION_LAYER_ADJUST calculations */
	if ( !animstate->m_bAdjustStarted && stopped_moving_this_frame && on_ground && !on_ladder && !animstate->m_bLanding && animstate->m_flStutterStep < 50.0f ) {
		SetSequence ( animstate, ANIMATION_LAYER_ADJUST, SelectWeightedSequence ( animstate, 980 ) );
		animstate->m_bAdjustStarted = true;
	}

	const int adjust_activity = GetLayerActivity ( animstate, ANIMATION_LAYER_ADJUST );

	if ( adjust_activity == 980 || adjust_activity == 979 ) {
		if ( animstate->m_bAdjustStarted && animstate->m_flSpeedAsPortionOfCrouchTopSpeed  <= 0.25f ) {
			IncrementLayerCycleWeightRateGeneric ( animstate, ANIMATION_LAYER_ADJUST );
			animstate->m_bAdjustStarted = !IsLayerSequenceCompleted ( animstate, ANIMATION_LAYER_ADJUST );
		}
		else {
			animstate->m_bAdjustStarted = false;

			auto &layer = animstate->m_pPlayer->m_AnimOverlay( ) [ ANIMATION_LAYER_ADJUST ];
			const float weight = layer.m_weight;
			layer.m_weight = valve_math::Approach ( 0.0f, weight, animstate->m_flLastUpdateIncrement * 5.0f );
			SetWeightDeltaRate ( animstate, ANIMATION_LAYER_ADJUST, weight );
		}
	}

	Weapon *weapon = animstate->m_pPlayer->GetActiveWeapon ( );

	const float max_speed_run = weapon ? std::max ( weapon->GetWpnData ( )->m_max_player_speed, 0.001f ) : CS_PLAYER_SPEED_RUN;
	const vec3_t velocity = animstate->m_pPlayer->m_vecVelocity ( );

	animstate->m_flVelocityLengthXY = std::min ( velocity.length ( ), CS_PLAYER_SPEED_RUN );

	if ( animstate->m_flVelocityLengthXY > 0.0f )
		animstate->m_vecVelocityNormalizedNonZero = velocity.normalized ( );

	animstate->m_flSpeedAsPortionOfWalkTopSpeed  = animstate->m_flVelocityLengthXY / ( max_speed_run * CS_PLAYER_SPEED_WALK_MODIFIER );

	/* only for localplayer (we dont have enemy usercmds) */
	if ( g_cl.m_local && animstate->m_pPlayer == g_cl.m_local ) {
		/* FROM src/game/prediction.cpp */
		const uint32_t buttons = *reinterpret_cast< uint32_t * >( reinterpret_cast< uintptr_t >( animstate->m_pPlayer ) + 0x3208 );

		bool move_right = ( buttons & ( IN_MOVERIGHT ) ) != 0;
		bool move_left = ( buttons & ( IN_MOVELEFT ) ) != 0;
		bool move_forward = ( buttons & ( IN_FORWARD ) ) != 0;
		bool move_backward = ( buttons & ( IN_BACK ) ) != 0;
		
		vec3_t forward, right;
		math::AngleVectors ( ang_t ( 0, animstate->m_flFootYaw, 0 ), &forward, &right );
		right.normalize ( );

		const float vel_to_right_dot = animstate->m_vecVelocityNormalizedNonZero.dot ( right );
		const float vel_to_forward_dot = animstate->m_vecVelocityNormalizedNonZero.dot ( forward );

		const bool strafe_right = ( animstate->m_flSpeedAsPortionOfWalkTopSpeed  >= 0.73f && move_right && !move_left && vel_to_right_dot < -0.63f );
		const bool strafe_left = ( animstate->m_flSpeedAsPortionOfWalkTopSpeed  >= 0.73f && move_left && !move_right && vel_to_right_dot > 0.63f );
		const bool strafe_forward = ( animstate->m_flSpeedAsPortionOfWalkTopSpeed  >= 0.65f && move_forward && !move_backward && vel_to_forward_dot < -0.55f );
		const bool strafe_back = ( animstate->m_flSpeedAsPortionOfWalkTopSpeed  >= 0.65f && move_backward && !move_forward && vel_to_forward_dot > 0.55f );

		*reinterpret_cast< bool * >( reinterpret_cast< uintptr_t >( animstate->m_pPlayer ) + 0x39E0 ) = ( strafe_right || strafe_left || strafe_forward || strafe_back );
	}

	if ( ( animstate->m_flLadderWeight > 0.0f || on_ladder ) && started_laddering_this_frame )
		SetSequence ( animstate, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, SelectWeightedSequence ( animstate, 987 ) );

	if ( on_ground ) {
		bool next_landing = false;
		if ( !animstate->m_bLanding && ( landed_on_ground_this_frame || stopped_laddering_this_frame ) ) {
			SetSequence ( animstate, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, SelectWeightedSequence ( animstate, ( animstate->m_flDurationInAir > 1.0f ) ? 989 : 988 ) );
			next_landing = true;
		}

		if ( next_landing && GetLayerActivity ( animstate, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) != 987 ) {
			m_bJumping = false;

			/* some client code here */

			auto &layer = animstate->m_pPlayer->m_AnimOverlay ( )[ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ];
			/* dont actually mess up the value */
			const float backup_cycle = layer.m_cycle;
			/* run this calculation ahead of time so we dont get 1-tick-late landing */
			IncrementLayerCycle ( animstate, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, false );

			if ( IsLayerSequenceCompleted ( animstate, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) )
				next_landing = false;

			layer.m_cycle = backup_cycle;
		}

		if ( !next_landing && !m_bJumping && animstate->m_flLadderWeight <= 0.0f ) {
			auto &layer = animstate->m_pPlayer->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ];
			layer.m_weight = 0.0f;
		}
	}
	else if ( !on_ladder ) {
		if ( animstate->m_bOnGround || stopped_laddering_this_frame ) {
			if ( !m_bJumping ) {
				//play_animation ( animstate, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, 986 );
				SetSequence ( animstate, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, SelectWeightedSequence ( animstate, ACT_CSGO_FALL ) );
			}
		}
	}

	/* update animation */
	animstate->m_pPlayer->UpdateAnimState ( ( CCSGOPlayerAnimState* )animstate, angles );

	/* undo bad clientsided adjust layer calculation */
	auto &adjust_layer = animlayers [ ANIMATION_LAYER_ADJUST ];

	if ( adjust_layer.m_weight > 0.0f ) {
		adjust_layer.m_cycle = std::clamp ( adjust_layer.m_cycle - animstate->m_flLastUpdateIncrement * adjust_layer.m_playback_rate, 0.0f, 0.999f );
		adjust_layer.m_weight = std::clamp ( adjust_layer.m_weight - animstate->m_flLastUpdateIncrement *adjust_layer.m_weight_delta_rate, 0.0f, 1.0f );
	}

	/* rebuild some more animation code */
	if ( animstate->m_flVelocityLengthXY <= CS_PLAYER_SPEED_STOPPED
		&& animstate->m_bOnGround
		&& !animstate->m_bOnLadder
		&& !animstate->m_bLanding
		&& animstate->m_flLastUpdateIncrement > 0.0f
		&& abs ( valve_math::AngleDiff ( animstate->m_flFootYawLast, animstate->m_flFootYaw ) / animstate->m_flLastUpdateIncrement > CSGO_ANIM_READJUST_THRESHOLD ) ) {
		SetSequence ( animstate, ANIMATION_LAYER_ADJUST, SelectWeightedSequence ( animstate, ACT_CSGO_IDLE_TURN_BALANCEADJUST ) );
		animstate->m_bAdjustStarted = true;
	}

	/* restore client vars */
	g_csgo.m_globals->m_curtime = backup_cur_time;
	g_csgo.m_globals->m_frametime = backup_frametime;
	g_csgo.m_globals->m_frame = backup_framecount;
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