#include "includes.h"

void Hooks::LevelInitPreEntity ( const char *map ) {
	float rate { 1.f / g_csgo.m_globals->m_interval };

	// set rates when joining a server.
	g_csgo.cl_updaterate->SetValue ( rate );
	g_csgo.cl_cmdrate->SetValue ( rate );

	g_aimbot.reset ( );
	g_visuals.m_hit_start = g_visuals.m_hit_end = g_visuals.m_hit_duration = 0.f;

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPreEntity_t > ( CHLClient::LEVELINITPREENTITY )( this, map );
}

void Hooks::LevelInitPostEntity ( ) {
	g_cl.OnMapload ( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPostEntity_t > ( CHLClient::LEVELINITPOSTENTITY )( this );
}

void Hooks::LevelShutdown ( ) {
	g_aimbot.reset ( );

	g_cl.m_local = nullptr;
	g_cl.m_weapon = nullptr;
	//g_cl.m_processing = false;
	g_cl.m_weapon_info = nullptr;
	g_cl.m_round_end = false;

	g_cl.m_sequences.clear ( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelShutdown_t > ( CHLClient::LEVELSHUTDOWN )( this );
}

/*int Hooks::IN_KeyEvent( int evt, int key, const char* bind ) {
	// see if this key event was fired for the drop bind.
	/*if( bind && FNV1a::get( bind ) == HASH( "drop" ) ) {
		// down.
		if( evt ) {
			g_cl.m_drop = true;
			g_cl.m_drop_query = 2;
			g_cl.print( "drop\n" );
		}

		// up.
		else
			g_cl.m_drop = false;

		// ignore the event.
		return 0;
	}

	return g_hooks.m_client.GetOldMethod< IN_KeyEvent_t >( CHLClient::INKEYEVENT )( this, evt, key, bind );
}*/

void Hooks::FrameStageNotify ( Stage_t stage ) {
	// save stage.
	if ( stage != FRAME_START )
		g_cl.m_stage = stage;

	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player * > ( g_csgo.m_engine->GetLocalPlayer ( ) );

	if ( stage == FRAME_RENDER_START ) {
		if ( g_cl.m_local && g_cl.m_local->alive ( ) ) {
			g_cl.RestoreLocalData ( );
		}

		// draw our custom beams.
		g_visuals.DrawBeams ( );
	}

	if ( stage == FRAME_NET_UPDATE_END ) {

		const auto viewmodel = g_csgo.m_entlist->GetClientEntityFromHandle< ViewModel * > ( g_cl.m_local->m_hViewModel ( ) );

		if ( g_cl.m_local && g_cl.m_local->m_hViewModel ( ) != 0xFFFFFFF ) {
			// restore viewmodel when model renders a scene
			viewmodel->m_flCycle ( ) = g_inputpred.m_weapon_cycle;
			viewmodel->m_nSequence ( ) = g_inputpred.m_weapon_sequence;
			viewmodel->m_flAnimTime ( ) = g_inputpred.m_weapon_anim_time;
		}
	}
		
	if ( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END && ( g_cl.m_local && g_cl.m_local->alive ( ) ) ) {
		g_inputpred.stored.m_old_velocity_modifier = g_cl.m_local->m_flVelocityModifier ( );

		if ( g_cl.m_local->m_flVelocityModifier ( ) < g_inputpred.stored.m_old_velocity_modifier ) {
			g_inputpred.stored.m_velocity_modifier = g_inputpred.stored.m_old_velocity_modifier;

			// force games prediction to update
			g_inputpred.ForceUpdate ( true );
		}
	}

	// call og.
	g_hooks.m_client.GetOldMethod< FrameStageNotify_t > ( CHLClient::FRAMESTAGENOTIFY )( this, stage );

	if ( stage == FRAME_RENDER_START ) {
#ifdef _DEBUG
		// server hitboxes.
		if ( g_menu.main.players.show_server_boxes.get ( ) ) {
			float fDuration = -1.f;

			static auto DrawServerHitboxes = pattern::find ( g_csgo.m_server_dll, XOR ( "E8 ? ? ? ? F6 83 ? ? ? ? ? 0F 84 ? ? ? ? 33 FF 39 BB" ) ).rel32 ( 0x1 ).as < void * > ( );
			static auto UTIL_PlayerByIndex = pattern::find ( g_csgo.m_server_dll, XOR ( "85 C9 7E 2A A1" ) ).as < void *( __fastcall* ) ( int ) > ( );

			for ( int i = 1; i < g_csgo.m_globals->m_max_clients; i++ ) {
				auto e = g_csgo.m_entlist->GetClientEntity ( i );

				if ( !e )
					continue;

				auto ent = ( Player * ) UTIL_PlayerByIndex ( e->index ( ) );

				if ( !ent )
					continue;

				__asm {
					pushad
					movss xmm1, fDuration
					push 1 //bool monoColor
					mov ecx, ent
					call DrawServerHitboxes
					popad
				}
			}
		}
#endif
	}

	else if ( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START ) {
		// restore non-compressed netvars.
		// g_netdata.apply( );
		g_skins.think ( );
	}

	else if ( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END ) {
		g_visuals.NoSmoke ( );
	}

	else if ( stage == FRAME_NET_UPDATE_END ) {
		// restore non-compressed netvars.
		g_netdata.apply ( );

		// store layers before our player modifies our data.
		if ( g_cl.m_local && g_cl.m_local->m_AnimOverlay ( ) ) {
			memcpy ( g_cl.anim_data.m_last_queued_layers, g_cl.m_local->m_AnimOverlay ( ), sizeof ( C_AnimationLayer ) * 13 );
		}

		// null out layers
		else if ( !g_cl.m_local ) {
			memset ( g_cl.anim_data.m_last_queued_layers, 0, sizeof ( C_AnimationLayer ) * 13 );
		}

		// copy back layers just in-case if we have our shit fucked up.
		else if ( !g_cl.m_local->alive ( ) ) {
			memcpy ( g_cl.anim_data.m_last_queued_layers, g_cl.m_local->m_AnimOverlay ( ), sizeof ( C_AnimationLayer ) * 13 );
		}

		// update all players.
		for ( int i { 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
			Player *player = g_csgo.m_entlist->GetClientEntity< Player * > ( i );
			if ( !player || player->m_bIsLocalPlayer ( ) )
				continue;

			AimPlayer *data = &g_aimbot.m_players [ i - 1 ];
			data->OnNetUpdate ( player );
		}
	}
}