#include "includes.h"

#define CSGO_ANIM_AIMMATRIX_DEFAULT_YAW_MAX 58.0f
#define CSGO_ANIM_AIMMATRIX_DEFAULT_YAW_MIN -58.0f
#define CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MAX 90.0f
#define CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MIN -90.0f

#define CSGO_ANIM_LOWER_CATCHUP_IDLE	100.0f
#define CSGO_ANIM_AIM_NARROW_WALK	0.8f
#define CSGO_ANIM_AIM_NARROW_RUN	0.5f
#define CSGO_ANIM_AIM_NARROW_CROUCHMOVING	0.5f
#define CSGO_ANIM_LOWER_CATCHUP_WITHIN	3.0f
#define CSGO_ANIM_LOWER_REALIGN_DELAY	1.1f
#define CSGO_ANIM_READJUST_THRESHOLD	120.0f
#define EIGHT_WAY_WIDTH 22.5f

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

void rebuilt::UpdateAnimationState ( CCSGOGamePlayerAnimState *state, float eyeYaw, float eyePitch, bool bForce ) {
	static auto cache_sequences = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F8 83 EC 34 53 56 8B F1 57 8B 46 60" ) ).as< bool ( __thiscall * )( void * ) > ( );
	static auto *enable_invalidate_bone_cache = pattern::find ( g_csgo.m_client_dll, XOR ( "C6 05 ? ? ? ? ? F3 0F 5F 05 ? ? ? ? F3 0F 11 47 ? F3 0F" ) ).add ( 0x2 ).to ( ).as < bool * > ( );

	if ( !state )
		return;

	const auto player = state->m_pPlayer;

	if ( !player || !player->alive ( ) || !cache_sequences ( state ) )
		return;

	{
		// Apply recoil angle to aim matrix so bullets still come out of the gun straight while spraying
		eyePitch = valve_math::AngleNormalize ( eyePitch + *reinterpret_cast< float * >( reinterpret_cast< uintptr_t >( player ) + 0xB844 ) );
	}

	state->m_flLastUpdateIncrement = std::max< float > ( 0.0f, g_csgo.m_globals->m_curtime - state->m_flLastUpdateTime ); // negative values possible when clocks on client and server go out of sync..

	*enable_invalidate_bone_cache = false;

	state->m_flEyeYaw = valve_math::AngleNormalize ( eyeYaw );
	state->m_flEyePitch = valve_math::AngleNormalize ( eyePitch );
	state->m_vecPositionCurrent = player->GetAbsOrigin ( );
	state->m_pWeapon = player->GetActiveWeapon ( );

	// purge layer dispatches on weapon change and init
	if ( state->m_pWeapon != state->m_pWeaponLast || state->m_bFirstRunSinceInit ) {
		*reinterpret_cast< int * >( reinterpret_cast< uintptr_t >( player ) + 0xA30 ) = 0;

		for ( int i = 0; i < ANIMATION_LAYER_COUNT; i++ ) {
			const auto layer = player->m_AnimOverlay ( ) ? ( player->m_AnimOverlay ( ) + 1 ) : nullptr;

			if ( layer ) {
				*reinterpret_cast< int * >( reinterpret_cast< uintptr_t >( layer ) + 8 ) = 0;
				*reinterpret_cast< int * >( reinterpret_cast< uintptr_t >( layer ) + 12 ) = -1;
				*reinterpret_cast< int * >( reinterpret_cast< uintptr_t >( layer ) + 16 ) = -1;
			}
		}
	}

	state->m_flAnimDuckAmount = std::clamp< float > ( valve_math::Approach ( std::clamp< float > ( player->m_flDuckAmount ( ) + state->m_flDuckAdditional, 0.f, 1.f ), state->m_flAnimDuckAmount, state->m_flLastUpdateIncrement * 6.0f ), 0, 1 );

	// no matter what, we're always playing 'default' underneath
	{
		g_csgo.m_model_cache->BeginLock ( );

		util::get_method < void ( __thiscall * )( void *, int ) > ( player, 213 )( player, 0 );
		*reinterpret_cast< float * >( reinterpret_cast< uintptr_t >( player ) + 0xA18 ) = 0.0f;

		if ( *reinterpret_cast< float * >( reinterpret_cast< uintptr_t >( player ) + 0xA14 ) != 0.0f ) {
			*reinterpret_cast< float * >( reinterpret_cast< uintptr_t >( player ) + 0xA14 ) = 0;
			InvalidatePhysicsRecursive ( player, 8 );
		}

		g_csgo.m_model_cache->EndLock ( );
	}

	// all the layers get set up here
	SetupVelocity ( state );			// calculate speed and set up body yaw values
	SetupAimMatrix ( state );			// aim matrices are full body, so they not only point the weapon down the eye dir, they can crouch the idle player
	SetupWeaponAction ( state );		// firing, reloading, silencer-swapping, deploying
	SetupMovement ( state );			// jumping, climbing, ground locomotion, post-weapon crouch-stand
	SetupAliveLoop ( state );			// breathe and fidget
	SetupWholeBodyAction ( state );		// defusing, planting, whole-body custom events (INLINED)
	SetupFlashedReaction ( state );		// shield eyes from flashbang
	SetupFlinch ( state );				// flinch when shot
	SetupLean ( state );				// lean into acceleration

	for ( auto i = 0; i < ANIMATION_LAYER_COUNT; i++ ) {
		const auto layer = player->m_AnimOverlay ( ) ? ( player->m_AnimOverlay ( ) + i ) : nullptr;

		if ( layer && !layer->m_sequence )
			SetWeight ( state, i, 0.0f );
	}

	// force abs angles on client and server to line up hitboxes
	player->SetAbsAngles ( { 0, state->m_flFootYaw, 0 } );

	// restore bonecache invalidation (NOT ON SERVER)
	*enable_invalidate_bone_cache = true;

	state->m_pWeaponLast = state->m_pWeapon;
	state->m_vecPositionLast = state->m_vecPositionCurrent;
	state->m_bFirstRunSinceInit = false;
	state->m_flLastUpdateTime = g_csgo.m_globals->m_curtime;
	state->m_nLastUpdateFrame = g_csgo.m_globals->m_frame;
}

