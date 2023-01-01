#include "includes.h"

Aimbot g_aimbot { };;

bool Aimbot::FixVelocity ( Player *ent, LagRecord *previous, vec3_t &vel, C_AnimationLayer *animlayers, const vec3_t &origin ) {
	const auto idx = ent->index ( );

	vel.zero ( );

	if ( !!( ent->m_fFlags ( ) & FL_ONGROUND ) && !animlayers [ 6 ].m_weight ) {
		vel.zero ( );
		return true;
	}

	auto time_difference = std::max < float > ( game::TICKS_TO_TIME ( 1 ), ent->m_flSimulationTime ( ) - previous->m_sim_time );
	auto origin_delta = origin - previous->m_origin;

	if ( origin_delta.is_zero ( ) ) {
		vel.zero ( );
		return true;
	}

	if ( game::TIME_TO_TICKS ( ent->m_flSimulationTime ( ) - previous->m_sim_time ) <= 1 ) {
		vel = origin_delta / time_difference;
		return true;
	}

	auto fixed = false;

	/* skeet */
	if ( !!( ent->m_fFlags ( ) & FL_ONGROUND ) ) {
		vel.z = 0.0f;
		vel = origin_delta / time_difference;

		auto max_speed = 260.0f;
		const auto weapon = g_cl.m_local->GetActiveWeapon ( );

		if ( weapon && weapon->GetWpnData ( ) )
			max_speed = g_cl.m_local->m_bIsScoped ( ) ? weapon->GetWpnData ( )->m_max_player_speed_alt : weapon->GetWpnData ( )->m_max_player_speed;

		if ( animlayers [ 6 ].m_weight <= 0.0f ) {
			vel.zero ( );
			fixed = true;
		}
		else {
			if ( animlayers [ 6 ].m_playback_rate > 0.0f ) {
				auto origin_delta_vel_norm = vel.normalized ( );
				origin_delta_vel_norm.z = 0.0f;

				auto origin_delta_vel_len = vel.length_2d ( );

				const auto flMoveWeightWithAirSmooth = animlayers [ 6 ].m_weight / std::max ( 1.0f - animlayers [ 5 ].m_weight, 0.55f );
				const auto flTargetMoveWeight_to_speed2d = math::Lerp ( max_speed * 0.52f, max_speed * 0.34f, ent->m_flDuckAmount ( ) ) * flMoveWeightWithAirSmooth;

				const auto speed_as_portion_of_run_top_speed = 0.35f * ( 1.0f - animlayers [ 11 ].m_weight );

				if ( animlayers [ 11 ].m_weight > 0.0f && animlayers [ 11 ].m_weight < 1.0f ) {
					vel = origin_delta_vel_norm * ( max_speed * ( speed_as_portion_of_run_top_speed + 0.55f ) );
					fixed = true;
				}
				else if ( flMoveWeightWithAirSmooth < 0.95f || flTargetMoveWeight_to_speed2d > origin_delta_vel_len ) {
					vel = origin_delta_vel_norm * flTargetMoveWeight_to_speed2d;
					fixed = flMoveWeightWithAirSmooth < 0.95f;
				}
				else {
					auto flTargetMoveWeight_adjusted_speed2d = std::min ( max_speed, util::get_method < float ( __thiscall * )( Player * ) > ( ent, 269 ) ( ent ) );

					if ( !!( ent->m_fFlags ( ) & FL_DUCKING ) )
						flTargetMoveWeight_adjusted_speed2d *= 0.34f;
					else if ( *reinterpret_cast< bool * >( reinterpret_cast< uintptr_t >( ent ) + 0x9975 ) )
						flTargetMoveWeight_adjusted_speed2d *= 0.52f;

					if ( origin_delta_vel_len > flTargetMoveWeight_adjusted_speed2d ) {
						vel = origin_delta_vel_norm * flTargetMoveWeight_adjusted_speed2d;
						fixed = true;
					}
				}
			}
		}
	}
	else {
		if ( animlayers [ 4 ].m_weight > 0.0f
			&& animlayers [ 4 ].m_playback_rate > 0.0f
			&& rebuilt::GetLayerActivity ( ( CCSGOGamePlayerAnimState * ) ent->m_PlayerAnimState ( ), ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ) == 985 /* act_csgo_jump */ ) {
			vel.z = ( ( animlayers [ 4 ].m_cycle / animlayers [ 4 ].m_playback_rate ) / game::TICKS_TO_TIME ( 1 ) + 0.5f ) * game::TICKS_TO_TIME ( 1 );
			vel.z = g_csgo.sv_jump_impulse->GetFloat ( ) - vel.z * g_csgo.sv_gravity->GetFloat ( );
		}

		vel.x = origin_delta.x / time_difference;
		vel.y = origin_delta.y / time_difference;

		fixed = true;
	}

	///* predict vel dir */
	//if ( records.size ( ) >= 2 && records [ 0 ].m_simtime - records [ 1 ].m_simtime > cs::ticks2time ( 1 ) && vel.length_2d ( ) > 0.0f ) {
	//	const auto last_avg_vel = ( records [ 0 ].m_origin - records [ 1 ].m_origin ) / ( records [ 0 ].m_simtime - records [ 1 ].m_simtime );
	//
	//	if ( last_avg_vel.length_2d ( ) > 0.0f ) {
	//		float deg_1 = cs::rad2deg ( atan2 ( origin_delta.y, origin_delta.x ) );
	//		float deg_2 = cs::rad2deg ( atan2 ( last_avg_vel.y, last_avg_vel.x ) );
	//
	//		float deg_delta = cs::normalize ( deg_1 - deg_2 );
	//		float deg_lerp = cs::normalize ( deg_1 + deg_delta * 0.5f );
	//		float rad_dir = cs::deg2rad ( deg_lerp );
	//
	//		float sin_dir, cos_dir;
	//		cs::sin_cos ( rad_dir, &sin_dir, &cos_dir );
	//
	//		float vel_len = vel.length_2d ( );
	//
	//		vel.x = cos_dir * vel_len;
	//		vel.y = sin_dir * vel_len;
	//	}
	//}

	return fixed;
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

VelocityDetail_t AimPlayer::FixVelocity ( C_AnimationLayer *animlayers, LagRecord *previous, Player *player ) {
	if ( !animlayers )
		return DETAIL_NONE;

	const auto &jump_or_fall_layer = animlayers [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ];
	const auto &move_layer = animlayers [ ANIMATION_LAYER_MOVEMENT_MOVE ];
	const auto &land_or_climb_layer = animlayers [ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ];
	const auto &alive_loop_layer = animlayers [ ANIMATION_LAYER_ALIVELOOP ];

	const bool on_ground = ( player->m_fFlags ( ) & FL_ONGROUND ) != 0;
	vec3_t &velocity = player->m_vecVelocity ( );

	/* standing still on ground */
	if ( on_ground && move_layer.m_weight == 0.0f ) {
		//csgo::csgo.engine_client->client_cmd_unrestricted ( fmt::format ( "echo pbr: {:.4f}\n", move_layer.playback_rate * 100.0f ).c_str ( ) );
		velocity.zero ( );
		return DETAIL_ZERO;
	}

	const float dt = std::max ( g_csgo.m_globals->m_interval, player->m_flSimulationTime ( ) - previous->m_sim_time );
	vec3_t avg_velocity = ( player->m_vecOrigin ( ) - previous->m_origin ) / dt;

	/* if we just started moving, we can probably use this velocity (they probably havent had time to switch directions) */
	/* not choking. an average will be pretty close */
	if ( previous->m_velocity.length_2d ( ) <= 0.1f || dt <= g_csgo.m_globals->m_interval ) {
		velocity = avg_velocity;
		return DETAIL_PERFECT;
	}

	const auto *weapon_info = player->GetActiveWeapon ( ) ? player->GetActiveWeapon ( )->GetWpnData ( ) : nullptr;
	float max_weapon_speed = weapon_info ? ( player->m_bIsScoped ( ) ? weapon_info->m_max_player_speed : weapon_info->m_max_player_speed_alt ) : 250.0f;

	if ( on_ground ) {
		avg_velocity.z = 0.0f;
		velocity = avg_velocity;

		//if ( track.size ( ) >= 2
		//	&& previous.simulation_time - track [ track.size ( ) - 2 ].simulation_time >= csgo::csgo.globals->interval_per_tick
		//	&& avg_velocity.length_2d ( ) > 0.0f ) {
		//	const auto last_avg_vel = ( previous.origin - track [ track.size ( ) - 2 ].origin ) / ( previous.simulation_time - track [ track.size ( ) - 2 ].simulation_time );
		//
		//	if ( last_avg_vel.length_2d ( ) > 0.0f ) {
		//		const float deg_1 = math::rad2deg ( atan2 ( avg_velocity.y, avg_velocity.x ) );
		//		const float deg_2 = math::rad2deg ( atan2 ( last_avg_vel.y, last_avg_vel.x ) );
		//
		//		const float deg_delta = math::normalize ( deg_1 - deg_2 );
		//		const float deg_lerp = math::normalize ( deg_1 + deg_delta * 0.5f );
		//		const float rad_dir = math::deg2rad ( deg_lerp );
		//
		//		float sin_dir, cos_dir;
		//		math::sin_cos ( rad_dir, sin_dir, cos_dir );
		//
		//		const float vel_len = avg_velocity.length_2d ( );
		//
		//		avg_velocity.x = cos_dir * vel_len;
		//		avg_velocity.y = sin_dir * vel_len;
		//
		//		velocity = avg_velocity;
		//	}
		//}

		VelocityDetail_t detail = DETAIL_NONE;

		/* SKEET VELOCITY FIX */
		if ( move_layer.m_playback_rate > 0.0f ) {
			vec3_t direction = velocity.normalized ( );
			direction.z = 0.0f;

			const float avg_speed_xy = velocity.length_2d ( );
			const float move_weight_with_air_smooth = move_layer.m_weight;
			const float target_move_weight_to_speed_xy = max_weapon_speed * math::Lerp ( CS_PLAYER_SPEED_WALK_MODIFIER, CS_PLAYER_SPEED_DUCK_MODIFIER, player->m_flDuckAmount ( ) ) * move_weight_with_air_smooth;
			const float speed_as_portion_of_run_top_speed = 0.35f * ( 1.0f - alive_loop_layer.m_weight );

			if ( alive_loop_layer.m_weight > 0.0f && alive_loop_layer.m_weight < 1.0f ) {
				const float speed_xy = max_weapon_speed * ( speed_as_portion_of_run_top_speed + 0.55f );
				velocity = direction * speed_xy;
				detail = DETAIL_RUNNING;
			}
			else if ( move_weight_with_air_smooth < 0.95f || target_move_weight_to_speed_xy > avg_speed_xy ) {
				velocity = direction * target_move_weight_to_speed_xy;

				const float prev_move_weight = previous->m_layers [ ANIMATION_LAYER_MOVEMENT_MOVE ].m_weight;
				const float weight_delta_rate = ( move_layer.m_weight - prev_move_weight ) / dt;
				const bool walking_speed = velocity.length_2d ( ) > max_weapon_speed * CS_PLAYER_SPEED_WALK_MODIFIER;
				const bool constant_speed = abs ( weight_delta_rate ) < ( walking_speed ? 0.9f : 0.15f );
				const bool accelerating = weight_delta_rate > 1.0f;

				///* move weight hasn't changed. we are confident there speed is correct/constant */
				//if ( constant_speed )
				//	detail = lagcomp::VelocityDetail::Constant;
				///* player is accelerating */
				//else if ( accelerating )
				//	detail = lagcomp::VelocityDetail::Accelerating;

				/* move weight hasn't changed. we are confident there speed is correct/constant */
				if ( move_layer.m_weight == prev_move_weight )
					detail = DETAIL_CONSTANT;
				/* player is accelerating */
				else if ( move_layer.m_weight > prev_move_weight )
					detail = DETAIL_ACCELERATING;
			}
			else {
				//float target_move_weight_adjusted_speed_xy = std::min ( max_weapon_speed, deployable_limit_max_speed(player) );
				float target_move_weight_adjusted_speed_xy = max_weapon_speed * move_weight_with_air_smooth;

				if ( ( player->m_fFlags ( ) & FL_DUCKING ) != 0 )
					target_move_weight_adjusted_speed_xy *= CS_PLAYER_SPEED_DUCK_MODIFIER;
				else if ( player->m_bIsWalking ( ) )
					target_move_weight_adjusted_speed_xy *= CS_PLAYER_SPEED_WALK_MODIFIER;

				const float prev_move_weight = previous->m_layers [ ANIMATION_LAYER_MOVEMENT_MOVE ].m_weight;

				if ( avg_speed_xy > target_move_weight_adjusted_speed_xy ) {
					velocity = direction * target_move_weight_adjusted_speed_xy;

					const float weight_delta_rate = ( move_layer.m_weight - prev_move_weight ) / dt;
					const bool walking_speed = velocity.length_2d ( ) > max_weapon_speed * CS_PLAYER_SPEED_WALK_MODIFIER;
					const bool constant_speed = abs ( weight_delta_rate ) < ( walking_speed ? 0.9f : 0.15f );
					const bool accelerating = weight_delta_rate > 1.0f;

					///* move weight hasn't changed. we are confident there speed is correct/constant */
					//if ( constant_speed )
					//	detail = lagcomp::VelocityDetail::Constant;
					///* player is accelerating */
					//else if ( accelerating )
					//	detail = lagcomp::VelocityDetail::Accelerating;
				}

				/* move weight hasn't changed. we are confident there speed is correct/constant */
				if ( move_layer.m_weight == prev_move_weight )
					detail = DETAIL_CONSTANT;
				/* player is accelerating */
				else if ( move_layer.m_weight > prev_move_weight )
					detail = DETAIL_ACCELERATING;
			}
		}

		//csgo::csgo.engine_client->client_cmd_unrestricted ( fmt::format ( "echo [lotus] {}, {}\n", velocity.length_2d ( ), move_layer.weight ).c_str ( ) );

		///* ONETAP VELOCITY FIX */
		//if ( alive_loop_layer.weight > 0.0f && alive_loop_layer.weight < 1.0f ) {
		//	const float v186 = ( 1.0f - alive_loop_layer.weight ) * 0.35f;
		//
		//	if ( v186 > 0.0f && v186 < 1.0f ) {
		//		const float v187 = ( v186 + 0.55f ) * max_weapon_speed;
		//		velocity = velocity.normalized ( ) * v187;
		//		detail = lagcomp::VelocityDetail::Running;
		//	}
		//}
		//else {
		//	const float move_weight_air_smooth = move_layer.weight / std::max ( 1.0f - land_or_climb_layer.weight, 0.55f );
		//	float speed = max_weapon_speed * move_weight_air_smooth;
		//	const float speed_scaled = speed * math::lerp ( CS_PLAYER_SPEED_WALK_MODIFIER, CS_PLAYER_SPEED_DUCK_MODIFIER, player->duck_amount ( ) );
		//	const float estimated_speed = avg_velocity.length_2d ( );
		//
		//	if ( move_layer.weight < 0.95f || speed_scaled > estimated_speed ) {
		//		if ( move_layer.weight < 0.1f )
		//			speed = speed_scaled;
		//		else
		//			speed = estimated_speed;
		//	}
		//	else if ( move_layer.weight != 0.0f ) {
		//		float want_speed = speed;
		//
		//		if ( ( player->flags ( ) & Flags::Ducking ) != 0 )
		//			want_speed *= CS_PLAYER_SPEED_DUCK_MODIFIER;
		//		else if ( player->is_walking ( ) )
		//			want_speed *= CS_PLAYER_SPEED_WALK_MODIFIER;
		//
		//		if ( estimated_speed > want_speed )
		//			speed = want_speed;
		//		else
		//			speed = estimated_speed;
		//	}
		//
		//	if ( move_layer.weight > 0.0f && move_layer.weight <= 1.0f ) {
		//		const float prev_move_weight = previous.animlayers [ lagcomp::DesyncSide::Middle ][ AnimstateLayer::Move ].weight;
		//		const float weight_delta_rate = ( move_layer.weight - prev_move_weight ) / dt;
		//
		//		velocity = velocity.normalized ( ) * speed;
		//
		//		const bool walking_speed = velocity.length_2d ( ) > max_weapon_speed * CS_PLAYER_SPEED_WALK_MODIFIER;
		//		const bool constant_speed = abs( weight_delta_rate ) < ( walking_speed ? 0.9f : 0.15f );
		//		const bool accelerating = weight_delta_rate > 1.0f;
		//
		//		/* move weight hasn't changed. we are confident there speed is correct/constant */
		//		if ( move_layer.weight == prev_move_weight )
		//			detail = lagcomp::VelocityDetail::Constant;
		//		/* player is accelerating */
		//		else if ( move_layer.weight > prev_move_weight )
		//			detail = lagcomp::VelocityDetail::Accelerating;
		//
		//		///* move weight hasn't changed. we are confident there speed is correct/constant */
		//		//if ( constant_speed )
		//		//	detail = lagcomp::VelocityDetail::Constant;
		//		///* player is accelerating */
		//		//else if ( accelerating )
		//		//	detail = lagcomp::VelocityDetail::Accelerating;
		//
		//		//csgo::csgo.engine_client->client_cmd_unrestricted ( fmt::format ( "echo [lotus] {}, {}\n", move_layer.weight, speed ).c_str ( ) );
		//	}
		//}

		return detail;
	}
	else {
		int seq = -1;

		const bool crouch = player->m_flDuckAmount ( ) > 0.0f;
		const float speed_as_portion_of_walk_top_speed = avg_velocity.length_2d ( ) / ( max_weapon_speed * CS_PLAYER_SPEED_WALK_MODIFIER );
		const bool moving = speed_as_portion_of_walk_top_speed > 0.25f;

		seq = moving + 17;
		if ( !crouch )
			seq = moving + 15;

		velocity = avg_velocity;

		if ( jump_or_fall_layer.m_weight > 0.0f
			&& jump_or_fall_layer.m_playback_rate > 0.0f
			&& jump_or_fall_layer.m_sequence == seq ) {
			const float time_since_jump = jump_or_fall_layer.m_cycle * jump_or_fall_layer.m_playback_rate;
			velocity.z = g_csgo.sv_jump_impulse->GetFloat ( ) - time_since_jump * g_csgo.sv_gravity->GetFloat ( ) * 0.5f;
		}

		return DETAIL_PERFECT;
	}

	player->m_vecVelocity ( ) = velocity;

	return DETAIL_NONE;
}

float accelerate_rebuilt ( CUserCmd *cmd, Player *local, vec3_t wish_dir, const vec3_t &wish_speed, bool &ducking ) {
	//static auto sv_accelerate_use_weapon_speed = g_csgo.m_cvar->FindVar ( HASH ( "sv_accelerate_use_weapon_speed" ) );
	//static auto sv_accelerate = g_csgo.m_cvar->FindVar ( HASH ( "sv_accelerate" ) );

	//float duck_amount; // xmm3_4
	//int flags_check; // edx
	//char in_duck; // al
	//char is_ducking; // al
	//float duck_speed; // xmm0_4
	//int duck_speed_ptr; // eax
	//float next_duck; // xmm2_4
	//float backup_next_duck; // xmm3_4
	//char new_flags; // al
	//char new_is_ducking; // cl
	//Weapon* v19; // ecx
	//Weapon *weapon; // eax
	////double ( __thiscall * GetMaxSpeed )( _DWORD * ); // eax
	//float speed2d; // [esp-24h] [ebp-30h]
	//float v26; // [esp-24h] [ebp-30h]
	////int ( __thiscall * GetZoomLevels )( _DWORD * ); // [esp-20h] [ebp-2Ch]
	////int ( __thiscall * GetWeaponId )( _DWORD * ); // [esp-1Ch] [ebp-28h]
	//CUserCmd *cmd_1; // [esp-14h] [ebp-20h]
	//char is_ducking_1; // [esp-14h] [ebp-20h]
	//Weapon *v31; // [esp-14h] [ebp-20h]
	//Weapon *v32; // [esp-10h] [ebp-1Ch]
	//bool v33; // [esp-1h] [ebp-Dh]
	//int v34; // [esp+8h] [ebp-4h] BYREF

	//cmd_1 = cmd;
	//speed2d = ( float ) ( ( float ) ( wish_dir.y * wish_speed.y ) + ( float ) ( wish_dir.x * wish_speed.x ) ) + ( float ) ( wish_dir.z * wish_speed [ 2 ] );
	//duck_amount = local->m_flDuckAmount ( );
	//flags_check = local->m_fFlags ( );
	//in_duck = 1;
	//if ( ( cmd_1->m_buttons & IN_DUCK ) == 0 ) {
	//	is_ducking = local->m_bDucking ( );
	//	if ( duck_amount > 0.0 )
	//		is_ducking = 1;
	//	is_ducking_1 = is_ducking;
	//	if ( is_ducking || duck_amount >= 1.0 ) {
	//		duck_speed = 2.0;
	//		duck_speed_ptr = local->m_flDuckSpeed ( );
	//		if ( duck_speed_ptr >= 1.5f )
	//			duck_speed = duck_speed_ptr;
	//		next_duck = std::fmaxf ( 0.0f, duck_amount - ( float ) ( duck_speed * game::TICKS_TO_TIME ( 1 ) ) );
	//		backup_next_duck = next_duck;
	//		if ( next_duck <= 0.0 )
	//			next_duck = 0.0;
	//		new_flags = flags_check & ~2;
	//		if ( backup_next_duck > 0.0 )
	//			new_flags = local->m_fFlags ( );
	//		flags_check = new_flags & ~2;
	//		if ( next_duck > 0.75 )
	//			flags_check = new_flags;
	//		new_is_ducking = 0;
	//		if ( backup_next_duck > 0.0 )
	//			new_is_ducking = is_ducking_1;
	//		if ( new_is_ducking )
	//			goto LABEL_19;
	//	}
	//	if ( ( flags_check & 2 ) != 0 )
	//		LABEL_19:
	//	in_duck = 1;
	//	else
	//		in_duck = 0;
	//}
	//ducking = in_duck;
	//v33 = ( cmd->m_buttons & IN_SPEED ) != 0 && !in_duck;
	//if ( sv_accelerate_use_weapon_speed->GetInt ( ) ) {
	//	v19 = local->GetActiveWeapon ( );
	//	weapon = v19;
	//	v32 = weapon;
	//	v31 = weapon;
	//	if ( weapon ) {
	//		// GetMaxSpeed (441)
	//		auto GetMaxSpeed = local->m_bIsScoped ( ) ? weapon->GetWpnData ( )->m_max_player_speed_alt : weapon->GetWpnData ( )->m_max_player_speed;
	//		// GetZoomLevels (461)
	//
	//		if ( weapon->m_zoomLevel ( ) > 0 ) {
	//			// GetWeaponId (442)
	//			if ( ( !ducking && !is_walking ) || ( ( is_walking || ducking ) && slowed_by_scope ) )
	//				acceleration_scale *= std::min ( 1.0f, ( max_speed / max_speed ) );
	//		}
	//	}
	//}
	//auto accel = g_csgo.sv_airaccelerate->GetFloat ( );

	//if ( local->m_bIsWalking ( ) && wish_speed > ( goal_speed - 5.0f ) )
	//	accel *= std::clamp ( 1.0f - ( std::max ( 0.0f, wanted_speed - ( goal_speed - 5.0f ) ) / std::max ( 0.0f, goal_speed - ( goal_speed - 5.0f ) ) ), 0.0f, 1.0f );
	//
	return 0.f;
}

void Aimbot::Stop ( CUserCmd *cmd, float target_speed, vec3_t &cur_velocity, vec3_t a8, bool compensate_for_duck ) {
	auto weapon = g_cl.m_weapon;

	if ( !weapon )
		return;

	static auto sv_stopspeed = g_csgo.m_cvar->FindVar ( HASH ( "sv_stopspeed" ) );
	static auto sv_friction = g_csgo.m_cvar->FindVar ( HASH ( "sv_friction" ) );

	//v46 = a3;
	//v47 = retaddr;
	auto vel = cur_velocity;
	auto not_in_jump = !( cmd->m_buttons & IN_JUMP );
	auto delta_target_speed = target_speed;
	auto max_speed = g_cl.m_local->m_bIsScoped ( ) ? weapon->GetWpnData ( )->m_max_player_speed_alt : weapon->GetWpnData ( )->m_max_player_speed;
	// = a2;

	if ( not_in_jump
		&& std::sqrtf ( ( ( cmd->m_forward_move * cmd->m_forward_move ) + ( cmd->m_side_move * cmd->m_side_move ) ) + ( cmd->m_up_move * cmd->m_up_move ) ) >= 38.0f ) {
		//*&speed2d_sqr = LODWORD ( vel.x );
		auto speed2d_sqr = vel.length_2d_sqr ( );
		//	*&speed2d1 = *&speed2d_sqr;
		auto speed2d1 = std::sqrtf ( speed2d_sqr );
		auto speed2d = speed2d1;
		//	*&speed2d = *&speed2d1;

		if ( speed2d1 >= 28.0 ) {
			auto stop_speed = sv_stopspeed->GetFloat ( );
			stop_speed = std::max < float > ( stop_speed, speed2d );

			auto friction = sv_friction->GetFloat ( );
			auto tick_interval = game::TICKS_TO_TIME ( 1 );
			stop_speed = friction * stop_speed;
			auto max_stop_speed = std::max < float > ( speed2d - ( tick_interval * stop_speed ), 0.0f );
			auto delta_target_speed_1 = delta_target_speed - max_stop_speed;
			const auto max_stop_speed_vec = vec3_t { vel.x * ( max_stop_speed / speed2d ), vel.y * ( max_stop_speed / speed2d ),  ( max_stop_speed / speed2d ) * 0.0f };

			delta_target_speed = delta_target_speed_1;

			if ( fabs ( delta_target_speed_1 ) >= 0.5 ) {
				vec3_t move_dir;

				if ( delta_target_speed_1 >= 0.0 ) {
					cmd->m_buttons &= ~IN_SPEED;

					vec3_t fwd;
					math::AngleVectors ( cmd->m_view_angles, &fwd, &vel );
					const auto right = fwd.cross ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

					move_dir.y = ( vel.y - ( right.y / right.x ) * vel.x ) / ( fwd.y - ( right.y / right.x ) * fwd.x );
					move_dir.x = ( vel.x - fwd.x * cmd->m_forward_move ) / right.x;
					move_dir.z = 0.f;
				}

				//move_dir.z = 0.0;
				//auto inv_vel = 1.0 / ( std::sqrtf ( ( -vel.x * -vel.x ) + ( -vel.y * -vel.y ) ) + 0.00000011920929f );
				//auto vel_norm_y = inv_vel * -vel.y;
				//auto vel_norm_x = inv_vel * -vel.x;
				//move_dir.x = vel_norm_x;
				//move_dir.y = vel_norm_y;

				//if ( vel_norm_y != 0.0 || vel_norm_x != 0.0 )
				//	atan2f ( );

				a8.y = 0.0f;
				a8.x = 90.0f;
				a8.z = 0.0f;

				cmd->m_side_move = 0.0;
				cmd->m_forward_move = 450.0;
				delta_target_speed = -delta_target_speed_1;
				//compensate_for_duck = 0;
				stop_speed = -4917.7676;
				auto speed = max_speed;
				//cmd_2 = cmd_1;
				auto v40 = util::get_method < float ( __thiscall * )( Player * ) > ( g_cl.m_local, 269 ) ( g_cl.m_local );
				max_speed = g_cl.m_local->m_flMaxspeed ( ) + max_speed;
				auto v25 = max_speed;
				auto acceleration_amount = v25;
				auto speed_dot_dir = ( ( max_stop_speed_vec.y * move_dir.y ) + ( max_stop_speed_vec.x * move_dir.x ) )
					+ ( max_stop_speed_vec.z * move_dir.z );
				auto addspeed = std::min < float > ( max_speed, v40 ) - speed_dot_dir;
				float zero = 0.0f;
				if ( addspeed > 0.0 )
					zero = fminf ( addspeed, acceleration_amount );
				if ( zero > ( delta_target_speed + 0.5 ) ) {
					auto move_length = speed_dot_dir + delta_target_speed;

					if ( g_cl.m_local->m_flDuckAmount ( ) + speed == 1.0f )
						move_length = move_length / 0.34f;

					auto move_scale = move_length
						/ ( std::sqrtf (
							( ( cmd->m_forward_move * cmd->m_forward_move ) + ( cmd->m_side_move * cmd->m_side_move ) )
							+ ( cmd->m_up_move * cmd->m_up_move ) )
							+ 0.00000011920929f );
					cmd->m_forward_move = cmd->m_forward_move * move_scale;
					cmd->m_side_move = cmd->m_side_move * move_scale;
					cmd->m_up_move = cmd->m_up_move * move_scale;
				}
			}
			else {
				cmd->m_side_move = 0.0f;
				cmd->m_up_move = 0.0f;
			}
		}
	}
}

void AimPlayer::UpdateAnimations ( LagRecord *record ) {
	CCSGOPlayerAnimState *state = m_player->m_PlayerAnimState ( );
	if ( !state )
		return;

	auto game_state = ( CCSGOGamePlayerAnimState * ) state;

	// player respawned.
	if ( m_player->m_flSpawnTime ( ) != m_spawn ) {
		// reset animation state.
		game::ResetAnimationState ( state );

		// note new spawn time.
		m_spawn = m_player->m_flSpawnTime ( );
	}

	float curtime = g_csgo.m_globals->m_curtime;
	float frametime = g_csgo.m_globals->m_frametime;

	g_csgo.m_globals->m_curtime = m_player->m_flSimulationTime ( );
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	AnimationBackup_t backup;

	backup.m_origin = m_player->m_vecOrigin ( );
	backup.m_abs_origin = m_player->GetAbsOrigin ( );
	backup.m_velocity = m_player->m_vecVelocity ( );
	backup.m_abs_velocity = m_player->m_vecAbsVelocity ( );
	backup.m_flags = m_player->m_fFlags ( );
	backup.m_eflags = m_player->m_iEFlags ( );
	backup.m_duck = m_player->m_flDuckAmount ( );
	backup.m_body = m_player->m_flLowerBodyYawTarget ( );
	backup.m_mins = m_player->m_vecMins ( );
	backup.m_maxs = m_player->m_vecMaxs ( );

	m_player->GetAnimLayers ( backup.m_layers );
	
	g_csgo.m_globals->m_curtime = curtime;
	g_csgo.m_globals->m_frametime = frametime;

	bool bot = game::IsFakePlayer ( m_player->index ( ) );

	record->m_fake_walk = false;
	record->m_mode = Resolver::Modes::RESOLVE_NONE;

	if ( record->m_lag > 0 && record->m_lag < 16 && m_records.size ( ) >= 2 ) {
		LagRecord *previous = m_records [ 1 ].get ( );

		if ( previous && !previous->dormant ( ) ) {
			//record->m_velocity = ( record->m_origin - previous->m_origin ) * ( 1.f / game::TICKS_TO_TIME ( record->m_lag ) );
			record->m_velocity_detail = FixVelocity ( record->m_layers, previous, m_player );
		}
	}

	record->m_anim_velocity = record->m_velocity;

	if ( record->m_anim_velocity.length_2d ( ) <= 1.0f ) {
		if ( !game_state->m_flDurationMoving && !game_state->m_vecPositionLast.z ) {
			float lastUpdateIncrement = game_state->m_flLastUpdateIncrement;
			if ( lastUpdateIncrement > 0.0 ) {
				float currentFeetYaw = game_state->m_flFootYawLast;
				float goalFeetYaw = game_state->m_flFootYaw;
				float feetDelta = currentFeetYaw - game_state->m_flFootYaw;
				if ( goalFeetYaw < currentFeetYaw ) {
					if ( feetDelta >= 180.0 )
						feetDelta = feetDelta - 360.0;
				}
				else if ( feetDelta <= -180.0 ) {
					feetDelta = feetDelta + 360.0;
				}
				if ( ( feetDelta / lastUpdateIncrement ) > 120.0 ) {
					m_player->m_AnimOverlay ( ) [ ANIMATION_LAYER_ADJUST ].m_cycle = 0.0;
					m_player->m_AnimOverlay ( ) [ ANIMATION_LAYER_ADJUST ].m_weight = 0.0;
					m_player->m_AnimOverlay ( ) [ ANIMATION_LAYER_ADJUST ].m_sequence = m_player->GetSequenceActivity ( 979 );
				}
			}
		}
	}

	if ( record->m_lag > 1 && !bot ) {
		float speed = m_player->m_vecVelocity ( ).length_2d ( );

		if ( record->m_flags & FL_ONGROUND && record->m_layers [ 6 ].m_weight == 0.f && ( speed > 0.1f && speed < 100.f ) || record->m_velocity_detail == DETAIL_PERFECT )
			record->m_fake_walk = true;

		if ( record->m_fake_walk )
			record->m_anim_velocity = record->m_velocity = { 0.f, 0.f, 0.f };

		if ( m_records.size ( ) >= 2 ) {
			LagRecord *previous = m_records [ 1 ].get ( );

			if ( previous && !previous->dormant ( ) ) {
				float time = record->m_sim_time - previous->m_sim_time;

				const auto crouch_amount = m_player->m_flDuckAmount ( );
				const auto crouch_speed = std::max ( 1.5f, m_player->m_flDuckSpeed ( ) );

				if ( !record->m_fake_walk ) {
					vec3_t velo = record->m_velocity - previous->m_velocity;

					vec3_t accel = ( velo / time ) * g_csgo.m_globals->m_interval;

					record->m_anim_velocity = previous->m_velocity + accel;
				}
			}
		}
	}

	m_player->m_vecOrigin ( ) = record->m_origin;
	m_player->m_vecVelocity ( ) = m_player->m_vecAbsVelocity ( ) = record->m_anim_velocity;
	m_player->m_iEFlags ( ) &= ~0x1000;
	m_player->m_angEyeAngles ( ) = record->m_eye_angles;
	m_player->m_flLowerBodyYawTarget ( ) = record->m_body;
	game_state->m_flLastUpdateTime = m_player->m_flSimulationTime ( );

	UpdatePlayer ( backup, record );

	//m_player->m_flSimulationTime ( ) = backup_simtime;
	

	m_player->m_flDuckAmount ( ) = backup.m_duck;

	g_csgo.m_globals->m_curtime = curtime;
	g_csgo.m_globals->m_frametime = frametime;
}

void AimPlayer::UpdatePlayer ( AnimationBackup_t backup, LagRecord *record ) {
	CCSGOPlayerAnimState *state = m_player->m_PlayerAnimState ( );
	if ( !state )
		return;

	const auto backup_curtime = g_csgo.m_globals->m_curtime;
	const auto backup_frametime = g_csgo.m_globals->m_frametime;

	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_curtime = record->m_sim_time;
	//g_csgo.m_globals->m_abs_frametime = g_csgo.m_globals->m_interval;
	//g_csgo.m_globals->m_frame = sim_ticks;
	//g_csgo.m_globals->m_tick_count = sim_ticks;
	//g_csgo.m_globals->m_interp_amt = 0.0;

	backup.m_origin = m_player->m_vecOrigin ( );
	backup.m_abs_origin = m_player->GetAbsOrigin ( );
	backup.m_velocity = m_player->m_vecVelocity ( );
	backup.m_abs_velocity = m_player->m_vecAbsVelocity ( );
	backup.m_flags = m_player->m_fFlags ( );
	backup.m_eflags = m_player->m_iEFlags ( );
	backup.m_duck = m_player->m_flDuckAmount ( );
	backup.m_mins = m_player->m_vecMins ( );
	backup.m_maxs = m_player->m_vecMaxs ( );
	backup.m_body = m_player->m_flLowerBodyYawTarget ( );

	auto game_state = ( CCSGOGamePlayerAnimState * ) state;
	bool bot = game::IsFakePlayer ( m_player->index ( ) );
	bool fake = !bot && g_menu.main.aimbot.correct.get ( );

	if ( fake )
		g_resolver.ResolveAngles ( m_player, record );

	m_player->m_bClientSideAnimation ( ) = g_cl.m_update_ent = true;

	if ( state->m_frame == g_csgo.m_globals->m_frame )
		state->m_frame -= 1;

	rebuilt::UpdateAnimationState ( ( CCSGOGamePlayerAnimState* )state, record->m_eye_angles.y, record->m_eye_angles.x, false );
	m_player->m_bClientSideAnimation ( ) = g_cl.m_update_ent = false;

	m_player->GetPoseParameters ( record->m_poses );

	if ( fake )
		g_resolver.ResolvePoses ( m_player, record );

	record->m_abs_ang = m_player->GetAbsAngles ( );

	m_player->m_vecOrigin ( ) = backup.m_origin;
	m_player->m_vecVelocity ( ) = backup.m_velocity;
	m_player->m_vecAbsVelocity ( ) = backup.m_abs_velocity;
	m_player->m_fFlags ( ) = backup.m_flags;
	m_player->m_vecMins ( ) = backup.m_mins;
	m_player->m_vecMaxs ( ) = backup.m_maxs;
	m_player->m_iEFlags ( ) = backup.m_eflags;
	m_player->m_flLowerBodyYawTarget ( ) = backup.m_body;
	m_player->SetAbsOrigin ( backup.m_origin );
	m_player->SetAnimLayers ( backup.m_layers );

//	g_csgo.m_globals->m_realtime = dword_44732EA4;
	g_csgo.m_globals->m_curtime = backup_curtime;
	g_csgo.m_globals->m_frametime = backup_frametime;
	//g_csgo.m_globals->m_abs_frametime = dword_44732EB0;
	//g_csgo.m_globals->m_interp_amt = dword_44732EB4;
	//g_csgo.m_globals->m_frame = dword_44732EB8;
	//g_csgo.m_globals->m_tick_count = dword_44732EBC;
}

void AimPlayer::OnNetUpdate ( Player *player ) {
	bool reset = ( !g_menu.main.aimbot.enable.get ( ) || !player->enemy ( g_cl.m_local ) || player->m_lifeState ( ) == LIFE_DEAD );
	bool disable = !reset;

	if ( reset ) {
		player->m_bClientSideAnimation ( ) = true;
		m_records.clear ( );
		return;
	}

	if ( m_player != player )
		m_records.clear ( );

	m_player = player;

	if ( player->dormant ( ) ) {
		bool insert = true;

		if ( !m_records.empty ( ) ) {
			LagRecord *front = m_records.front ( ).get ( );

			if ( front->dormant ( ) )
				insert = false;
		}

		if ( insert ) {
			m_records.emplace_front ( std::make_shared< LagRecord > ( player ) );

			LagRecord *current = m_records.front ( ).get ( );

			current->m_dormant = true;
		}
	}

	bool update = ( m_records.empty ( ) || player->m_flSimulationTime ( ) > m_records.front ( ).get ( )->m_sim_time );

	if ( m_records.size ( ) >= 2 ) {
		LagRecord *previous = m_records [ 1 ].get ( );

		if ( previous ) {
			const float curtime = player->m_flSimulationTime ( );
			const int tick = game::TIME_TO_TICKS ( curtime );
			const int tick_count = static_cast< int >( curtime / g_csgo.m_globals->m_interval + 0.5f );
			const int ticks_ago_hit_ground = static_cast< int >( ( player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_cycle * player->m_AnimOverlay ( ) [ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ].m_playback_rate ) / g_csgo.m_globals->m_interval + 0.5f );
			const int tick_hit_ground = tick_count - ticks_ago_hit_ground;
			const bool twice_in_air = !( previous->m_flags & FL_ONGROUND ) && !( m_records.front ( ).get ( )->m_flags & FL_ONGROUND );
			const bool jumped = ( previous->m_flags & FL_ONGROUND ) && !( m_records.front ( ).get ( )->m_flags & FL_ONGROUND );
			const bool landed = !( previous->m_flags & FL_ONGROUND ) && ( m_records.front ( ).get ( )->m_flags & FL_ONGROUND );
			const bool on_ground = ( previous->m_flags & FL_ONGROUND ) && ( m_records.front ( ).get ( )->m_flags & FL_ONGROUND );

			int flags_int = 0;

			if ( ( landed && tick >= tick_hit_ground )
				|| ( jumped && tick <= tick_hit_ground )
				|| ( twice_in_air && tick == tick_hit_ground )
				|| on_ground ) {
				player->m_fFlags ( ) |= FL_ONGROUND;

				/* jumped, set velocity to upwards dir  */
				if ( ( jumped && flags_int == tick ) || twice_in_air ) {
					player->m_vecVelocity ( ).z = g_csgo.sv_jump_impulse->GetFloat ( );
					g_cl.print ( XOR ( "player hit ground this tick! [%d]\n" ), tick );
				}
			}
		}
	}

	if ( update ) {
		m_records.emplace_front ( std::make_shared< LagRecord > ( player ) );

		LagRecord *current = m_records.front ( ).get ( );
		current->m_dormant = false;

		UpdateAnimations ( current );

		BoneArray m [ 128 ];
		g_bones.Build ( m_player, current, m, g_csgo.m_globals->m_curtime );
		memcpy ( current->m_bones, m, sizeof ( BoneArray ) * 128 );

		current->m_shifting = false;
	}

	//// ghetto fix
	//if ( shift && !player->dormant ( ) ) {
	//	LagRecord *current = m_records.front ( ).get ( );

	//	if ( current ) {
	//		if ( current->m_lag >= 1 && !player->dormant ( ) )
	//			current->m_shifting = ( player->m_flSimulationTime ( ) <= player->m_flOldSimulationTime ( ) );
	//	}
	//}

	auto tickrate = 1.0f / g_csgo.m_globals->m_interval;

	while ( m_records.size ( ) > tickrate )
		m_records.pop_back ( );
}

void AimPlayer::OnRoundStart ( Player *player ) {
	m_player = player;
	m_walk_record = LagRecord { };
	m_shots = 0;
	m_missed_shots = 0;

	m_stand_index = 0;
	m_stand_index2 = 0;
	m_body_index = 0;

	m_records.clear ( );
	m_hitboxes.clear ( );
}

void AimPlayer::SetupHitboxes ( LagRecord *record, bool history ) {
	m_hitboxes.clear ( );

	if ( g_cl.m_weapon_id == ZEUS ) {
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );
		return;
	}

	if ( g_menu.main.aimbot.baim1.get ( 0 ) )
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );

	if ( g_menu.main.aimbot.baim1.get ( 1 ) )
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::LETHAL } );

	if ( g_menu.main.aimbot.baim1.get ( 2 ) )
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::LETHAL2 } );

	if ( g_menu.main.aimbot.baim1.get ( 3 ) && record->m_mode != Resolver::Modes::RESOLVE_NONE && record->m_mode != Resolver::Modes::RESOLVE_WALK )
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );

	if ( g_menu.main.aimbot.baim1.get ( 4 ) && !( record->m_pred_flags & FL_ONGROUND ) )
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );

	bool only { false };

	if ( g_menu.main.aimbot.baim2.get ( 0 ) ) {
		only = true;
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );
	}

	if ( g_menu.main.aimbot.baim2.get ( 1 ) && m_player->m_iHealth ( ) <= ( int ) g_menu.main.aimbot.baim_hp.get ( ) ) {
		only = true;
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );
	}

	if ( g_menu.main.aimbot.baim2.get ( 2 ) && record->m_mode != Resolver::Modes::RESOLVE_NONE && record->m_mode != Resolver::Modes::RESOLVE_WALK ) {
		only = true;
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );
	}

	if ( g_menu.main.aimbot.baim2.get ( 3 ) && !( record->m_pred_flags & FL_ONGROUND ) ) {
		only = true;
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );
	}

	if ( g_input.GetKeyState ( g_menu.main.aimbot.baim_key.get ( ) ) ) {
		only = true;
		m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::PREFER } );
	}

	if ( only )
		return;

	std::vector< size_t > hitbox { history ? g_menu.main.aimbot.hitbox_history.GetActiveIndices ( ) : g_menu.main.aimbot.hitbox.GetActiveIndices ( ) };
	if ( hitbox.empty ( ) )
		return;

	for ( const auto &h : hitbox ) {
		if ( h == 0 )
			m_hitboxes.push_back ( { HITBOX_HEAD, HitscanMode::NORMAL } );

		if ( h == 1 ) {
			m_hitboxes.push_back ( { HITBOX_THORAX, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_CHEST, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_UPPER_CHEST, HitscanMode::NORMAL } );
		}

		if ( h == 2 ) {
			m_hitboxes.push_back ( { HITBOX_PELVIS, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_BODY, HitscanMode::NORMAL } );
		}

		if ( h == 3 ) {
			m_hitboxes.push_back ( { HITBOX_L_UPPER_ARM, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_R_UPPER_ARM, HitscanMode::NORMAL } );
		}

		if ( h == 4 ) {
			m_hitboxes.push_back ( { HITBOX_L_THIGH, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_R_THIGH, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_L_CALF, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_R_CALF, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_L_FOOT, HitscanMode::NORMAL } );
			m_hitboxes.push_back ( { HITBOX_R_FOOT, HitscanMode::NORMAL } );
		}
	}
}

