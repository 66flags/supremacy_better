#include "includes.h"

Resolver g_resolver {};;

LagRecord *Resolver::FindIdealRecord ( AimPlayer *data ) {
	LagRecord *first_valid, *current;

	if ( data->m_records.empty ( ) )
		return nullptr;

	first_valid = nullptr;

	// iterate records.
	for ( const auto &it : data->m_records ) {
		if ( it->dormant ( ) || it->immune ( ) || !it->valid ( ) )
			continue;

		// get current record.
		current = it.get ( );

		// first record that was valid, store it for later.
		if ( !first_valid )
			first_valid = current;

		// try to find a record with a shot, lby update, walking or no anti-aim.
		if ( it->m_shot || it->m_mode == Modes::RESOLVE_BODY || it->m_mode == Modes::RESOLVE_WALK || it->m_mode == Modes::RESOLVE_NONE )
			return current;
	}

	// none found above, return the first valid record if possible.
	return ( first_valid ) ? first_valid : nullptr;
}

LagRecord *Resolver::FindLastRecord ( AimPlayer *data ) {
	LagRecord *current;

	if ( data->m_records.empty ( ) )
		return nullptr;

	// iterate records in reverse.
	for ( auto it = data->m_records.crbegin ( ); it != data->m_records.crend ( ); ++it ) {
		current = it->get ( );

		// if this record is valid.
		// we are done since we iterated in reverse.
		if ( current->valid ( ) && !current->immune ( ) && !current->dormant ( ) )
			return current;
	}

	return nullptr;
}

void Resolver::OnBodyUpdate ( Player *player, float value ) {
	AimPlayer *data = &g_aimbot.m_players [ player->index ( ) - 1 ];

	// set data.
	data->m_old_body = data->m_body;
	data->m_body = value;
}

float Resolver::GetAwayAngle ( LagRecord *record ) {
	float  delta { std::numeric_limits< float >::max ( ) };
	vec3_t pos;
	ang_t  away;

	// other cheats predict you by their own latency.
	// they do this because, then they can put their away angle to exactly
	// where you are on the server at that moment in time.

	// the idea is that you would need to know where they 'saw' you when they created their user-command.
	// lets say you move on your client right now, this would take half of our latency to arrive at the server.
	// the delay between the server and the target client is compensated by themselves already, that is fortunate for us.

	// we have no historical origins.
	// no choice but to use the most recent one.
	//if( g_cl.m_net_pos.empty( ) ) {
	math::VectorAngles ( g_cl.m_local->m_vecOrigin ( ) - record->m_origin, away );
	return away.y;
	//}

	// half of our rtt.
	// also known as the one-way delay.
	//float owd = ( g_cl.m_latency / 2.f );

	// since our origins are computed here on the client
	// we have to compensate for the delay between our client and the server
	// therefore the OWD should be subtracted from the target time.
	//float target = record->m_pred_time; //- owd;

	// iterate all.
	//for( const auto &net : g_cl.m_net_pos ) {
		// get the delta between this records time context
		// and the target time.
	//	float dt = std::abs( target - net.m_time );

		// the best origin.
	//	if( dt < delta ) {
	//		delta = dt;
	//		pos   = net.m_pos;
	//	}
	//}

	//math::VectorAngles( pos - record->m_pred_origin, away );
	//return away.y;
}

void Resolver::MatchShot ( AimPlayer *data, LagRecord *record ) {
	// do not attempt to do this in nospread mode.
	if ( g_menu.main.config.mode.get ( ) == 1 )
		return;

	float shoot_time = -1.f;

	Weapon *weapon = data->m_player->GetActiveWeapon ( );
	if ( weapon ) {
		// with logging this time was always one tick behind.
		// so add one tick to the last shoot time.
		shoot_time = weapon->m_fLastShotTime ( ) + g_csgo.m_globals->m_interval;
	}

	// this record has a shot on it.
	if ( game::TIME_TO_TICKS ( shoot_time ) == game::TIME_TO_TICKS ( record->m_sim_time ) ) {
		if ( record->m_lag <= 2 )
			record->m_shot = true;

		// more then 1 choke, cant hit pitch, apply prev pitch.
		else if ( data->m_records.size ( ) >= 2 ) {
			LagRecord *previous = data->m_records [ 1 ].get ( );

			if ( previous && !previous->dormant ( ) )
				record->m_eye_angles.x = previous->m_eye_angles.x;
		}
	}
}

