#include "includes.h"

Movement g_movement{ };;

void Movement::JumpRelated( ) {
	if( g_cl.m_local->m_MoveType( ) == MOVETYPE_NOCLIP )
		return;

	if( ( g_cl.m_cmd->m_buttons & IN_JUMP ) && !( g_cl.m_flags & FL_ONGROUND ) ) {
		// bhop.
		if( g_menu.main.movement.bhop.get( ) )
			g_cl.m_cmd->m_buttons &= ~IN_JUMP;

		// duck jump ( crate jump ).
		if( g_menu.main.movement.airduck.get( ) )
			g_cl.m_cmd->m_buttons |= IN_DUCK;
	}
}

void Movement::DirectionalStrafe ( CUserCmd *cmd, const ang_t &old_angs ) {
	static int strafer_flags = 0;

	if ( !!( g_cl.m_local->m_fFlags ( ) & FL_ONGROUND ) ) {
		strafer_flags = 0;
		return;
	}

	auto velocity = g_cl.m_local->m_vecVelocity ( );
	auto velocity_len = velocity.length_2d ( );

	if ( velocity_len <= 0.0f ) {
		strafer_flags = 0;
		return;
	}

	auto ideal_step = std::min < float > ( 90.0f, 845.5f / velocity_len );
	auto velocity_yaw = ( velocity.y || velocity.x ) ? math::rad_to_deg ( atan2f ( velocity.y, velocity.x ) ) : 0.0f;

	auto unmod_angles = old_angs;
	auto angles = old_angs;

	if ( velocity_len < 2.0f && !!( cmd->m_buttons & IN_JUMP ) )
		cmd->m_forward_move = 450.0f;

	auto forward_move = cmd->m_forward_move;
	auto onground = !!( g_cl.m_local->m_fFlags ( ) & FL_ONGROUND );

	if ( forward_move || cmd->m_side_move ) {
		cmd->m_forward_move = 0.0f;

		if ( velocity_len != 0.0f && abs ( velocity.z ) != 0.0f ) {
			if ( !onground ) {
			DO_IT_AGAIN:
				vec3_t fwd;
				math::AngleVectors ( angles, &fwd );
				auto right = fwd.cross ( vec3_t ( 0.0f, 0.0f, 1.0f ) );

				auto v262 = ( fwd.x * forward_move ) + ( cmd->m_side_move * right.x );
				auto v263 = ( right.y * cmd->m_side_move ) + ( fwd.y * forward_move );

				angles.y = ( v262 || v263 ) ? math::rad_to_deg ( atan2f ( v263, v262 ) ) : 0.0f;
			}
		}
	}

	auto yaw_to_use = 0.0f;

	strafer_flags &= ~4;

	if ( !onground ) {
		auto clamped_angles = angles.y;

		if ( clamped_angles < -180.0f ) clamped_angles += 360.0f;
		if ( clamped_angles > 180.0f ) clamped_angles -= 360.0f;

		yaw_to_use = old_angs.y;

		strafer_flags |= 4;
	}

	if ( strafer_flags & 4 ) {
		auto diff = angles.y - yaw_to_use;

		if ( diff < -180.0f ) diff += 360.0f;
		if ( diff > 180.0f ) diff -= 360.0f;

		if ( abs ( diff ) > ideal_step && abs ( diff ) <= 30.0f ) {
			auto move = 450.0f;

			if ( diff < 0.0f )
				move *= -1.0f;

			cmd->m_side_move = move;
			return;
		}
	}

	auto diff = angles.y - velocity_yaw;

	if ( diff < -180.0f ) diff += 360.0f;
	if ( diff > 180.0f ) diff -= 360.0f;

	auto step = 0.6f * ( ideal_step + ideal_step );
	auto side_move = 0.0f;

	if ( abs ( diff ) > 170.0f && velocity_len > 80.0f || diff > step && velocity_len > 80.0f ) {
		angles.y = step + velocity_yaw;
		cmd->m_side_move = -450.0f;
	}
	else if ( -step <= diff || velocity_len <= 80.0f ) {
		if ( strafer_flags & 1 ) {
			angles.y -= ideal_step;
			cmd->m_side_move = -450.0f;
		}
		else {
			angles.y += ideal_step;
			cmd->m_side_move = 450.0f;
		}
	}
	else {
		angles.y = velocity_yaw - step;
		cmd->m_side_move = 450.0f;
	}

	if ( !( cmd->m_buttons & IN_BACK ) && !cmd->m_side_move )
		goto DO_IT_AGAIN;

	strafer_flags ^= ( strafer_flags ^ ~strafer_flags ) & 1;

	if ( angles.y < -180.0f ) angles.y += 360.0f;
	if ( angles.y > 180.0f ) angles.y -= 360.0f;

	FixMove ( cmd, angles );
}

