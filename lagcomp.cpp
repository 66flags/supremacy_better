#include "includes.h"

LagCompensation g_lagcomp {};;

void LagCompensation::Simulate ( AimPlayer *data ) {
}

bool LagCompensation::StartPrediction ( AimPlayer *data ) {
	// we have no data to work with.
	// this should never happen if we call this
	if ( data->m_records.empty ( ) )
		return false;

	// meme.
	if ( data->m_player->dormant ( ) )
		return false;

	// compute the true amount of updated records
	// since the last time the player entered pvs.
	size_t size {};

	// iterate records.
	for ( const auto &it : data->m_records ) {
		if ( it->dormant ( ) )
			break;

		// increment total amount of data.
		++size;
	}

	// get first record.
	LagRecord *record = data->m_records [ 0 ].get ( );
	LagRecord *current = data->m_records.front ( ).get ( );

	// reset all prediction related variables.
	// this has been a recurring problem in all my hacks lmfao.
	// causes the prediction to stack on eachother.
	record->predict ( );

	float flTargetTime = game::TICKS_TO_TIME ( g_cl.m_cmd->m_tick ) - g_cl.m_lerp;

	// check if lc broken.
	if ( size > 1 && ( ( record->m_origin - data->m_records [ 1 ]->m_origin ).length_sqr ( ) > 4096.f
		|| size > 2 && ( data->m_records [ 1 ]->m_origin - data->m_records [ 2 ]->m_origin ).length_sqr ( ) > 4096.f ) )
		record->m_broke_lc = true;

	if ( !record->m_broke_lc )
		return false;

	if ( record->m_sim_time <= flTargetTime )
		return false; // hurra, stop

	int simulation = game::TIME_TO_TICKS ( record->m_sim_time );

	// this is too much lag to fix.
	if ( std::abs ( g_cl.m_arrival_tick - simulation ) >= 128 )
		return true;

	// compute the amount of lag that we will predict for, if we have one set of data, use that.
	// if we have more data available, use the prevoius lag delta to counter weird fakelags that switch between 14 and 2.
	int lag = game::TIME_TO_TICKS ( record->m_sim_time - record->m_old_sim_time );

	// clamp this just to be sure.
	math::clamp ( lag, 1, 17 );

	// get the delta in ticks between the last server net update
	// and the net update on which we created this record.
	int updatedelta = g_cl.m_server_tick - record->m_tick;

	// if the lag delta that is remaining is less than the current netlag
	// that means that we can shoot now and when our shot will get processed
	// the origin will still be valid, therefore we do not have to predict.
	if ( g_cl.m_latency_ticks <= lag - updatedelta )
		return true;

	// the next update will come in, wait for it.
	int next = record->m_tick + 1;
	if ( next + lag >= g_cl.m_arrival_tick )
		return true;

	float change = 0.f, dir = 0.f;

	// get the direction of the current velocity.
	if ( record->m_velocity.y != 0.f || record->m_velocity.x != 0.f )
		dir = math::rad_to_deg ( std::atan2 ( record->m_velocity.y, record->m_velocity.x ) );

	// we have more than one update
	// we can compute the direction.
	if ( size > 1 ) {
		// get the delta time between the 2 most recent records.
		float dt = record->m_sim_time - data->m_records [ 1 ]->m_sim_time;

		// init to 0.
		float prevdir = 0.f;

		// get the direction of the prevoius velocity.
		if ( data->m_records [ 1 ]->m_velocity.y != 0.f || data->m_records [ 1 ]->m_velocity.x != 0.f )
			prevdir = math::rad_to_deg ( std::atan2 ( data->m_records [ 1 ]->m_velocity.y, data->m_records [ 1 ]->m_velocity.x ) );

		// compute the direction change per tick.
		change = ( math::NormalizedAngle ( dir - prevdir ) / dt ) * g_csgo.m_globals->m_interval;
	}

	if ( std::abs ( change ) > 6.f )
		change = 0.f;

	// get the pointer to the players animation state.
	CCSGOPlayerAnimState *state = data->m_player->m_PlayerAnimState ( );

	// backup the animation state.
	CCSGOPlayerAnimState backup {};
	if ( state )
		std::memcpy ( &backup, state, sizeof ( CCSGOPlayerAnimState ) );

	// add in the shot prediction here.
	int shot = 0;

	/*Weapon* pWeapon = data->m_player->GetActiveWeapon( );
	if( pWeapon && !data->m_fire_bullet.empty( ) ) {
		static Address offset = g_netvars.get( HASH( "DT_BaseCombatWeapon" ), HASH( "m_fLastShotTime" ) );
		float last = pWeapon->get< float >( offset );

		if( game::TIME_TO_TICKS( data->m_fire_bullet.front( ).m_sim_time - last ) == 1 ) {
			WeaponInfo* wpndata = pWeapon->GetWpnData( );

			if( wpndata )
				shot = game::TIME_TO_TICKS( last + wpndata->m_cycletime ) + 1;
		}
	}*/

	int pred = 0;

	for ( int i = 0; i < lag; i++ ) {
		const auto time = game::TICKS_TO_TIME ( i + 1 );

		if ( size > 1 ) {
			auto previous = data->m_records [ 1 ];

			if ( previous ) {
				auto delta = static_cast< float >( i + 1 ) / static_cast< float >( record->m_lag );

				record->m_player->m_vecVelocity ( ).x = math::Lerp ( record->m_velocity.x, previous->m_velocity.x, delta );
				record->m_player->m_vecVelocity ( ).y = math::Lerp ( record->m_velocity.y, previous->m_velocity.y, delta );
				record->m_player->m_vecVelocity ( ).z = math::Lerp ( record->m_velocity.z, previous->m_velocity.z, delta );
			}
		}

		if ( !( record->m_flags & FL_ONGROUND ) )
			record->m_velocity.z -= g_csgo.sv_gravity->GetFloat ( ) * game::TICKS_TO_TIME ( 1 );
		else if ( !( record->m_prev_flags & FL_ONGROUND ) )
			record->m_velocity.z = g_csgo.sv_jump_impulse->GetFloat ( );

		vec3_t start, end, normal;
		CGameTrace trace;
		CTraceFilterSimple filter ( record->m_player );

		start = record->m_origin;
		end = start + record->m_velocity * game::TICKS_TO_TIME ( 1 );

		Ray ray ( start, end, record->m_player->m_vecMins ( ), record->m_player->m_vecMaxs ( ) );
		g_csgo.m_engine_trace->TraceRay ( ray, MASK_PLAYERSOLID, &filter, &trace );

		if ( trace.m_fraction != 1.0f ) {
			for ( auto i = 0; i < 2; ++i ) {
				record->m_velocity -= trace.m_plane.m_normal * record->m_velocity.dot ( trace.m_plane.m_normal );

				float adjust = record->m_velocity.dot ( trace.m_plane.m_normal );

				if ( adjust < 0.0f )
					record->m_velocity -= ( trace.m_plane.m_normal * adjust );

				start = trace.m_endpos;
				end = start + ( record->m_velocity * ( game::TICKS_TO_TIME ( 1 ) * ( 1.0f - trace.m_fraction ) ) );

				ray.Init ( start, end, record->m_player->m_vecMins ( ), record->m_player->m_vecMaxs ( ) );
				g_csgo.m_engine_trace->TraceRay ( ray, MASK_PLAYERSOLID, &filter, &trace );

				if ( trace.m_fraction == 1.0f )
					break;
			}
		}

		start = end = record->m_origin = trace.m_endpos;
		end.z -= 2.0f;

		ray.Init ( start, end, record->m_player->m_vecMins ( ), record->m_player->m_vecMaxs ( ) );
		g_csgo.m_engine_trace->TraceRay ( ray, MASK_PLAYERSOLID, &filter, &trace );

		record->m_prev_flags = record->m_flags;

		record->m_flags &= ~FL_ONGROUND;

		if ( trace.m_fraction != 1.0f && trace.m_plane.m_normal.z > 0.7f )
			record->m_flags |= FL_ONGROUND;

		record->m_pred_time += g_csgo.m_globals->m_interval;
		record->m_did_predict = true;
		pred++;

		if ( i == 0 && state )
			PredictAnimations ( state, record );
	}

	// restore state.
	if ( state )
		std::memcpy ( state, &backup, sizeof ( CCSGOPlayerAnimState ) );

	if ( pred <= 0 )
		return true;

	// lagcomp broken, invalidate bones.
	record->invalidate ( );

	// re-setup bones for this record.
	g_bones.Build ( data->m_player, record, record->m_bones, g_csgo.m_globals->m_curtime );

	return true;
}