void rebuilt::IncrementLayerWeight ( CCSGOGamePlayerAnimState *state, int layer_idx ) {
	auto layer = state->m_pPlayer->m_AnimOverlay ( ) + layer_idx;

	if ( !layer )
		return;

	if ( abs ( layer->m_weight_delta_rate ) <= 0 )
		return;

	float flCurrentWeight = layer->m_weight;

	flCurrentWeight += state->m_flLastUpdateIncrement * layer->m_weight_delta_rate;
	flCurrentWeight = std::clamp< float > ( flCurrentWeight, 0, 1 );

	SetWeight ( state, layer_idx, flCurrentWeight );
}

void rebuilt::SetupWholeBodyAction ( CCSGOGamePlayerAnimState *state ) {
	int layer_idx = ANIMATION_LAYER_WHOLE_BODY;

	if ( state->m_pPlayer->m_AnimOverlay ( ) [ layer_idx ].m_weight > 0 ) {
		IncrementLayerCycle ( state, layer_idx, false );
		IncrementLayerWeight ( state, layer_idx );
	}
}

void rebuilt::DoAnimationEvent ( CCSGOGamePlayerAnimState *state, int event, int data ) {
	const auto player = state->m_pPlayer;

	if ( !player || !state )
		return;

	// TODO: recreate all animation events on the client.
}