void Movement::Strafe( CUserCmd *cmd, const ang_t &old_angs ) {
	// don't strafe while noclipping or on ladders..
	if( g_cl.m_local->m_MoveType( ) == MOVETYPE_NOCLIP || g_cl.m_local->m_MoveType( ) == MOVETYPE_LADDER )
		return;

	// disable strafing while pressing shift.
	// don't strafe if not holding primary jump key.
	if( ( g_cl.m_buttons & IN_SPEED ) || !( g_cl.m_buttons & IN_JUMP ) || ( g_cl.m_flags & FL_ONGROUND ) )
		return;

	//static cvar_t *cl_sidespeed = interfaces::m_cvar->find_var ( HASH ( "cl_sidespeed" ) );
	//static cvar_t *cl_forwardspeed = interfaces::m_cvar->find_var ( HASH ( "cl_forwardspeed" ) );

	if ( g_menu.main.movement.autostrafe.get ( ) && g_cl.m_local->m_vecVelocity ( ).length_2d ( ) >= 30.0f ) {
		if ( g_menu.main.movement.strafe_type.get ( ) == 1 ) {
			DirectionalStrafe ( cmd, old_angs );
		}
		else {
			if ( std::abs ( cmd->m_mousedx ) > 2 ) {
				cmd->m_side_move = cmd->m_mousedx < 0 ? -g_csgo.cl_sidespeed->GetFloat ( ) : g_csgo.cl_sidespeed->GetFloat ( );
			}
			else {
				cmd->m_side_move = cmd->m_command_number % 2 ? -g_csgo.cl_sidespeed->GetFloat ( ) : g_csgo.cl_sidespeed->GetFloat ( );
				cmd->m_forward_move = g_csgo.cl_forwardspeed->GetFloat ( );
			}
		}
	}
}

void Movement::DoPrespeed( ) {
	float   mod, min, max, step, strafe, time, angle;
	vec3_t  plane;

	// min and max values are based on 128 ticks.
	mod = g_csgo.m_globals->m_interval * 128.f;

	// scale min and max based on tickrate.
	min = 2.25f * mod;
	max = 5.f * mod;

	// compute ideal strafe angle for moving in a circle.
	strafe = m_ideal * 2.f;

	// clamp ideal strafe circle value to min and max step.
	math::clamp( strafe, min, max );

	// calculate time.
	time = 320.f / m_speed;

	// clamp time.
	math::clamp( time, 0.35f, 1.f );

	// init step.
	step = strafe;

	while( true ) {
		// if we will not collide with an object or we wont accelerate from such a big step anymore then stop.
		if( !WillCollide( time, step ) || max <= step )
			break;

		// if we will collide with an object with the current strafe step then increment step to prevent a collision.
		step += 0.2f;
	}

	if( step > max ) {
		// reset step.
		step = strafe;

		while( true ) {
			// if we will not collide with an object or we wont accelerate from such a big step anymore then stop.
			if( !WillCollide( time, step ) || step <= -min )
				break;

			// if we will collide with an object with the current strafe step decrement step to prevent a collision.
			step -= 0.2f;
		}

		if( step < -min ) {
			if( GetClosestPlane( plane ) ) {
				// grab the closest object normal
				// compute the angle of the normal
				// and push us away from the object.
				angle = math::rad_to_deg( std::atan2( plane.y, plane.x ) );
				step = -math::NormalizedAngle( m_circle_yaw - angle ) * 0.1f;
			}
		}

		else
			step -= 0.2f;
	}

	else
		step += 0.2f;

	// add the computed step to the steps of the previous circle iterations.
	m_circle_yaw = math::NormalizedAngle( m_circle_yaw + step );

	// apply data to usercmd.
	g_cl.m_cmd->m_view_angles.y = m_circle_yaw;
	g_cl.m_cmd->m_side_move = ( step >= 0.f ) ? -450.f : 450.f;
}

bool Movement::GetClosestPlane( vec3_t &plane ) {
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;
	vec3_t                start{ m_origin };
	float                 smallest{ 1.f };
	const float		      dist{ 75.f };

	// trace around us in a circle
	for( float step{ }; step <= math::pi_2; step += ( math::pi / 10.f ) ) {
		// extend endpoint x units.
		vec3_t end = start;
		end.x += std::cos( step ) * dist;
		end.y += std::sin( step ) * dist;

		g_csgo.m_engine_trace->TraceRay( Ray( start, end, m_mins, m_maxs ), CONTENTS_SOLID, &filter, &trace );

		// we found an object closer, then the previouly found object.
		if( trace.m_fraction < smallest ) {
			// save the normal of the object.
			plane = trace.m_plane.m_normal;
			smallest = trace.m_fraction;
		}
	}

	// did we find any valid object?
	return smallest != 1.f && plane.z < 0.1f;
}