void LagCompensation::PlayerMove ( LagRecord *record ) {
}

void LagCompensation::AirAccelerate ( LagRecord *record, ang_t angle, float fmove, float smove ) {
	vec3_t fwd, right, wishvel, wishdir;
	float  maxspeed, wishspd, wishspeed, currentspeed, addspeed, accelspeed;

	// determine movement angles.
	math::AngleVectors ( angle, &fwd, &right );

	// zero out z components of movement vectors.
	fwd.z = 0.f;
	right.z = 0.f;

	// normalize remainder of vectors.
	fwd.normalize ( );
	right.normalize ( );

	// determine x and y parts of velocity.
	for ( int i {}; i < 2; ++i )
		wishvel [ i ] = ( fwd [ i ] * fmove ) + ( right [ i ] * smove );

	// zero out z part of velocity.
	wishvel.z = 0.f;

	// determine maginitude of speed of move.
	wishdir = wishvel;
	wishspeed = wishdir.normalize ( );

	// get maxspeed.
	// TODO; maybe global this or whatever its 260 anyway always.
	maxspeed = record->m_player->m_flMaxspeed ( );

	// clamp to server defined max speed.
	if ( wishspeed != 0.f && wishspeed > maxspeed )
		wishspeed = maxspeed;

	// make copy to preserve original variable.
	wishspd = wishspeed;

	// cap speed.
	if ( wishspd > 30.f )
		wishspd = 30.f;

	// determine veer amount.
	currentspeed = record->m_pred_velocity.dot ( wishdir );

	// see how much to add.
	addspeed = wishspd - currentspeed;

	// if not adding any, done.
	if ( addspeed <= 0.f )
		return;

	// Determine acceleration speed after acceleration
	accelspeed = g_csgo.sv_airaccelerate->GetFloat ( ) * wishspeed * g_csgo.m_globals->m_interval;

	// cap it.
	if ( accelspeed > addspeed )
		accelspeed = addspeed;

	// add accel.
	record->m_pred_velocity += ( wishdir * accelspeed );
}