void Resolver::SetMode ( LagRecord *record ) {
	// the resolver has 3 modes to chose from.
	// these modes will vary more under the hood depending on what data we have about the player
	// and what kind of hack vs. hack we are playing (mm/nospread).

	float speed = record->m_anim_velocity.length ( );

	// if on ground, moving, and not fakewalking.
	if ( ( record->m_flags & FL_ONGROUND ) && speed > 0.1f && !record->m_fake_walk )
		record->m_mode = Modes::RESOLVE_WALK;

	// if on ground, not moving or fakewalking.
	if ( ( record->m_flags & FL_ONGROUND ) && ( speed <= 0.1f || record->m_fake_walk ) )
		record->m_mode = Modes::RESOLVE_STAND;

	// if not on ground.
	else if ( !( record->m_flags & FL_ONGROUND ) )
		record->m_mode = Modes::RESOLVE_AIR;
}

void Resolver::AntiFreestand ( Player *player, AimPlayer *data, LagRecord *record ) {
	constexpr float STEP { 2.f };
	constexpr float RANGE { 32.f };

	// best target.
	struct AutoTarget_t { float fov; Player *player; };
	AutoTarget_t target { 180.f + 1.f, nullptr };

	float away = GetAwayAngle ( record );

	std::vector< AdaptiveAngle > angles { };
	angles.emplace_back ( away - 180.f );
	angles.emplace_back ( away + 90.f );
	angles.emplace_back ( away - 90.f );

	// start the trace at the enemy shoot pos.
	vec3_t start = g_cl.m_local->GetShootPosition ( );
	vec3_t player_end = record->m_player->GetShootPosition ( );

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid { false };

	// iterate vector of angles.
	for ( auto it = angles.begin ( ); it != angles.end ( ); ++it ) {
		// compute the 'rough' estimation of where our head will be.
		vec3_t end { player_end.x + std::cos ( math::deg_to_rad ( it->m_yaw ) ) * RANGE,
			player_end.y + std::sin ( math::deg_to_rad ( it->m_yaw ) ) * RANGE,
			player_end.z };

		// draw a line for debugging purposes.
		g_csgo.m_debug_overlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.1f );

		// compute the direction.
		vec3_t dir = end - start;
		float len = dir.normalize ( );

		// should never happen.
		if ( len <= 0.f )
			continue;

		// step thru the total distance, 4 units per step.
		for ( float i { 0.f }; i < len; i += STEP ) {
			// get the current step position.
			vec3_t point = start + ( dir * i );

			// get the contents at this point.
			int contents = g_csgo.m_engine_trace->GetPointContents ( point, MASK_SHOT_HULL );

			// contains nothing that can stop a bullet.
			if ( !( contents & MASK_SHOT_HULL ) )
				continue;

			float mult = 1.f;

			// over 50% of the total length, prioritize this shit.
			if ( i > ( len * 0.5f ) )
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if ( i > ( len * 0.75f ) )
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if ( i > ( len * 0.9f ) )
				mult = 2.f;

			// append 'penetrated distance'.
			it->m_dist += ( STEP * mult );

			// mark that we found anything.
			valid = true;
		}
	}

	if ( !valid ) {
		// set angle to backwards.
		//m_auto = math::NormalizedAngle ( record->m_eye_angles.y - 180.f );
		return;
	}

	// put the most distance at the front of the container.
	std::sort ( angles.begin ( ), angles.end ( ),
		[ ] ( const AdaptiveAngle &a, const AdaptiveAngle &b ) {
			return a.m_dist > b.m_dist;
		} );

	// the best angle should be at the front now.
	AdaptiveAngle *best = &angles.front ( );

	if ( best->m_dist != record->m_freestand_dist ) {
		// set yaw to the best result.
		record->m_eye_angles.y = best->m_yaw;
		record->m_freestand_dist = best->m_dist;
	}
}