void Aimbot::init ( ) {
	m_targets.clear ( );

	m_target = nullptr;
	m_aim = vec3_t { };
	m_angle = ang_t { };
	m_damage = 0.f;
	m_record = nullptr;
	m_stop = false;

	m_best_dist = std::numeric_limits< float >::max ( );
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = std::numeric_limits< float >::max ( );
	m_best_height = std::numeric_limits< float >::max ( );
}

void Aimbot::StripAttack ( ) {
	if ( g_cl.m_weapon_id == REVOLVER )
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK2;

	else
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK;
}

void Aimbot::think ( ) {
	init ( );

	if ( !g_cl.m_weapon )
		return;

	if ( g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_C4 )
		return;

	if ( !g_cl.m_weapon_fire )
		StripAttack ( );

	if ( !g_menu.main.aimbot.enable.get ( ) )
		return;

	bool revolver = g_cl.m_weapon_id == REVOLVER && g_cl.m_revolver_cock != 0;

	if ( revolver && g_cl.m_revolver_cock > 0 && g_cl.m_revolver_cock == g_cl.m_revolver_query ) {
		*g_cl.m_packet = false;
		return;
	}

	if ( g_cl.m_weapon_fire && !g_cl.m_lag && !revolver ) {
		*g_cl.m_packet = false;
		StripAttack ( );
		return;
	}

	if ( !g_cl.m_weapon_fire )
		return;

	for ( int i { 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
		Player *player = g_csgo.m_entlist->GetClientEntity< Player * > ( i );

		if ( !IsValidTarget ( player ) )
			continue;

		AimPlayer *data = &m_players [ i - 1 ];
		if ( !data )
			continue;

		m_targets.emplace_back ( data );
	}

	if ( g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS ) {
		if ( g_menu.main.aimbot.knifebot.get ( ) )
			knife ( );

		return;
	}

	find ( );

	apply ( );
}

// ty cbrs
void Aimbot::Slow ( CUserCmd *ucmd ) {
	if ( !g_cl.m_local->alive ( ) || !( g_cl.m_local->m_fFlags ( ) & FL_ONGROUND ) || !g_cl.m_local->GetActiveWeapon ( ) || !g_cl.m_local->GetActiveWeapon ( )->GetWpnData ( ) )
		return;

	auto quick_stop = [ & ] ( ) {
		const auto target_vel = -g_cl.m_local->m_vecVelocity ( ).normalized ( ) * g_csgo.cl_forwardspeed->GetFloat ( );

		ang_t angles;
		g_csgo.m_engine->GetViewAngles ( angles );

		vec3_t fwd;
		math::AngleVectors ( angles, &fwd );
		const auto right = fwd.cross ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

		ucmd->m_forward_move = ( target_vel.y - ( right.y / right.x ) * target_vel.x ) / ( fwd.y - ( right.y / right.x ) * fwd.x );
		ucmd->m_side_move = ( target_vel.x - fwd.x * ucmd->m_forward_move ) / right.x;

		ucmd->m_forward_move = std::clamp <float> ( ucmd->m_forward_move, -g_csgo.cl_forwardspeed->GetFloat ( ), g_csgo.cl_forwardspeed->GetFloat ( ) );
		ucmd->m_side_move = std::clamp <float> ( ucmd->m_side_move, -g_csgo.cl_forwardspeed->GetFloat ( ), g_csgo.cl_sidespeed->GetFloat ( ) );

		ucmd->m_buttons &= ~IN_WALK;
		ucmd->m_buttons |= IN_SPEED;
	};

	const auto speed = g_cl.m_local->m_vecVelocity ( ).length_2d ( );

	if ( speed <= 4.0f )
		return;

	auto max_speed = 260.0f;
	const auto weapon = g_cl.m_local->GetActiveWeapon ( );

	if ( weapon && weapon->GetWpnData ( ) )
		max_speed = g_cl.m_local->m_bIsScoped ( ) ? weapon->GetWpnData ( )->m_max_player_speed_alt : weapon->GetWpnData ( )->m_max_player_speed;

	const auto pure_accurate_speed = max_speed * 0.34f;
	const auto accurate_speed = max_speed * 0.315f;

	//    actually slowwalk
	if ( speed <= pure_accurate_speed ) {
		const auto cmd_speed = sqrt ( ucmd->m_forward_move * ucmd->m_forward_move + ucmd->m_side_move * ucmd->m_side_move );
		const auto local_speed = std::max ( g_cl.m_local->m_vecVelocity ( ).length_2d ( ), 0.1f );
		const auto speed_multiplier = ( local_speed / cmd_speed ) * ( accurate_speed / local_speed );

		ucmd->m_forward_move = std::clamp <float> ( ucmd->m_forward_move * speed_multiplier, -g_csgo.cl_forwardspeed->GetFloat ( ), g_csgo.cl_forwardspeed->GetFloat ( ) );
		ucmd->m_side_move = std::clamp <float> ( ucmd->m_side_move * speed_multiplier, -g_csgo.cl_forwardspeed->GetFloat ( ), g_csgo.cl_sidespeed->GetFloat ( ) );

		ucmd->m_buttons &= ~IN_WALK;
		ucmd->m_buttons |= IN_SPEED;
	}
	//    we are fast
	else {
		quick_stop ( );
	}
}

void Aimbot::find ( ) {
	struct BestTarget_t { Player *player; vec3_t pos; float damage; LagRecord *record; };

	vec3_t       tmp_pos;
	float        tmp_damage;
	BestTarget_t best;
	best.player = nullptr;
	best.damage = -1.f;
	best.pos = vec3_t { };
	best.record = nullptr;

	if ( m_targets.empty ( ) )
		return;

	if ( g_cl.m_weapon_id == ZEUS && !g_menu.main.aimbot.zeusbot.get ( ) )
		return;

	for ( const auto &t : m_targets ) {
		if ( t->m_records.empty ( ) )
			continue;

		if ( g_lagcomp.StartPrediction ( t ) ) {
			LagRecord *front = t->m_records.front ( ).get ( );

			t->SetupHitboxes ( front, false );
			if ( t->m_hitboxes.empty ( ) )
				continue;

			if ( t->GetBestAimPosition ( tmp_pos, tmp_damage, front ) && SelectTarget ( front, tmp_pos, tmp_damage ) ) {
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = front;
			}
		}

		else {
			LagRecord *ideal = g_resolver.FindIdealRecord ( t );
			if ( !ideal )
				continue;

			t->SetupHitboxes ( ideal, false );
			if ( t->m_hitboxes.empty ( ) )
				continue;

			if ( t->GetBestAimPosition ( tmp_pos, tmp_damage, ideal ) && SelectTarget ( ideal, tmp_pos, tmp_damage ) ) {
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = ideal;
			}

			LagRecord *last = g_resolver.FindLastRecord ( t );
			if ( !last || last == ideal )
				continue;

			t->SetupHitboxes ( last, true );
			if ( t->m_hitboxes.empty ( ) )
				continue;

			if ( t->GetBestAimPosition ( tmp_pos, tmp_damage, last ) && SelectTarget ( last, tmp_pos, tmp_damage ) ) {
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = last;
			}
		}
	}

	m_stop = g_menu.main.movement.autostop.get ( ) && !( g_cl.m_buttons & IN_JUMP );

	vec3_t a8;

	if ( m_stop && ( best.damage && best.player ) )
		Slow ( g_cl.m_cmd );
	//Stop ( g_cl.m_cmd, g_cl.m_local->m_vecVelocity ( ).length_2d ( ), g_cl.m_local->m_vecVelocity ( ), a8, true );

	if ( best.player && best.record ) {
		math::VectorAngles ( best.pos - g_cl.m_shoot_pos, m_angle );

		m_target = best.player;
		m_aim = best.pos;
		m_damage = best.damage;
		m_record = best.record;
		m_record->cache ( );

		bool on = g_menu.main.aimbot.hitchance.get ( ) && g_menu.main.config.mode.get ( ) == 0;
		bool hit = on && CheckHitchance ( m_target, m_angle );

		bool can_scope = !g_cl.m_local->m_bIsScoped ( ) && ( g_cl.m_weapon_id == AUG || g_cl.m_weapon_id == SG553 || g_cl.m_weapon_type == WEAPONTYPE_SNIPER_RIFLE );

		if ( can_scope ) {
			if ( g_menu.main.aimbot.zoom.get ( ) == 1 ) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}

			else if ( g_menu.main.aimbot.zoom.get ( ) == 2 && on && !hit ) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}
		}

		if ( hit || !on ) {
			if ( g_menu.main.config.mode.get ( ) == 1 && g_cl.m_weapon_id == REVOLVER )
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;

			else
				g_cl.m_cmd->m_buttons |= IN_ATTACK;
		}
	}
}