void LagCompensation::PredictAnimations ( CCSGOPlayerAnimState *state, LagRecord *record ) {
	struct AnimBackup_t {
		float  curtime;
		float  frametime;
		int    flags;
		int    eflags;
		vec3_t velocity;
		vec3_t mins, maxs;
		vec3_t origin;
		float  lby;
		float simtime, oldsimtime;
		float duck;
	};

	// get player ptr.
	Player *player = record->m_player;

	// backup data.
	AnimBackup_t backup;

	const auto dword_44732EA4 = g_csgo.m_globals->m_realtime;
	const auto dword_44732EA8 = g_csgo.m_globals->m_curtime;
	const auto dword_44732EAC = g_csgo.m_globals->m_frametime;
	const auto dword_44732EB0 = g_csgo.m_globals->m_abs_frametime;
	const auto dword_44732EB4 = g_csgo.m_globals->m_interp_amt;
	const auto dword_44732EB8 = g_csgo.m_globals->m_frame;
	const auto dword_44732EBC = g_csgo.m_globals->m_tick_count;
	const auto v4 = dword_44732EA8 / g_csgo.m_globals->m_interval;

	const auto sim_ticks = ( v4 + 0.5f );

	backup.curtime = g_csgo.m_globals->m_curtime;
	backup.frametime = g_csgo.m_globals->m_frametime;
	backup.flags = player->m_fFlags ( );
	backup.eflags = player->m_iEFlags ( );
	backup.velocity = player->m_vecAbsVelocity ( );
	backup.origin = player->m_vecOrigin ( );
	backup.duck = player->m_flDuckAmount ( );
	backup.mins = player->m_vecMins ( );
	backup.maxs = player->m_vecMaxs ( );
	backup.lby = player->m_flLowerBodyYawTarget ( );

	// set globals appropriately for animation.
	g_csgo.m_globals->m_curtime = record->m_pred_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	// EFL_DIRTY_ABSVELOCITY
	// skip call to C_BaseEntity::CalcAbsoluteVelocity
	player->m_iEFlags ( ) &= ~0x1000;

	// set predicted flags and velocity.
	player->m_fFlags ( ) = record->m_flags;
	player->m_vecAbsVelocity ( ) = record->m_velocity;

	// enable re-animation in the same frame if animated already.
	if ( state->m_frame >= g_csgo.m_globals->m_frame )
		state->m_frame = g_csgo.m_globals->m_frame - 1;

	bool fake = g_menu.main.aimbot.correct.get ( );

	// rerun the resolver since we edited the origin.
	if ( fake )
		g_resolver.ResolveAngles ( player, record );

	// update animations.
	rebuilt::UpdateAnimationState ( ( CCSGOGamePlayerAnimState * ) state, record->m_eye_angles.y, record->m_eye_angles.x, false );

	// rerun the pose correction cuz we are re-setupping them.
	if ( fake )
		g_resolver.ResolvePoses ( player, record );

	// get new rotation poses and layers.
	player->GetPoseParameters ( record->m_poses );
	player->GetAnimLayers ( record->m_layers );
	record->m_abs_ang = player->GetAbsAngles ( );

	// restore globals.
	g_csgo.m_globals->m_curtime = backup.curtime;
	g_csgo.m_globals->m_frametime = backup.frametime;
	g_csgo.m_globals->m_realtime = dword_44732EA4;
	g_csgo.m_globals->m_curtime = dword_44732EA8;
	g_csgo.m_globals->m_frametime = dword_44732EAC;
	g_csgo.m_globals->m_abs_frametime = dword_44732EB0;
	g_csgo.m_globals->m_interp_amt = dword_44732EB4;
	g_csgo.m_globals->m_frame = dword_44732EB8;
	g_csgo.m_globals->m_tick_count = dword_44732EBC;

	// invalidate physics
	rebuilt::InvalidatePhysicsRecursive ( player, 8 );

	// restore player data.
	player->m_fFlags ( ) = backup.flags;
	player->m_iEFlags ( ) = backup.eflags;
	player->m_vecAbsVelocity ( ) = backup.velocity;
	player->m_vecOrigin ( ) = backup.origin;
	player->m_flDuckAmount ( ) = backup.duck;
	player->m_vecMins ( ) = backup.mins;
	player->m_vecMaxs ( ) = backup.maxs;
	player->m_flLowerBodyYawTarget ( ) = backup.lby;
}