bool Movement::WillCollide( float time, float change ) {
	struct PredictionData_t {
		vec3_t start;
		vec3_t end;
		vec3_t velocity;
		float  direction;
		bool   ground;
		float  predicted;
	};

	PredictionData_t      data;
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;

	// set base data.
	data.ground = g_cl.m_flags & FL_ONGROUND;
	data.start = m_origin;
	data.end = m_origin;
	data.velocity = g_cl.m_local->m_vecVelocity( );
	data.direction = math::rad_to_deg( std::atan2( data.velocity.y, data.velocity.x ) );

	for( data.predicted = 0.f; data.predicted < time; data.predicted += g_csgo.m_globals->m_interval ) {
		// predict movement direction by adding the direction change.
		// make sure to normalize it, in case we go over the -180/180 turning point.
		data.direction = math::NormalizedAngle( data.direction + change );

		// pythagoras.
		float hyp = data.velocity.length_2d( );

		// adjust velocity for new direction.
		data.velocity.x = std::cos( math::deg_to_rad( data.direction ) ) * hyp;
		data.velocity.y = std::sin( math::deg_to_rad( data.direction ) ) * hyp;

		// assume we bhop, set upwards impulse.
		if( data.ground )
			data.velocity.z = g_csgo.sv_jump_impulse->GetFloat( );

		else
			data.velocity.z -= g_csgo.sv_gravity->GetFloat( ) * g_csgo.m_globals->m_interval;

		// we adjusted the velocity for our new direction.
		// see if we can move in this direction, predict our new origin if we were to travel at this velocity.
		data.end += ( data.velocity * g_csgo.m_globals->m_interval );

		// trace
		g_csgo.m_engine_trace->TraceRay( Ray( data.start, data.end, m_mins, m_maxs ), MASK_PLAYERSOLID, &filter, &trace );

		// check if we hit any objects.
		if( trace.m_fraction != 1.f && trace.m_plane.m_normal.z <= 0.9f )
			return true;
		if( trace.m_startsolid || trace.m_allsolid )
			return true;

		// adjust start and end point.
		data.start = data.end = trace.m_endpos;

		// move endpoint 2 units down, and re-trace.
		// do this to check if we are on th floor.
		g_csgo.m_engine_trace->TraceRay( Ray( data.start, data.end - vec3_t{ 0.f, 0.f, 2.f }, m_mins, m_maxs ), MASK_PLAYERSOLID, &filter, &trace );

		// see if we moved the player into the ground for the next iteration.
		data.ground = trace.hit( ) && trace.m_plane.m_normal.z > 0.7f;
	}

	// the entire loop has ran
	// we did not hit shit.
	return false;
}

void Movement::FixMove( CUserCmd *cmd, const ang_t &wish_angles ) {
	vec3_t  move, dir;
	float   delta, len;
	ang_t   move_angle;

	// roll nospread fix.
	if( !( g_cl.m_flags & FL_ONGROUND ) && cmd->m_view_angles.z != 0.f )
		cmd->m_side_move = 0.f;

	// convert movement to vector.
	move = { cmd->m_forward_move, cmd->m_side_move, 0.f };

	// get move length and ensure we're using a unit vector ( vector with length of 1 ).
	len = move.normalize( );
	if( !len )
		return;

	// convert move to an angle.
	math::VectorAngles( move, move_angle );

	// calculate yaw delta.
	delta = ( cmd->m_view_angles.y - wish_angles.y );

	// accumulate yaw delta.
	move_angle.y += delta;

	// calculate our new move direction.
	// dir = move_angle_forward * move_length
	math::AngleVectors( move_angle, &dir );

	// scale to og movement.
	dir *= len;

	// strip old flags.
	g_cl.m_cmd->m_buttons &= ~( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT );

	// fix ladder and noclip.
	if( g_cl.m_local->m_MoveType( ) == MOVETYPE_LADDER ) {
		// invert directon for up and down.
		if( cmd->m_view_angles.x >= 45.f && wish_angles.x < 45.f && std::abs( delta ) <= 65.f )
			dir.x = -dir.x;

		// write to movement.
		cmd->m_forward_move = dir.x;
		cmd->m_side_move = dir.y;

		// set new button flags.
		if( cmd->m_forward_move > 200.f )
			cmd->m_buttons |= IN_FORWARD;

		else if( cmd->m_forward_move < -200.f )
			cmd->m_buttons |= IN_BACK;

		if( cmd->m_side_move > 200.f )
			cmd->m_buttons |= IN_MOVERIGHT;

		else if( cmd->m_side_move < -200.f )
			cmd->m_buttons |= IN_MOVELEFT;
	}

	// we are moving normally.
	else {
		// we must do this for pitch angles that are out of bounds.
		if( cmd->m_view_angles.x < -90.f || cmd->m_view_angles.x > 90.f )
			dir.x = -dir.x;

		// set move.
		cmd->m_forward_move = dir.x;
		cmd->m_side_move = dir.y;

		// set new button flags.
		if( cmd->m_forward_move > 0.f )
			cmd->m_buttons |= IN_FORWARD;

		else if( cmd->m_forward_move < 0.f )
			cmd->m_buttons |= IN_BACK;

		if( cmd->m_side_move > 0.f )
			cmd->m_buttons |= IN_MOVERIGHT;

		else if( cmd->m_side_move < 0.f )
			cmd->m_buttons |= IN_MOVELEFT;
	}
}