bool Aimbot::CheckHitchance ( Player *player, const ang_t &angle ) {
	constexpr float HITCHANCE_MAX = 100.f;
	constexpr int   SEED_MAX = 255;

	vec3_t     start { g_cl.m_shoot_pos }, end, fwd, right, up, dir, wep_spread;
	float      inaccuracy, spread;
	CGameTrace tr;
	size_t     total_hits { }, needed_hits { ( size_t ) std::ceil ( ( g_menu.main.aimbot.hitchance_amount.get ( ) * SEED_MAX ) / HITCHANCE_MAX ) };

	math::AngleVectors ( angle, &fwd, &right, &up );

	inaccuracy = g_cl.m_weapon->GetInaccuracy ( );
	spread = g_cl.m_weapon->GetSpread ( );

	for ( int i { }; i <= SEED_MAX; ++i ) {
		wep_spread = g_cl.m_weapon->CalculateSpread ( i, inaccuracy, spread );

		dir = ( fwd + ( right * wep_spread.x ) + ( up * wep_spread.y ) ).normalized ( );

		end = start + ( dir * g_cl.m_weapon_info->m_range );

		g_csgo.m_engine_trace->ClipRayToEntity ( Ray ( start, end ), MASK_SHOT, player, &tr );

		if ( tr.m_entity == player && game::IsValidHitgroup ( tr.m_hitgroup ) )
			++total_hits;

		if ( total_hits >= needed_hits )
			return true;

		if ( ( SEED_MAX - i + total_hits ) < needed_hits )
			return false;
	}

	return false;
}