void rebuilt::SetupVelocity ( CCSGOGamePlayerAnimState *state ) {
	auto player = state->m_pPlayer;

	if ( !state || !player )
		return;

	static auto GetSeqDesc = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 79 04 00 75 25 8B 45 08 8B 09 85 C0 78 08 3B 81" ) ).as < void *( __thiscall * )( void *, int ) > ( );

	g_csgo.m_model_cache->BeginLock ( );

	// update the local velocity variables so we don't recalculate them unnecessarily
	vec3_t vecAbsVelocity = player->m_vecAbsVelocity ( );

	// save vertical velocity component
	state->m_flVelocityLengthZ = vecAbsVelocity.z;

	// discard z component
	vecAbsVelocity.z = 0;

	// remember if the player is accelerating.
	state->m_bPlayerIsAccelerating = ( state->m_vecVelocityLast.length_sqr ( ) < vecAbsVelocity.length_sqr ( ) );

	// rapidly approach ideal velocity instead of instantly adopt it. This helps smooth out instant velocity changes, like
	// when the player runs headlong into a wall and their velocity instantly becomes zero.
	state->m_vecVelocity = valve_math::Approach ( vecAbsVelocity, state->m_vecVelocity, state->m_flLastUpdateIncrement * 2000 );
	state->m_vecVelocityNormalized = state->m_vecVelocity.normalized ( );

	// save horizontal velocity length
	state->m_flVelocityLengthXY = std::min< float > ( state->m_vecVelocity.length ( ), CS_PLAYER_SPEED_RUN );

	if ( state->m_flVelocityLengthXY > 0 )
		state->m_vecVelocityNormalizedNonZero = state->m_vecVelocityNormalized;

	float flMaxSpeedRun = state->m_pWeapon ? std::max< float > ( player->m_bIsScoped ( ) ? state->m_pWeapon->GetWpnData ( )->m_max_player_speed_alt : state->m_pWeapon->GetWpnData ( )->m_max_player_speed, 0.001f ) : CS_PLAYER_SPEED_RUN;

	state->m_flSpeedAsPortionOfRunTopSpeed = std::clamp< float > ( state->m_flVelocityLengthXY / flMaxSpeedRun, 0.f, 1.f );
	state->m_flSpeedAsPortionOfWalkTopSpeed = state->m_flVelocityLengthXY / ( flMaxSpeedRun * CS_PLAYER_SPEED_WALK_MODIFIER );
	state->m_flSpeedAsPortionOfCrouchTopSpeed = state->m_flVelocityLengthXY / ( flMaxSpeedRun * CS_PLAYER_SPEED_DUCK_MODIFIER );

	if ( state->m_flSpeedAsPortionOfWalkTopSpeed >= 1 )
		state->m_flStaticApproachSpeed = state->m_flVelocityLengthXY;
	else if ( state->m_flSpeedAsPortionOfWalkTopSpeed < 0.5f )
		state->m_flStaticApproachSpeed = valve_math::Approach ( 80, state->m_flStaticApproachSpeed, state->m_flLastUpdateIncrement * 60 );

	bool bStartedMovingThisFrame = false;
	bool bStoppedMovingThisFrame = false;

	if ( state->m_flVelocityLengthXY > 0 ) {
		bStartedMovingThisFrame = ( state->m_flDurationMoving <= 0 );
		state->m_flDurationStill = 0;
		state->m_flDurationMoving += state->m_flLastUpdateIncrement;
	}
	else {
		bStoppedMovingThisFrame = ( state->m_flDurationStill <= 0 );
		state->m_flDurationMoving = 0;
		state->m_flDurationStill += state->m_flLastUpdateIncrement;
	}

	if ( !state->m_bAdjustStarted && bStoppedMovingThisFrame && state->m_bOnGround && !state->m_bOnLadder && !state->m_bLanding && state->m_flStutterStep < 50 ) {
		SetSequence ( state, ANIMATION_LAYER_ADJUST, SelectWeightedSequence ( state, ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ) );
		state->m_bAdjustStarted = true;
	}

	if ( GetLayerActivity ( state, ANIMATION_LAYER_ADJUST ) == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ||
		GetLayerActivity ( state, ANIMATION_LAYER_ADJUST ) == ACT_CSGO_IDLE_TURN_BALANCEADJUST ) {
		if ( state->m_bAdjustStarted && state->m_flSpeedAsPortionOfCrouchTopSpeed <= 0.25f ) {
			IncrementLayerCycleWeightRateGeneric ( state, ANIMATION_LAYER_ADJUST );
			state->m_bAdjustStarted = !( IsLayerSequenceCompleted ( state, ANIMATION_LAYER_ADJUST ) );
		}
		else {
			state->m_bAdjustStarted = false;
			float flWeight = player->m_AnimOverlay ( ) [ ANIMATION_LAYER_ADJUST ].m_weight;
			SetWeight ( state, ANIMATION_LAYER_ADJUST, valve_math::Approach ( 0, flWeight, state->m_flLastUpdateIncrement * 5 ) );
			SetWeightDeltaRate ( state, ANIMATION_LAYER_ADJUST, flWeight );
		}
	}

	// if the player is looking far enough to either side, turn the feet to keep them within the extent of the aim matrix
	state->m_flFootYawLast = state->m_flFootYaw;
	state->m_flFootYaw = std::clamp< float > ( state->m_flFootYaw, -360.f, 360.f );
	float flEyeFootDelta = valve_math::AngleDiff ( state->m_flEyeYaw, state->m_flFootYaw );

	// narrow the available aim matrix width as speed increases
	float flAimMatrixWidthRange = math::Lerp ( std::clamp< float > ( state->m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f ), 1.0f, math::Lerp ( state->m_flWalkToRunTransition, CSGO_ANIM_AIM_NARROW_WALK, CSGO_ANIM_AIM_NARROW_RUN ) );

	if ( state->m_flAnimDuckAmount > 0 )
		flAimMatrixWidthRange = math::Lerp ( state->m_flAnimDuckAmount * std::clamp< float > ( state->m_flSpeedAsPortionOfCrouchTopSpeed, 0.f, 1.f ), flAimMatrixWidthRange, CSGO_ANIM_AIM_NARROW_CROUCHMOVING );

	float flTempYawMax = state->m_flAimYawMax * flAimMatrixWidthRange;
	float flTempYawMin = state->m_flAimYawMin * flAimMatrixWidthRange;

	if ( flEyeFootDelta > flTempYawMax )
		state->m_flFootYaw = state->m_flEyeYaw - abs ( flTempYawMax );
	else if ( flEyeFootDelta < flTempYawMin )
		state->m_flFootYaw = state->m_flEyeYaw + abs ( flTempYawMin );

	state->m_flFootYaw = valve_math::AngleNormalize ( state->m_flFootYaw );

	// pull the lower body direction towards the eye direction, but only when the player is moving
	if ( state->m_bOnGround ) {
		if ( state->m_flVelocityLengthXY > 0.1f ) {
			state->m_flFootYaw = valve_math::ApproachAngle ( state->m_flEyeYaw, state->m_flFootYaw, state->m_flLastUpdateIncrement * ( 30.0f + 20.0f * state->m_flWalkToRunTransition ) );

			// state->m_flLowerBodyRealignTimer = g_csgo.m_globals->curtime + ( CSGO_ANIM_LOWER_REALIGN_DELAY * 0.2f );
			// player->m_flLowerBodyYawTarget.Set( m_flEyeYaw );
		}
		else {
			state->m_flFootYaw = valve_math::ApproachAngle ( player->m_flLowerBodyYawTarget ( ), state->m_flFootYaw, state->m_flLastUpdateIncrement * CSGO_ANIM_LOWER_CATCHUP_IDLE );

			// if ( g_csgo.m_globals->curtime > m_flLowerBodyRealignTimer && abs( valve_math::AngleDiff( state->m_flFootYaw, state->m_flEyeYaw ) ) > 35.0f )
			// {
			// 	m_flLowerBodyRealignTimer = gpGlobals->curtime + CSGO_ANIM_LOWER_REALIGN_DELAY;
			// 	player->m_flLowerBodyYawTarget.Set( m_flEyeYaw );
			// }
		}
	}

	if ( state->m_flVelocityLengthXY <= CS_PLAYER_SPEED_STOPPED && state->m_bOnGround && !state->m_bOnLadder && !state->m_bLanding && state->m_flLastUpdateIncrement > 0 && abs ( valve_math::AngleDiff ( state->m_flFootYawLast, state->m_flFootYaw ) / state->m_flLastUpdateIncrement > CSGO_ANIM_READJUST_THRESHOLD ) ) {
		SetSequence ( state, ANIMATION_LAYER_ADJUST, SelectWeightedSequence ( state, ACT_CSGO_IDLE_TURN_BALANCEADJUST ) );
		state->m_bAdjustStarted = true;
	}

	// the final model render yaw is aligned to the foot yaw
	if ( state->m_flVelocityLengthXY > 0 && state->m_bOnGround ) {
		// convert horizontal velocity vec to angular yaw
		float flRawYawIdeal = ( atan2 ( -state->m_vecVelocity [ 1 ], -state->m_vecVelocity [ 0 ] ) * 180 / M_PI );

		if ( flRawYawIdeal < 0 )
			flRawYawIdeal += 360;

		state->m_flMoveYawIdeal = valve_math::AngleNormalize ( valve_math::AngleDiff ( flRawYawIdeal, state->m_flFootYaw ) );
	}

	// delta between current yaw and ideal velocity derived target (possibly negative!)
	state->m_flMoveYawCurrentToIdeal = valve_math::AngleNormalize ( valve_math::AngleDiff ( state->m_flMoveYawIdeal, state->m_flMoveYaw ) );

	if ( bStartedMovingThisFrame && state->m_flMoveWeight <= 0 ) {
		state->m_flMoveYaw = state->m_flMoveYawIdeal;

		// select a special starting cycle that's set by the animator in content
		int nMoveSeq = player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_MOVE ].m_sequence;

		if ( nMoveSeq != -1 ) {
			void *seqdesc = GetSeqDesc ( player->GetModelPtr ( ), nMoveSeq );

			if ( *reinterpret_cast< int * >( reinterpret_cast< uintptr_t >( seqdesc ) + 0x31 ) > 0 ) {
				if ( abs ( valve_math::AngleDiff ( state->m_flMoveYaw, 180 ) ) <= EIGHT_WAY_WIDTH ) //N
				{
					state->m_flPrimaryCycle = player->GetFirstSequenceAnimTag ( nMoveSeq, ANIMTAG_STARTCYCLE_N, 0, 1 );
				}
				else if ( abs ( valve_math::AngleDiff ( state->m_flMoveYaw, 135 ) ) <= EIGHT_WAY_WIDTH ) //NE
				{
					state->m_flPrimaryCycle = player->GetFirstSequenceAnimTag ( nMoveSeq, ANIMTAG_STARTCYCLE_NE, 0, 1 );
				}
				else if ( abs ( valve_math::AngleDiff ( state->m_flMoveYaw, 90 ) ) <= EIGHT_WAY_WIDTH ) //E
				{
					state->m_flPrimaryCycle = player->GetFirstSequenceAnimTag ( nMoveSeq, ANIMTAG_STARTCYCLE_E, 0, 1 );
				}
				else if ( abs ( valve_math::AngleDiff ( state->m_flMoveYaw, 45 ) ) <= EIGHT_WAY_WIDTH ) //SE
				{
					state->m_flPrimaryCycle = player->GetFirstSequenceAnimTag ( nMoveSeq, ANIMTAG_STARTCYCLE_SE, 0, 1 );
				}
				else if ( abs ( valve_math::AngleDiff ( state->m_flMoveYaw, 0 ) ) <= EIGHT_WAY_WIDTH ) //S
				{
					state->m_flPrimaryCycle = player->GetFirstSequenceAnimTag ( nMoveSeq, ANIMTAG_STARTCYCLE_S, 0, 1 );
				}
				else if ( abs ( valve_math::AngleDiff ( state->m_flMoveYaw, -45 ) ) <= EIGHT_WAY_WIDTH ) //SW
				{
					state->m_flPrimaryCycle = player->GetFirstSequenceAnimTag ( nMoveSeq, ANIMTAG_STARTCYCLE_SW, 0, 1 );
				}
				else if ( abs ( valve_math::AngleDiff ( state->m_flMoveYaw, -90 ) ) <= EIGHT_WAY_WIDTH ) //W
				{
					state->m_flPrimaryCycle = player->GetFirstSequenceAnimTag ( nMoveSeq, ANIMTAG_STARTCYCLE_W, 0, 1 );
				}
				else if ( abs ( valve_math::AngleDiff ( state->m_flMoveYaw, -135 ) ) <= EIGHT_WAY_WIDTH ) //NW
				{
					state->m_flPrimaryCycle = player->GetFirstSequenceAnimTag ( nMoveSeq, ANIMTAG_STARTCYCLE_NW, 0, 1 );
				}
			}
		}
	}
	else {
		if ( player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_STRAFECHANGE ].m_weight >= 1 ) {
			state->m_flMoveYaw = state->m_flMoveYawIdeal;
		}
		else {
			float flMoveWeight = math::Lerp ( state->m_flAnimDuckAmount, std::clamp< float > ( state->m_flSpeedAsPortionOfWalkTopSpeed, 0, 1 ), std::clamp< float > ( state->m_flSpeedAsPortionOfCrouchTopSpeed, 0, 1 ) );
			float flRatio = valve_math::Bias ( flMoveWeight, 0.18f ) + 0.1f;

			state->m_flMoveYaw = valve_math::AngleNormalize ( state->m_flMoveYaw + ( state->m_flMoveYawCurrentToIdeal * flRatio ) );
		}
	}

	player->m_flPoseParameter ( ) [ POSE_MOVE_YAW ] = std::clamp ( valve_math::AngleNormalize ( state->m_flMoveYaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;

	float flAimYaw = valve_math::AngleDiff ( state->m_flEyeYaw, state->m_flFootYaw );

	if ( flAimYaw >= 0 && state->m_flAimYawMax != 0 )
		flAimYaw = ( flAimYaw / state->m_flAimYawMax ) * 60.0f;
	else if ( state->m_flAimYawMin != 0 )
		flAimYaw = ( flAimYaw / state->m_flAimYawMin ) * -60.0f;

	player->m_flPoseParameter ( ) [ POSE_BODY_YAW ] = std::clamp ( valve_math::AngleNormalize ( flAimYaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;

	// we need non-symmetrical arbitrary min/max bounds for vertical aim (pitch) too
	float flPitch = valve_math::AngleDiff ( state->m_flEyePitch, 0 );

	if ( flPitch > 0 ) {
		flPitch = ( flPitch / state->m_flAimPitchMax ) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MAX;
	}
	else {
		flPitch = ( flPitch / state->m_flAimPitchMin ) * CSGO_ANIM_AIMMATRIX_DEFAULT_PITCH_MIN;
	}

	player->m_flPoseParameter ( ) [ POSE_BODY_PITCH ] = std::clamp ( flPitch, -90.f, 90.f ) / 180.0f + 0.5f;
	player->m_flPoseParameter ( ) [ POSE_SPEED ] = std::clamp ( state->m_flSpeedAsPortionOfWalkTopSpeed, 0.f, 1.f );
	player->m_flPoseParameter ( ) [ POSE_STAND ] = std::clamp ( 1.0f - ( state->m_flAnimDuckAmount * state->m_flInAirSmoothValue ), 0.f, 1.f );
}

#define CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED 2.0f
#define CSGO_ANIM_ONGROUND_FUZZY_APPROACH 8.0f
#define CSGO_ANIM_ONGROUND_FUZZY_APPROACH_CROUCH 16.0f
#define CSGO_ANIM_LADDER_CLIMB_COVERAGE 100.0f
#define CSGO_ANIM_RUN_ANIM_PLAYBACK_MULTIPLIER 0.85f
#define MAX_ANIMSTATE_ANIMNAME_CHARS 64
#define ANIM_TRANSITION_WALK_TO_RUN 0
#define ANIM_TRANSITION_RUN_TO_WALK 1

void rebuilt::UpdateAnimLayer ( CCSGOGamePlayerAnimState *state, int layer_idx, int seq, float playback_rate, float weight, float cycle ) {
	static auto UpdateLayerOrderPreset = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 51 53 56 57 8B F9 83 7F 60 00 0F 84 ? ? ? ? 83" ) ).as < void ( __thiscall * )( CCSGOGamePlayerAnimState *, int, int ) > ( );

	if ( seq > 1 ) {
		g_csgo.m_model_cache->BeginLock ( );

		const auto layer = state->m_pPlayer->m_AnimOverlay ( ) + layer_idx;

		if ( !layer )
			return;

		layer->m_sequence = seq;
		layer->m_playback_rate = playback_rate;

		SetCycle ( state, layer_idx, std::clamp< float > ( cycle, 0, 1 ) );
		SetWeight ( state, layer_idx, std::clamp< float > ( weight, 0, 1 ) );
		UpdateLayerOrderPreset ( state, layer_idx, seq );

		g_csgo.m_model_cache->EndLock ( );
	}
}

void rebuilt::SetupMovement ( CCSGOGamePlayerAnimState *state ) {
	auto player = state->m_pPlayer;

	if ( !state || !player )
		return;

	g_csgo.m_model_cache->BeginLock ( );

	bool &m_bJumping = state->m_bFlashed;

	// recreate jumping animation event on the client
	if ( !( player->m_fFlags ( ) & FL_ONGROUND )
		&& state->m_bOnGround
		&& player == g_cl.m_local
		&& player->m_vecVelocity ( ).z > 0.0f ) {
		m_bJumping = true;
		SetSequence ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, SelectWeightedSequence ( state, ACT_CSGO_JUMP ) );
	}

	if ( state->m_flWalkToRunTransition > 0 && state->m_flWalkToRunTransition < 1 ) {
		//currently transitioning between walk and run
		if ( state->m_bWalkToRunTransitionState == ANIM_TRANSITION_WALK_TO_RUN ) {
			state->m_flWalkToRunTransition += state->m_flLastUpdateIncrement * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
		}
		else // m_bWalkToRunTransitionState == ANIM_TRANSITION_RUN_TO_WALK
		{
			state->m_flWalkToRunTransition -= state->m_flLastUpdateIncrement * CSGO_ANIM_WALK_TO_RUN_TRANSITION_SPEED;
		}
		state->m_flWalkToRunTransition = std::clamp< float > ( state->m_flWalkToRunTransition, 0, 1 );
	}

	if ( state->m_flVelocityLengthXY > ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && state->m_bWalkToRunTransitionState == ANIM_TRANSITION_RUN_TO_WALK ) {
		//crossed the walk to run threshold
		state->m_bWalkToRunTransitionState = ANIM_TRANSITION_WALK_TO_RUN;
		state->m_flWalkToRunTransition = std::max< float > ( 0.01f, state->m_flWalkToRunTransition );
	}
	else if ( state->m_flVelocityLengthXY < ( CS_PLAYER_SPEED_RUN * CS_PLAYER_SPEED_WALK_MODIFIER ) && state->m_bWalkToRunTransitionState == ANIM_TRANSITION_WALK_TO_RUN ) {
		//crossed the run to walk threshold
		state->m_bWalkToRunTransitionState = ANIM_TRANSITION_RUN_TO_WALK;
		state->m_flWalkToRunTransition = std::min< float > ( 0.99f, state->m_flWalkToRunTransition );
	}

	player->m_flPoseParameter ( ) [ POSE_MOVE_BLEND_WALK ] = ( 1.0f - state->m_flWalkToRunTransition ) * ( 1.0f - state->m_flAnimDuckAmount );
	player->m_flPoseParameter ( ) [ POSE_MOVE_BLEND_RUN ] = ( state->m_flWalkToRunTransition ) * ( 1.0f - state->m_flAnimDuckAmount );
	player->m_flPoseParameter ( ) [ POSE_MOVE_BLEND_CROUCH ] = state->m_flAnimDuckAmount;

	char szWeaponMoveSeq [ 64 ];
	sprintf_s ( szWeaponMoveSeq, "move_%s", state->GetWeaponPrefix ( ) );

	int nWeaponMoveSeq = player->LookupSequence ( szWeaponMoveSeq );

	if ( nWeaponMoveSeq == -1 ) {
		nWeaponMoveSeq = player->LookupSequence ( "move" );
	}

	if ( *reinterpret_cast< int * >( reinterpret_cast< uintptr_t >( player ) + 0x38D0 ) != state->m_nPreviousMoveState ) {
		state->m_flStutterStep += 10;
	}

	state->m_nPreviousMoveState = *reinterpret_cast< int * >( reinterpret_cast< uintptr_t >( player ) + 0x38D0 );
	state->m_flStutterStep = std::clamp< float > ( valve_math::Approach ( 0, state->m_flStutterStep, state->m_flLastUpdateIncrement * 40 ), 0, 100 );

	float flTargetMoveWeight = math::Lerp ( state->m_flAnimDuckAmount, std::clamp< float > ( state->m_flSpeedAsPortionOfWalkTopSpeed, 0, 1 ), std::clamp< float > ( state->m_flSpeedAsPortionOfCrouchTopSpeed, 0, 1 ) );

	if ( state->m_flMoveWeight <= flTargetMoveWeight ) {
		state->m_flMoveWeight = flTargetMoveWeight;
	}
	else {
		state->m_flMoveWeight = valve_math::Approach ( flTargetMoveWeight, state->m_flMoveWeight, state->m_flLastUpdateIncrement * valve_math::RemapValClamped ( state->m_flStutterStep, 0.0f, 100.0f, 2, 20 ) );
	}

	vec3_t vecMoveYawDir;
	math::AngleVectors ( { 0, valve_math::AngleNormalize ( state->m_flFootYaw + state->m_flMoveYaw + 180 ), 0 }, &vecMoveYawDir );
	float flYawDeltaAbsDot = abs ( state->m_vecVelocityNormalizedNonZero.dot ( vecMoveYawDir ) );
	state->m_flMoveWeight *= valve_math::Bias ( flYawDeltaAbsDot, 0.2 );

	float flMoveWeightWithAirSmooth = state->m_flMoveWeight * state->m_flInAirSmoothValue;

	// dampen move weight for landings
	flMoveWeightWithAirSmooth *= std::max< float > ( ( 1.0f - player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ].m_weight ), 0.55f );

	float flMoveCycleRate = 0;
	if ( state->m_flVelocityLengthXY > 0 ) {
		flMoveCycleRate = player->GetSequenceCycleRate ( nWeaponMoveSeq );
		float flSequenceGroundSpeed = std::max< float > ( player->GetSequenceMoveDist ( player->GetModelPtr ( ), nWeaponMoveSeq ) / ( 1.0f / flMoveCycleRate ), 0.001f );
		flMoveCycleRate *= state->m_flVelocityLengthXY / flSequenceGroundSpeed;

		flMoveCycleRate *= math::Lerp ( state->m_flWalkToRunTransition, 1.0f, CSGO_ANIM_RUN_ANIM_PLAYBACK_MULTIPLIER );
	}

	float flLocalCycleIncrement = ( flMoveCycleRate * state->m_flLastUpdateIncrement );
	state->m_flPrimaryCycle = valve_math::ClampCycle ( state->m_flPrimaryCycle + flLocalCycleIncrement );

	flMoveWeightWithAirSmooth = std::clamp< float > ( flMoveWeightWithAirSmooth, 0, 1 );
	UpdateAnimLayer ( state, ANIMATION_LAYER_MOVEMENT_MOVE, nWeaponMoveSeq, flLocalCycleIncrement, flMoveWeightWithAirSmooth, state->m_flPrimaryCycle );

	const uint32_t buttons = *reinterpret_cast< uint32_t * >( reinterpret_cast< uintptr_t >( player ) + 0x31E8 );

	bool moveRight = ( buttons & IN_MOVERIGHT ) != 0;
	bool moveLeft = ( buttons & IN_MOVELEFT ) != 0;
	bool moveForward = ( buttons & IN_FORWARD ) != 0;
	bool moveBackward = ( buttons & IN_BACK ) != 0;

	vec3_t vecForward;
	vec3_t vecRight;

	valve_math::AngleVectors ( { 0, state->m_flFootYaw, 0 }, &vecForward, &vecRight, NULL );
	vecRight.normalize_place ( );

	float flVelToRightDot = state->m_vecVelocityNormalizedNonZero.dot ( vecRight );
	float flVelToForwardDot = state->m_vecVelocityNormalizedNonZero.dot ( vecForward );

	bool bStrafeRight = ( state->m_flSpeedAsPortionOfWalkTopSpeed >= 0.73f && moveRight && !moveLeft && flVelToRightDot < -0.63f );
	bool bStrafeLeft = ( state->m_flSpeedAsPortionOfWalkTopSpeed >= 0.73f && moveLeft && !moveRight && flVelToRightDot > 0.63f );
	bool bStrafeForward = ( state->m_flSpeedAsPortionOfWalkTopSpeed >= 0.65f && moveForward && !moveBackward && flVelToForwardDot < -0.55f );
	bool bStrafeBackward = ( state->m_flSpeedAsPortionOfWalkTopSpeed >= 0.65f && moveBackward && !moveForward && flVelToForwardDot > 0.55f );

	*reinterpret_cast< bool * >( reinterpret_cast< uintptr_t >( player ) + 0x39E0 ) = ( bStrafeRight || bStrafeLeft || bStrafeForward || bStrafeBackward );

	if ( *reinterpret_cast< bool * >( reinterpret_cast< uintptr_t >( player ) + 0x39E0 ) ) {
		if ( !state->m_bStrafeChanging ) {
			state->m_flDurationStrafing = 0;
		}

		state->m_bStrafeChanging = true;

		state->m_flStrafeChangeWeight = valve_math::Approach ( 1, state->m_flStrafeChangeWeight, state->m_flLastUpdateIncrement * 20 );
		state->m_flStrafeChangeCycle = valve_math::Approach ( 0, state->m_flStrafeChangeCycle, state->m_flLastUpdateIncrement * 10 );

		player->m_flPoseParameter ( ) [ POSE_STRAFE_YAW ] = std::clamp ( valve_math::AngleNormalize ( state->m_flMoveYaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;
	}
	else if ( state->m_flStrafeChangeWeight > 0 ) {
		state->m_flDurationStrafing += state->m_flLastUpdateIncrement;

		if ( state->m_flDurationStrafing > 0.08f )
			state->m_flStrafeChangeWeight = valve_math::Approach ( 0, state->m_flStrafeChangeWeight, state->m_flLastUpdateIncrement * 5 );

		state->m_nStrafeSequence = player->LookupSequence ( "strafe" );
		float flRate = player->GetSequenceCycleRate ( state->m_nStrafeSequence );
		state->m_flStrafeChangeCycle = std::clamp< float > ( state->m_flStrafeChangeCycle + state->m_flLastUpdateIncrement * flRate, 0, 1 );
	}

	if ( state->m_flStrafeChangeWeight <= 0 ) {
		state->m_bStrafeChanging = false;
	}

	// keep track of if the player is on the ground, and if the player has just touched or left the ground since the last check
	bool bPreviousGroundState = state->m_bOnGround;
	state->m_bOnGround = ( player->m_fFlags ( ) & FL_ONGROUND );

	state->m_bLandedOnGroundThisFrame = ( !state->m_bFirstRunSinceInit && bPreviousGroundState != state->m_bOnGround && state->m_bOnGround );
	state->m_bLeftTheGroundThisFrame = ( bPreviousGroundState != state->m_bOnGround && !state->m_bOnGround );

	float flDistanceFell = 0;
	if ( state->m_bLeftTheGroundThisFrame ) {
		state->m_flLeftGroundHeight = state->m_vecPositionCurrent.z;
	}

	if ( state->m_bLandedOnGroundThisFrame ) {
		flDistanceFell = abs ( state->m_flLeftGroundHeight - state->m_vecPositionCurrent.z );
		float flDistanceFallNormalizedBiasRange = valve_math::Bias ( valve_math::RemapValClamped ( flDistanceFell, 12.0f, 72.0f, 0.0f, 1.0f ), 0.4f );

		state->m_flLandAnimMultiplier = std::clamp< float > ( valve_math::Bias ( state->m_flDurationInAir, 0.3f ), 0.1f, 1.0f );
		state->m_flDuckAdditional = std::max< float > ( state->m_flLandAnimMultiplier, flDistanceFallNormalizedBiasRange );
	}
	else {
		state->m_flDuckAdditional = valve_math::Approach ( 0, state->m_flDuckAdditional, state->m_flLastUpdateIncrement * 2 );
	}

	// the in-air smooth value is a fuzzier representation of if the player is on the ground or not.
	// It will approach 1 when the player is on the ground and 0 when in the air. Useful for blending jump animations.
	state->m_flInAirSmoothValue = valve_math::Approach ( state->m_bOnGround ? 1 : 0, state->m_flInAirSmoothValue, math::Lerp ( state->m_flAnimDuckAmount, CSGO_ANIM_ONGROUND_FUZZY_APPROACH, CSGO_ANIM_ONGROUND_FUZZY_APPROACH_CROUCH ) * state->m_flLastUpdateIncrement );
	state->m_flInAirSmoothValue = std::clamp< float > ( state->m_flInAirSmoothValue, 0, 1 );

	state->m_flStrafeChangeWeight *= ( 1.0f - state->m_flAnimDuckAmount );
	state->m_flStrafeChangeWeight *= state->m_flInAirSmoothValue;
	state->m_flStrafeChangeWeight = std::clamp< float > ( state->m_flStrafeChangeWeight, 0, 1 );

	if ( state->m_nStrafeSequence != -1 )
		UpdateAnimLayer ( state, ANIMATION_LAYER_MOVEMENT_STRAFECHANGE, state->m_nStrafeSequence, 0, state->m_flStrafeChangeWeight, state->m_flStrafeChangeCycle );

	bool bPreviouslyOnLadder = state->m_bOnLadder;
	state->m_bOnLadder = !state->m_bOnGround && player->m_MoveType ( ) == MOVETYPE_LADDER;
	bool bStartedLadderingThisFrame = ( !bPreviouslyOnLadder && state->m_bOnLadder );
	bool bStoppedLadderingThisFrame = ( bPreviouslyOnLadder && !state->m_bOnLadder );

	// useless tbh
	if ( bStartedLadderingThisFrame || bStoppedLadderingThisFrame ) {
		//m_footLeft.m_flLateralWeight = 0;
		//m_footRight.m_flLateralWeight = 0;
	}

	if ( state->m_flLadderWeight > 0 || state->m_bOnLadder ) {
		if ( bStartedLadderingThisFrame ) {
			SetSequence ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, SelectWeightedSequence ( state, ACT_CSGO_CLIMB_LADDER ) );
		}

		if ( abs ( state->m_flVelocityLengthZ ) > 100 ) {
			state->m_flLadderSpeed = valve_math::Approach ( 1, state->m_flLadderSpeed, state->m_flLastUpdateIncrement * 10.0f );
		}
		else {
			state->m_flLadderSpeed = valve_math::Approach ( 0, state->m_flLadderSpeed, state->m_flLastUpdateIncrement * 10.0f );
		}

		state->m_flLadderSpeed = std::clamp< float > ( state->m_flLadderSpeed, 0, 1 );

		if ( state->m_bOnLadder ) {
			state->m_flLadderWeight = valve_math::Approach ( 1, state->m_flLadderWeight, state->m_flLastUpdateIncrement * 5.0f );
		}
		else {
			state->m_flLadderWeight = valve_math::Approach ( 0, state->m_flLadderWeight, state->m_flLastUpdateIncrement * 10.0f );
		}

		state->m_flLadderWeight = std::clamp< float > ( state->m_flLadderWeight, 0, 1 );

		vec3_t vecLadderNormal = player->m_vecLadderNormal ( );
		ang_t angLadder;

		math::VectorAngles ( vecLadderNormal, angLadder );

		float flLadderYaw = valve_math::AngleDiff ( angLadder.y, state->m_flFootYaw );

		player->m_flPoseParameter ( ) [ POSE_LADDER_YAW ] = std::clamp ( valve_math::AngleNormalize ( flLadderYaw ), -180.0f, 180.0f ) / 360.0f + 0.5f;

		float flLadderClimbCycle = player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ].m_cycle;
		flLadderClimbCycle += ( state->m_vecPositionCurrent.z - state->m_vecPositionLast.z ) * math::Lerp ( state->m_flLadderSpeed, 0.010f, 0.004f );

		player->m_flPoseParameter ( ) [ POSE_LADDER_SPEED ] = std::clamp ( state->m_flLadderSpeed, 0.0f, 1.0f );

		if ( GetLayerActivity ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) == ACT_CSGO_CLIMB_LADDER )
			SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, state->m_flLadderWeight );

		SetCycle ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLadderClimbCycle );

		if ( state->m_bOnLadder ) {
			float flIdealJumpWeight = 1.0f - state->m_flLadderWeight;

			if ( player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_weight > flIdealJumpWeight ) {
				SetWeight ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flIdealJumpWeight );
			}
		}
	}
	else {
		state->m_flLadderSpeed = 0;
	}

	if ( state->m_bOnGround ) {
		if ( !state->m_bLanding && ( state->m_bLandedOnGroundThisFrame || bStoppedLadderingThisFrame ) ) {
			SetSequence ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, SelectWeightedSequence ( state, ( state->m_flDurationInAir > 1 ) ? ACT_CSGO_LAND_HEAVY : ACT_CSGO_LAND_LIGHT ) );
			SetCycle ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
			state->m_bLanding = true;
		}

		state->m_flDurationInAir = 0;

		if ( state->m_bLanding && GetLayerActivity ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) != ACT_CSGO_CLIMB_LADDER ) {
			m_bJumping = false;

			IncrementLayerCycle ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, false );
			IncrementLayerCycle ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false );

			player->m_flPoseParameter ( ) [ POSE_JUMP_FALL ] = 0.f;

			if ( IsLayerSequenceCompleted ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) ) {
				state->m_bLanding = false;
				SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
				SetWeight ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, 0 );
				state->m_flLandAnimMultiplier = 1.0f;
			}
			else {
				float flLandWeight = GetLayerIdealWeightFromSequenceCycle ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ) * state->m_flLandAnimMultiplier;
				flLandWeight *= std::clamp< float > ( ( 1.0f - state->m_flAnimDuckAmount ), 0.2f, 1.0f );

				SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLandWeight );

				float flCurrentJumpFallWeight = player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_weight;
				if ( flCurrentJumpFallWeight > 0 ) {
					flCurrentJumpFallWeight = valve_math::Approach ( 0, flCurrentJumpFallWeight, state->m_flLastUpdateIncrement * 10.0f );
					SetWeight ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flCurrentJumpFallWeight );
				}
			}
		}

		if ( !state->m_bLanding && !m_bJumping && state->m_flLadderWeight <= 0 ) {
			SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, 0 );
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
				SetSequence ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, SelectWeightedSequence ( state, ACT_CSGO_FALL ) );
			}

			state->m_flDurationInAir = 0;
		}

		state->m_flDurationInAir += state->m_flLastUpdateIncrement;

		IncrementLayerCycle ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, false );

		// increase jump weight
		float flJumpWeight = player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_weight;
		float flNextJumpWeight = GetLayerIdealWeightFromSequenceCycle ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL );

		if ( flNextJumpWeight > flJumpWeight )
			SetWeight ( state, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, flNextJumpWeight );

		auto smoothstep_bounds = [ ] ( float edge0, float edge1, float x ) {
			x = std::clamp< float > ( ( x - edge0 ) / ( edge1 - edge0 ), 0, 1 );
			return x * x * ( 3 - 2 * x );
		};

		float flLingeringLandWeight = player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ].m_weight;

		if ( flLingeringLandWeight > 0 ) {
			flLingeringLandWeight *= smoothstep_bounds ( 0.2f, 0.0f, state->m_flDurationInAir );
			SetWeight ( state, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, flLingeringLandWeight );
		}

		// blend jump into fall. This is a no-op if we're playing a fall anim.
		player->m_flPoseParameter ( ) [ POSE_JUMP_FALL ] = std::clamp ( smoothstep_bounds ( 0.72f, 1.52f, state->m_flDurationInAir ), 0.0f, 1.0f );
	}

	g_csgo.m_model_cache->EndLock ( );
}

