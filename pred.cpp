#include "includes.h"
#include "pred.h"

InputPrediction g_inputpred{};;

void InputPrediction::update( ) {
	bool        valid{ g_csgo.m_cl->m_delta_tick > 0 };
	//int         outgoing_command, current_command;
	//CUserCmd    *cmd;

	// render start was not called.
	if( g_cl.m_stage == FRAME_NET_UPDATE_END ) {
		/*outgoing_command = g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands;

		// this must be done before update ( update will mark the unpredicted commands as predicted ).
		for( int i{}; ; ++i ) {
			current_command = g_csgo.m_cl->m_last_command_ack + i;

			// caught up / invalid.
			if( current_command > outgoing_command || i >= MULTIPLAYER_BACKUP )
				break;

			// get command.
			cmd = g_csgo.m_input->GetUserCmd( current_command );
			if( !cmd )
				break;

			// cmd hasn't been predicted.
			// m_nTickBase is incremented inside RunCommand ( which is called frame by frame, we are running tick by tick here ) and prediction hasn't run yet,
			// so we must fix tickbase by incrementing it ourselves on non-predicted commands.
			if( !cmd->m_predicted )
				++g_cl.m_local->m_nTickBase( );
		}*/

		// EDIT; from what ive seen RunCommand is called when u call Prediction::Update
		// so the above code is not fucking needed.

		int start = g_csgo.m_cl->m_last_command_ack;
		int stop  = g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands;

		// call CPrediction::Update.
		//g_csgo.m_prediction->Update( g_csgo.m_cl->m_delta_tick, valid, start, stop );
	}

	static bool unlocked_fakelag = false;
	if( !unlocked_fakelag ) {
		auto cl_move_clamp = pattern::find( g_csgo.m_engine_dll, XOR("B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC") ) + 1;
		unsigned long protect = 0;

		VirtualProtect( ( void * )cl_move_clamp, 4, PAGE_EXECUTE_READWRITE, &protect );
		*( std::uint32_t * )cl_move_clamp = 62;
		VirtualProtect( ( void * )cl_move_clamp, 4, protect, &protect );
		unlocked_fakelag = true;
	}
}

void InputPrediction::ForceUpdate ( bool error ) {
	if ( !g_cl.m_local || !g_csgo.m_prediction || !g_csgo.m_move_helper )
		return;

	if ( error ) {
		//g_csgo.m_prediction->m_prev_ack_had_errors = 1;
		//g_csgo.m_prediction->m_cmds_predicted = 0;
		*( int * ) ( std::uintptr_t ( g_csgo.m_prediction ) + 0x24 ) = 1;
		*( int * ) ( std::uintptr_t ( g_csgo.m_prediction ) + 0x1C ) = 0;
	}

	if ( g_csgo.m_cl->m_delta_tick > 0 && error )
		m_has_error = false;

	g_csgo.m_prediction->Update (
		g_csgo.m_cl->m_delta_tick,
		g_csgo.m_cl->m_delta_tick > 0,
		g_csgo.m_cl->m_last_command_ack,
		g_csgo.m_cl->m_last_outgoing_command + g_csgo.m_cl->m_choked_commands );
}

CMoveData data {};

void PostThink ( Player *ent ) {
	g_csgo.m_model_cache->BeginLock ( );

	static auto post_think_vphysics = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 8B D9 56 57 83 BB" ) ).as< bool ( __thiscall * )( Player * ) > ( );
	static auto simulate_player_simulated_entities = pattern::find ( g_csgo.m_client_dll, XOR ( "56 8B F1 57 8B BE ? ? ? ? 83 EF 01 78 72 90 8B 86" ) ).as< void ( __thiscall * )( void * ) > ( );

	if ( ent->alive ( ) ) {
		util::get_method< void ( __thiscall * )( void * ) > ( ent, 329 ) ( ent );

		if ( ent->m_fFlags ( ) & FL_ONGROUND )
			ent->m_flFallVelocity ( ) = 0.f;

		if ( ent->m_nSequence ( ) == -1 )
			ent->SetSequence ( 0 );

		util::get_method< void ( __thiscall * )( void * ) > ( ent, 214 ) ( ent );
		post_think_vphysics ( ent );
	}

	simulate_player_simulated_entities ( ent );
	g_csgo.m_model_cache->EndLock ( );
}