void Resolver::ResolveAngles ( Player *player, LagRecord *record ) {
	AimPlayer *data = &g_aimbot.m_players [ player->index ( ) - 1 ];

	auto game_state = ( CCSGOGamePlayerAnimState * ) player->m_PlayerAnimState ( );

	if ( record ) {
		for ( int i = 0; i < player->m_iNumOverlays ( ); i++ ) {
			auto layer = player->m_AnimOverlay ( ) [ i ];

			if ( record->m_triggered_balence_adjust = player->GetSequenceActivity ( layer.m_sequence ) == ACT_CSGO_IDLE_TURN_BALANCEADJUST && record->m_body >= data->m_old_body ) {
				record->m_flick = ( record->m_anim_time >= ( data->m_body_update ) ) ? true : false;

				//if ( record->valid ( ) )
				record->m_tick_flicked = game::TIME_TO_TICKS ( record->m_sim_time );
			}
		}
	}

	// mark this record if it contains a shot.
	MatchShot ( data, record );

	// next up mark this record with a resolver mode that will be used.
	SetMode ( record );

	// if we are in nospread mode, force all players pitches to down.
	// TODO; we should check thei actual pitch and up too, since those are the other 2 possible angles.
	// this should be somehow combined into some iteration that matches with the air angle iteration.
	if ( g_menu.main.config.mode.get ( ) == 1 )
		record->m_eye_angles.x = 90.f;


	AntiFreestand ( player, data, record );

	// we arrived here we can do the acutal resolve.
	if ( record->m_mode == Modes::RESOLVE_WALK )
		ResolveWalk ( data, record );

	else if ( record->m_mode == Modes::RESOLVE_STAND )
		ResolveStand ( data, record );

	else if ( record->m_mode == Modes::RESOLVE_AIR )
		ResolveAir ( data, record );

	// normalize the eye angles, doesn't really matter but its clean.
	math::NormalizeAngle ( record->m_eye_angles.y );
}

void Resolver::ResolveWalk ( AimPlayer *data, LagRecord *record ) {
	// apply lby to eyeangles.
	record->m_eye_angles.y = record->m_body;

	// delay body update.
	data->m_body_update = record->m_anim_time + 0.22f;

	// reset stand and body index.
	data->m_stand_index = 0;
	data->m_stand_index2 = 0;
	data->m_body_index = 0;

	// copy the last record that this player was walking
	// we need it later on because it gives us crucial data.
	std::memcpy ( &data->m_walk_record, record, sizeof ( LagRecord ) );
}

void Resolver::ResolveStand ( AimPlayer *data, LagRecord *record ) {	
	// for no-spread call a seperate resolver.
	if ( g_menu.main.config.mode.get ( ) == 1 ) {
		StandNS ( data, record );
		return;
	}

	// get predicted away angle for the player.
	float away = GetAwayAngle ( record );

	// pointer for easy access.
	LagRecord *move = &data->m_walk_record;

	// we have a valid moving record.
	if ( move->m_sim_time > 0.f ) {
		vec3_t delta = move->m_origin - record->m_origin;

		// check if moving record is close.
		if ( delta.length ( ) <= 128.f ) {
			// indicate that we are using the moving lby.
			data->m_moved = true;
		}
	}

	//if ( record->m_flick )
	//	record->m_eye_angles.y = move->m_body;

	// a valid moving context was found
	if ( data->m_moved ) {
		float diff = math::NormalizedAngle ( record->m_body - move->m_body );
		float delta = record->m_anim_time - move->m_anim_time;

		record->m_eye_angles.y = move->m_body;

		// it has not been time for this first update yet.
		if ( delta < 0.22f ) {
			// set angles to current LBY.

			// set resolve mode.
			record->m_mode = Modes::RESOLVE_STOPPED_MOVING;

			// exit out of the resolver, thats it.
			return;
		}

		// LBY SHOULD HAVE UPDATED HERE.
		else if ( record->m_anim_time >= data->m_body_update ) {
			// only shoot the LBY flick 3 times.
			// if we happen to miss then we most likely mispredicted.
			if ( data->m_body_index <= 3 ) {
				// set angles to current LBY.
				record->m_eye_angles.y = record->m_body;

				// predict next body update.
				data->m_body_update = record->m_anim_time + 1.1f;

				// set the resolve mode.
				record->m_mode = Modes::RESOLVE_BODY;

				return;
			}

			// set to stand1 -> known last move.
			record->m_mode = Modes::RESOLVE_STAND1;

			C_AnimationLayer *curr = &record->m_layers [ 3 ];
			int act = data->m_player->GetSequenceActivity ( curr->m_sequence );

			// ok, no fucking update. apply big resolver.
			record->m_eye_angles.y = move->m_body;

			// every third shot do some fuckery.
			if ( !( data->m_stand_index % 3 ) )
				record->m_eye_angles.y += g_csgo.RandomFloat ( -35.f, 35.f );

			// jesus we can fucking stop missing can we?
			if ( data->m_stand_index > 6 && act != 980 ) {
				// lets just hope they switched ang after move.
				record->m_eye_angles.y = move->m_body + 180.f;
			}

			// we missed 4 shots.
			else if ( data->m_stand_index > 4 && act != 980 ) {
				// try backwards.
				record->m_eye_angles.y = away + 180.f;
			}

			return;
		}
	}

	// stand2 -> no known last move.
	record->m_mode = Modes::RESOLVE_STAND2;

	switch ( data->m_stand_index2 % 6 ) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = record->m_body;
		break;

	case 2:
		record->m_eye_angles.y = record->m_body + 180.f;
		break;

	case 3:
		record->m_eye_angles.y = record->m_body + 110.f;
		break;

	case 4:
		record->m_eye_angles.y = record->m_body - 110.f;
		break;

	case 5:
		record->m_eye_angles.y = away;
		break;

	default:
		break;
	}
}