void rebuilt::SetupAliveLoop ( CCSGOGamePlayerAnimState *state ) {
	// ehhh, this function is entirely inlined so we can't call it and this function is quite useless.
	int nLayer = ANIMATION_LAYER_ALIVELOOP;

	IncrementLayerCycle ( state, nLayer, true );
}

void rebuilt::SetupFlashedReaction ( CCSGOGamePlayerAnimState *state ) {
	static auto setup_flashed_reaction = pattern::find ( g_csgo.m_client_dll, XOR ( "51 8B 41 60 83 B8 ?? ?? ?? ?? ?? 74 3E 8B 90 ?? ?? ?? ?? 81 C2" ) ).as< std::uintptr_t > ( );
	using setup_flashed_reaction_type = void ( __thiscall * )( CCSGOGamePlayerAnimState * );

	reinterpret_cast< setup_flashed_reaction_type >( setup_flashed_reaction )( state );
}

void rebuilt::SetupFlinch ( CCSGOGamePlayerAnimState *state ) {
	static auto setup_flinch = pattern::find ( g_csgo.m_client_dll, XOR ( "E8 ? ? ? ? 8B CF E8 ? ? ? ? 33 C0 89 44 24" ) ).rel32 ( 0x1 ).as< std::uintptr_t > ( );
	using setup_flinch_type = void ( __thiscall * )( CCSGOGamePlayerAnimState * );

	reinterpret_cast< setup_flinch_type >( setup_flinch )( state );
}