void Movement::AutoPeek( ) {
	// set to invert if we press the button.
	if( g_input.GetKeyState( g_menu.main.movement.autopeek.get( ) ) ) {
		if( g_cl.m_old_shot )
			m_invert = true;

		vec3_t move{ g_cl.m_cmd->m_forward_move, g_cl.m_cmd->m_side_move, 0.f };

		if( m_invert ) {
			move *= -1.f;
			g_cl.m_cmd->m_forward_move = move.x;
			g_cl.m_cmd->m_side_move = move.y;
		}
	}

	else m_invert = false;

	bool can_stop = g_menu.main.movement.autostop_always_on.get( ) || ( !g_menu.main.movement.autostop_always_on.get( ) && g_input.GetKeyState( g_menu.main.movement.autostop.get( ) ) );
	if( ( g_input.GetKeyState( g_menu.main.movement.autopeek.get( ) ) || can_stop ) && g_aimbot.m_stop ) {
		Movement::QuickStop( );
	}
}

void Movement::QuickStop( ) {
	// convert velocity to angular momentum.
	ang_t angle;
	math::VectorAngles( g_cl.m_local->m_vecVelocity( ), angle );

	// get our current speed of travel.
	float speed = g_cl.m_local->m_vecVelocity( ).length( );

	// fix direction by factoring in where we are looking.
	angle.y = g_cl.m_view_angles.y - angle.y;

	// convert corrected angle back to a direction.
	vec3_t direction;
	math::AngleVectors( angle, &direction );

	vec3_t stop = direction * -speed;

	if( g_cl.m_speed > 13.f ) {
		g_cl.m_cmd->m_forward_move = stop.x;
		g_cl.m_cmd->m_side_move = stop.y;
	}
	else {
		g_cl.m_cmd->m_forward_move = 0.f;
		g_cl.m_cmd->m_side_move = 0.f;
	}
}

void Movement::FakeWalk( ) {
	vec3_t velocity{ g_cl.m_local->m_vecVelocity( ) };
	int    ticks{ }, max{ 16 };

	if( !g_input.GetKeyState( g_menu.main.movement.fakewalk.get( ) ) )
		return;

	if( !g_cl.m_local->GetGroundEntity( ) )
		return;

	// user was running previously and abrubtly held the fakewalk key
	// we should quick-stop under this circumstance to hit the 0.22 flick
	// perfectly, and speed up our fakewalk after running even more.
	//if( g_cl.m_initial_flick ) {
	//	Movement::QuickStop( );
	//	return;
	//}
	
	// reference:
	// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/shared/gamemovement.cpp#L1612

	// calculate friction.
	float friction = g_csgo.sv_friction->GetFloat( ) * g_cl.m_local->m_surfaceFriction( );

	for( ; ticks < g_cl.m_max_lag; ++ticks ) {
		// calculate speed.
		float speed = velocity.length( );

		// if too slow return.
		if( speed <= 0.1f )
			break;

		// bleed off some speed, but if we have less than the bleed, threshold, bleed the threshold amount.
		float control = std::max( speed, g_csgo.sv_stopspeed->GetFloat( ) );

		// calculate the drop amount.
		float drop = control * friction * g_csgo.m_globals->m_interval;

		// scale the velocity.
		float newspeed = std::max( 0.f, speed - drop );

		if( newspeed != speed ) {
			// determine proportion of old speed we are using.
			newspeed /= speed;

			// adjust velocity according to proportion.
			velocity *= newspeed;
		}
	}

	// zero forwardmove and sidemove.
	if( ticks > ( ( max - 1 ) - g_csgo.m_cl->m_choked_commands ) || !g_csgo.m_cl->m_choked_commands ) {
		g_cl.m_cmd->m_forward_move = g_cl.m_cmd->m_side_move = 0.f;
	}
}