bool AimPlayer::SetupHitboxPoints ( LagRecord *record, BoneArray *bones, int index, std::vector< vec3_t > &points ) {
	points.clear ( );

	const model_t *model = m_player->GetModel ( );
	if ( !model )
		return false;

	studiohdr_t *hdr = g_csgo.m_model_info->GetStudioModel ( model );
	if ( !hdr )
		return false;

	mstudiohitboxset_t *set = hdr->GetHitboxSet ( m_player->m_nHitboxSet ( ) );
	if ( !set )
		return false;

	mstudiobbox_t *bbox = set->GetHitbox ( index );
	if ( !bbox )
		return false;

	float scale = g_menu.main.aimbot.scale.get ( ) / 100.f;

	if ( !( record->m_pred_flags ) & FL_ONGROUND )
		scale = 0.7f;

	float bscale = g_menu.main.aimbot.body_scale.get ( ) / 100.f;

	if ( bbox->m_radius <= 0.f ) {
		matrix3x4_t rot_matrix;
		g_csgo.AngleMatrix ( bbox->m_angle, rot_matrix );

		matrix3x4_t matrix;
		math::ConcatTransforms ( bones [ bbox->m_bone ], rot_matrix, matrix );

		vec3_t origin = matrix.GetOrigin ( );

		vec3_t center = ( bbox->m_mins + bbox->m_maxs ) / 2.f;

		if ( index == HITBOX_R_FOOT || index == HITBOX_L_FOOT ) {
			float d1 = ( bbox->m_mins.z - center.z ) * 0.875f;

			if ( index == HITBOX_L_FOOT )
				d1 *= -1.f;

			points.push_back ( { center.x, center.y, center.z + d1 } );

			if ( g_menu.main.aimbot.multipoint.get ( 3 ) ) {
				float d2 = ( bbox->m_mins.x - center.x ) * scale;
				float d3 = ( bbox->m_maxs.x - center.x ) * scale;

				points.push_back ( { center.x + d2, center.y, center.z } );

				points.push_back ( { center.x + d3, center.y, center.z } );
			}
		}

		if ( points.empty ( ) )
			return false;

		for ( auto &p : points ) {
			p = { p.dot ( matrix [ 0 ] ), p.dot ( matrix [ 1 ] ), p.dot ( matrix [ 2 ] ) };

			p += origin;
		}
	}

	else {
		float r = bbox->m_radius * scale;
		float br = bbox->m_radius * bscale;

		vec3_t center = ( bbox->m_mins + bbox->m_maxs ) / 2.f;

		if ( index == HITBOX_HEAD ) {
			points.push_back ( center );

			if ( g_menu.main.aimbot.multipoint.get ( 0 ) ) {
				constexpr float rotation = 0.70710678f;

				points.push_back ( { bbox->m_maxs.x + ( rotation * r ), bbox->m_maxs.y + ( -rotation * r ), bbox->m_maxs.z } );

				points.push_back ( { bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r } );

				points.push_back ( { bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r } );

				points.push_back ( { bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z } );

				CCSGOGamePlayerAnimState *state = ( CCSGOGamePlayerAnimState * ) record->m_player->m_PlayerAnimState ( );

				if ( state && state->m_flVelocityLengthXY <= 0.1f && record->m_eye_angles.x <= state->m_flAimPitchMin ) {
					points.push_back ( { bbox->m_maxs.x - r, bbox->m_maxs.y, bbox->m_maxs.z } );
				}
			}
		}

		else if ( index == HITBOX_BODY ) {
			points.push_back ( center );

			if ( g_menu.main.aimbot.multipoint.get ( 2 ) )
				points.push_back ( { center.x, bbox->m_maxs.y - br, center.z } );
		}

		else if ( index == HITBOX_PELVIS || index == HITBOX_UPPER_CHEST ) {
			points.push_back ( { center.x, bbox->m_maxs.y - r, center.z } );
		}

		else if ( index == HITBOX_THORAX || index == HITBOX_CHEST ) {
			points.push_back ( center );

			if ( g_menu.main.aimbot.multipoint.get ( 1 ) )
				points.push_back ( { center.x, bbox->m_maxs.y - r, center.z } );
		}

		else if ( index == HITBOX_R_CALF || index == HITBOX_L_CALF ) {
			points.push_back ( center );

			if ( g_menu.main.aimbot.multipoint.get ( 3 ) )
				points.push_back ( { bbox->m_maxs.x - ( bbox->m_radius / 2.f ), bbox->m_maxs.y, bbox->m_maxs.z } );
		}

		else if ( index == HITBOX_R_THIGH || index == HITBOX_L_THIGH ) {
			points.push_back ( center );
		}

		else if ( index == HITBOX_R_UPPER_ARM || index == HITBOX_L_UPPER_ARM ) {
			points.push_back ( { bbox->m_maxs.x + bbox->m_radius, center.y, center.z } );
		}

		if ( points.empty ( ) )
			return false;

		for ( auto &p : points )
			math::VectorTransform ( p, bones [ bbox->m_bone ], p );
	}

	return true;
}