void rebuilt::SetupAimMatrix ( CCSGOGamePlayerAnimState *state ) {
	static auto update_aim_matrix = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 81 EC ? ? ? ? 53 56 57 8B 3D ? ? ? ? 8B" ) ).as< std::uintptr_t > ( );
	using update_aim_matrix_type = void ( __thiscall * )( CCSGOGamePlayerAnimState * );

	reinterpret_cast< update_aim_matrix_type >( update_aim_matrix )( state );
}

void rebuilt::SetupWeaponAction ( CCSGOGamePlayerAnimState *state ) {
	static auto setup_weapon_action = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 81 EC ? ? ? ? 53 56 57 8B 3D ? ? ? ? 8B" ) ).as< std::uintptr_t > ( );
	using setup_weapon_action_type = void ( __thiscall * )( CCSGOGamePlayerAnimState * );

	reinterpret_cast< setup_weapon_action_type >( setup_weapon_action )( state );
}

void rebuilt::SetupLean ( CCSGOGamePlayerAnimState *state ) {
	static auto setup_lean = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F8 A1 ? ? ? ? 83 EC 20 F3 0F 10 48 ? 56 57 8B F9" ) ).as < std::uintptr_t > ( );
	using setup_lean_type = void ( __thiscall * )( CCSGOGamePlayerAnimState * );

	reinterpret_cast< setup_lean_type >( setup_lean )( state );
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