void Resolver::StandNS ( AimPlayer *data, LagRecord *record ) {
	// get away angles.
	float away = GetAwayAngle ( record );

	switch ( data->m_shots % 8 ) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = away + 90.f;
		break;
	case 2:
		record->m_eye_angles.y = away - 90.f;
		break;

	case 3:
		record->m_eye_angles.y = away + 45.f;
		break;
	case 4:
		record->m_eye_angles.y = away - 45.f;
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;
	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;

	case 7:
		record->m_eye_angles.y = away + 0.f;
		break;

	default:
		break;
	}

	// force LBY to not fuck any pose and do a true bruteforce.
	record->m_body = record->m_eye_angles.y;
}

void Resolver::ResolveAir ( AimPlayer *data, LagRecord *record ) {
	// for no-spread call a seperate resolver.
	if ( g_menu.main.config.mode.get ( ) == 1 ) {
		AirNS ( data, record );
		return;
	}

	// else run our matchmaking air resolver.

	// we have barely any speed.
	// either we jumped in place or we just left the ground.
	// or someone is trying to fool our resolver.
	if ( record->m_velocity.length_2d ( ) < 60.f ) {
		// set this for completion.
		// so the shot parsing wont pick the hits / misses up.
		// and process them wrongly.
		record->m_mode = Modes::RESOLVE_STAND;

		// invoke our stand resolver.
		ResolveStand ( data, record );

		// we are done.
		return;
	}

	// try to predict the direction of the player based on his velocity direction.
	// this should be a rough estimation of where he is looking.
	float velyaw = math::rad_to_deg ( std::atan2 ( record->m_velocity.y, record->m_velocity.x ) );

	switch ( data->m_shots % 3 ) {
	case 0:
		record->m_eye_angles.y = velyaw + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = velyaw - 90.f;
		break;

	case 2:
		record->m_eye_angles.y = velyaw + 90.f;
		break;
	}
}

void Resolver::AirNS ( AimPlayer *data, LagRecord *record ) {
	// get away angles.
	float away = GetAwayAngle ( record );

	switch ( data->m_shots % 9 ) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = away + 150.f;
		break;
	case 2:
		record->m_eye_angles.y = away - 150.f;
		break;

	case 3:
		record->m_eye_angles.y = away + 165.f;
		break;
	case 4:
		record->m_eye_angles.y = away - 165.f;
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;
	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;

	case 7:
		record->m_eye_angles.y = away + 90.f;
		break;
	case 8:
		record->m_eye_angles.y = away - 90.f;
		break;

	default:
		break;
	}
}

void Resolver::ResolvePoses ( Player *player, LagRecord *record ) {
	AimPlayer *data = &g_aimbot.m_players [ player->index ( ) - 1 ];

	//// only do this bs when in air.
	//if ( record->m_mode == Modes::RESOLVE_AIR ) {
	//	// ang = pose min + pose val x ( pose range )

	//	// lean_yaw
	//	player->m_flPoseParameter ( ) [ 2 ] = g_csgo.RandomInt ( 0, 4 ) * 0.25f;

	//	// body_yaw
	//	player->m_flPoseParameter ( ) [ 11 ] = g_csgo.RandomInt ( 1, 3 ) * 0.25f;
	//}
}