bool AimPlayer::GetBestAimPosition ( vec3_t &aim, float &damage, LagRecord *record ) {
	bool                  done, pen;
	float                 dmg, pendmg;
	HitscanData_t         scan;
	std::vector< vec3_t > points;

	int hp = std::min ( 100, m_player->m_iHealth ( ) );

	if ( g_cl.m_weapon_id == ZEUS ) {
		dmg = pendmg = hp;
		pen = true;
	}

	else {
		dmg = g_menu.main.aimbot.minimal_damage.get ( );
		if ( g_menu.main.aimbot.minimal_damage_hp.get ( ) )
			dmg = std::ceil ( ( dmg / 100.f ) * hp );

		pendmg = g_menu.main.aimbot.penetrate_minimal_damage.get ( );
		if ( g_menu.main.aimbot.penetrate_minimal_damage_hp.get ( ) )
			pendmg = std::ceil ( ( pendmg / 100.f ) * hp );

		pen = g_menu.main.aimbot.penetrate.get ( );
	}

	record->cache ( );

	for ( const auto &it : m_hitboxes ) {
		done = false;

		if ( !SetupHitboxPoints ( record, record->m_bones, it.m_index, points ) )
			continue;

		for ( const auto &point : points ) {
			penetration::PenetrationInput_t in;

			in.m_damage = dmg;
			in.m_damage_pen = pendmg;
			in.m_can_pen = pen;
			in.m_target = m_player;
			in.m_from = g_cl.m_local;
			in.m_pos = point;

			if ( it.m_mode == HitscanMode::LETHAL || it.m_mode == HitscanMode::LETHAL2 )
				in.m_damage = in.m_damage_pen = 1.f;

			penetration::PenetrationOutput_t out;

			if ( penetration::run ( &in, &out ) ) {
				if ( it.m_index == HITBOX_HEAD && out.m_hitgroup != HITGROUP_HEAD )
					continue;

				if ( it.m_mode == HitscanMode::PREFER )
					done = true;

				else if ( it.m_mode == HitscanMode::LETHAL && out.m_damage >= m_player->m_iHealth ( ) )
					done = true;

				else if ( it.m_mode == HitscanMode::LETHAL2 && ( out.m_damage * 2.f ) >= m_player->m_iHealth ( ) )
					done = true;

				else if ( it.m_mode == HitscanMode::NORMAL ) {
					if ( out.m_damage > scan.m_damage ) {
						scan.m_damage = out.m_damage;
						scan.m_pos = point;

						if ( point == points.front ( ) && out.m_damage >= m_player->m_iHealth ( ) )
							break;
					}
				}

				if ( done ) {
					scan.m_damage = out.m_damage;
					scan.m_pos = point;
					break;
				}
			}
		}

		if ( done )
			break;
	}

	if ( scan.m_damage > 0.f ) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		return true;
	}

	return false;
}