void InputPrediction::run( ) {
	if ( !g_cl.m_local || !g_cl.m_cmd )
		return;

	g_cl.m_local->m_pCurrentCommand( ) = g_cl.m_cmd;
	g_cl.m_local->m_PlayerCommand( )   = *g_cl.m_cmd;

	*g_csgo.m_nPredictionRandomSeed = g_cl.m_cmd->m_random_seed;
	g_csgo.m_pPredictionPlayer      = g_cl.m_local;

	m_curtime   = g_csgo.m_globals->m_curtime;
	m_frametime = g_csgo.m_globals->m_frametime;

	g_csgo.m_globals->m_curtime = g_cl.m_local->m_nTickBase ( ) * g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_frametime = g_csgo.m_prediction->m_engine_paused ? 0 : g_csgo.m_globals->m_interval;

	m_first_time_predicted = g_csgo.m_prediction->m_first_time_predicted;
	m_in_prediction = g_csgo.m_prediction->m_in_prediction;

	g_csgo.m_prediction->m_first_time_predicted = false;
	g_csgo.m_prediction->m_in_prediction = true;

	g_cl.m_cmd->m_buttons |= g_cl.m_local->m_afButtonForced ( );
	//g_cl.m_cmd->m_buttons &= ~g_cl.m_local->ButtonDisabled ( );

	g_csgo.m_move_helper->SetHost ( g_cl.m_local );
	g_csgo.m_game_movement->StartTrackPredictionErrors ( g_cl.m_local );

	if ( g_cl.m_cmd->m_weapon_select != 0 ) {
		Weapon *weapon = g_cl.m_local->GetActiveWeapon ( );

		if ( weapon ) {
			auto data = weapon->GetWpnData ( );

			if ( data )
				g_cl.m_local->SelectItem ( data->m_weapon_name, g_cl.m_cmd->m_weapon_subtype );
		}
	}

	const auto buttons_changed = g_cl.m_cmd->m_buttons ^ g_cl.m_local->m_afButtonLast ( );

	g_cl.m_local->m_nButtons ( ) = g_cl.m_local->m_afButtonLast ( );
	g_cl.m_local->m_afButtonLast ( ) = g_cl.m_cmd->m_buttons;
	g_cl.m_local->m_afButtonPressed ( ) = g_cl.m_cmd->m_buttons & buttons_changed;
	g_cl.m_local->m_afButtonReleased ( ) = buttons_changed & ~g_cl.m_cmd->m_buttons;

	g_csgo.m_prediction->CheckMovingGround ( g_cl.m_local, g_csgo.m_globals->m_frametime );
	g_csgo.m_prediction->SetLocalViewAngles ( g_cl.m_cmd->m_view_angles );

	if ( g_cl.m_local->PhysicsRunThink ( 0 ) )
		g_cl.m_local->PreThink ( );

	const auto think_tick = g_cl.m_local->GetThinkTick ( );

	if ( think_tick > 0 && think_tick <= g_cl.m_local->m_nTickBase ( ) ) {
		g_cl.m_local->GetThinkTick ( ) = -1;
		static auto set_next_think = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B C6 C1 E8 16 24 01 74 18" ) ).as< void ( __thiscall * )( void *, int ) > ( );
		set_next_think ( g_cl.m_local, 0 );
		//g_cl.m_local->Think ( );
	}

	g_csgo.m_prediction->SetupMove ( g_cl.m_local, g_cl.m_cmd, g_csgo.m_move_helper, &data );
	g_csgo.m_game_movement->ProcessMovement ( g_cl.m_local, &data );

	g_csgo.m_prediction->FinishMove ( g_cl.m_local, g_cl.m_cmd, &data );
	g_csgo.m_move_helper->ProcessImpacts ( );

	PostThink ( g_cl.m_local );

	g_csgo.m_prediction->m_first_time_predicted = m_first_time_predicted;
	g_csgo.m_prediction->m_in_prediction = m_in_prediction;
}

void InputPrediction::restore ( ) {
	if ( !g_cl.m_local || !g_cl.m_cmd || !g_csgo.m_move_helper )
		return;

	g_csgo.m_game_movement->FinishTrackPredictionErrors ( g_cl.m_local );
	g_csgo.m_move_helper->SetHost ( nullptr );

	g_cl.m_local->m_pCurrentCommand ( ) = nullptr;

	*g_csgo.m_nPredictionRandomSeed = -1;
	g_csgo.m_pPredictionPlayer      = nullptr;

	g_csgo.m_game_movement->Reset ( );

	g_csgo.m_globals->m_curtime = m_curtime;
	g_csgo.m_globals->m_frametime = m_frametime;
	g_csgo.m_prediction->m_in_prediction = m_in_prediction;
	g_csgo.m_prediction->m_first_time_predicted = m_first_time_predicted;
}