bool Aimbot::SelectTarget ( LagRecord *record, const vec3_t &aim, float damage ) {
	float dist, fov, height;
	int   hp;

	if ( g_menu.main.aimbot.fov.get ( ) ) {
		if ( math::GetFOV ( g_cl.m_view_angles, g_cl.m_shoot_pos, aim ) > g_menu.main.aimbot.fov_amount.get ( ) )
			return false;
	}

	switch ( g_menu.main.aimbot.selection.get ( ) ) {
	case 0:
		dist = ( record->m_origin - g_cl.m_shoot_pos ).length ( );

		if ( dist < m_best_dist ) {
			m_best_dist = dist;
			return true;
		}

		break;

	case 1:
		fov = math::GetFOV ( g_cl.m_view_angles, g_cl.m_shoot_pos, aim );

		if ( fov < m_best_fov ) {
			m_best_fov = fov;
			return true;
		}

		break;

	case 2:
		if ( damage > m_best_damage ) {
			m_best_damage = damage;
			return true;
		}

		break;

	case 3:
		hp = std::min ( 100, record->m_player->m_iHealth ( ) );

		if ( hp < m_best_hp ) {
			m_best_hp = hp;
			return true;
		}

		break;

	case 4:
		if ( record->m_lag < m_best_lag ) {
			m_best_lag = record->m_lag;
			return true;
		}

		break;

	case 5:
		height = record->m_origin.z - g_cl.m_local->m_vecOrigin ( ).z;

		if ( height < m_best_height ) {
			m_best_height = height;
			return true;
		}

		break;

	default:
		return false;
	}

	return false;
}

void Aimbot::apply ( ) {
	static auto sv_showlagcompensation_duration = g_csgo.m_cvar->FindVar ( HASH ( "sv_showlagcompensation_duration" ) );

	bool attack, attack2;
	attack = ( g_cl.m_cmd->m_buttons & IN_ATTACK );
	attack2 = ( g_cl.m_weapon_id == REVOLVER && g_cl.m_cmd->m_buttons & IN_ATTACK2 );

	if ( attack || attack2 ) {
		*g_cl.m_packet = false;

		if ( m_target ) {
			if ( m_record && !m_record->m_broke_lc )
				g_cl.m_cmd->m_tick = game::TIME_TO_TICKS ( m_record->m_sim_time + g_cl.m_lerp );

			g_cl.m_cmd->m_view_angles = m_angle;

			if ( !g_menu.main.aimbot.silent.get ( ) )
				g_csgo.m_engine->SetViewAngles ( m_angle );

			if ( g_menu.main.aimbot.show_capsules.get ( ) ) {
				g_visuals.DrawHitboxMatrix ( m_record, { 255, 255, 255, 150 }, sv_showlagcompensation_duration->GetFloat ( ) );

				//#ifdef _DEBUG
				//				BoneArray server_bones [ 128 ];
				//				g_bones.BuildServer ( m_record->m_player, server_bones, g_csgo.m_globals->m_curtime, false );
				//				g_visuals.DrawHitboxMatrix ( m_record->m_player, server_bones, { 255, 0, 0, 150 }, sv_showlagcompensation_duration->GetFloat ( ) );
				//#endif
			}
		}

		if ( g_menu.main.aimbot.nospread.get ( ) && g_menu.main.config.mode.get ( ) == 1 )
			NoSpread ( );

		if ( g_menu.main.aimbot.norecoil.get ( ) )
			g_cl.m_cmd->m_view_angles -= g_cl.m_local->m_aimPunchAngle ( ) * g_csgo.weapon_recoil_scale->GetFloat ( );

		g_shots.OnShotFire ( m_target ? m_target : nullptr, m_target ? m_damage : -1.f, g_cl.m_weapon_info->m_bullets, m_target ? m_record : nullptr );
		g_cl.m_shot = true;
	}
}

void Aimbot::NoSpread ( ) {
	bool    attack2;
	vec3_t  spread, forward, right, up, dir;

	attack2 = ( g_cl.m_weapon_id == REVOLVER && ( g_cl.m_cmd->m_buttons & IN_ATTACK2 ) );

	spread = g_cl.m_weapon->CalculateSpread ( g_cl.m_cmd->m_random_seed, attack2 );

	g_cl.m_cmd->m_view_angles -= { -math::rad_to_deg ( std::atan ( spread.length_2d ( ) ) ), 0.f, math::rad_to_deg ( std::atan2 ( spread.x, spread.y ) ) };
}