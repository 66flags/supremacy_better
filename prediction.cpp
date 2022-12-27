#include "includes.h"

bool Hooks::InPrediction( ) {
	Stack stack;
	ang_t *angles;

	// note - dex; first 2 'test al, al' instructions in C_BasePlayer::CalcPlayerView.
	static Address CalcPlayerView_ret1{ pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 0B 8B 0D ? ? ? ? 8B 01 FF 50 4C" ) ) };
	static Address CalcPlayerView_ret2{ pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 08 57 8B CE E8 ? ? ? ? 8B 06" ) ) };

	if( g_cl.m_local && g_menu.main.visuals.novisrecoil.get( ) ) {
		// note - dex; apparently this calls 'view->DriftPitch()'.
		//             i don't know if this function is crucial for normal gameplay, if it causes issues then comment it out.
		if( stack.ReturnAddress( ) == CalcPlayerView_ret1 )
			return true;

		if( stack.ReturnAddress( ) == CalcPlayerView_ret2 ) {
			// at this point, angles are copied into the CalcPlayerView's eyeAngles argument.
			// (ebp) InPrediction -> (ebp) CalcPlayerView + 0xC = eyeAngles.
			angles = stack.next( ).arg( 0xC ).to< ang_t* >( );

			if( angles ) {
				*angles -= g_cl.m_local->m_viewPunchAngle( )
					     + ( g_cl.m_local->m_aimPunchAngle( ) * g_csgo.weapon_recoil_scale->GetFloat( ) )
					     * g_csgo.view_recoil_tracking->GetFloat( );
			}

			return true;
		}
	}

	return g_hooks.m_prediction.GetOldMethod< InPrediction_t >( CPrediction::INPREDICTION )( this );
}

int old_num = 0;

void Hooks::RunCommand( Entity* ent, CUserCmd* cmd, IMoveHelper* movehelper ) {
	// airstuck jitter / overpred fix.
	if( cmd->m_tick >= std::numeric_limits< int >::max( ) )
		return;
	
	if ( g_cl.m_local ) {
		if ( !g_csgo.m_cl->m_choked_commands ) {
			auto nci = g_csgo.m_engine->GetNetChannelInfo ( );

			if ( nci && g_cl.m_cmd->m_command_number > old_num ) {
				const auto latency = game::TIME_TO_TICKS ( nci->GetLatency ( 1 ) );

				if ( g_inputpred.stored.m_velocity_modifier < 1.0f )
					g_inputpred.stored.m_velocity_modifier = std::clamp < float > ( g_inputpred.stored.m_velocity_modifier + ( game::TICKS_TO_TIME ( 1 ) + latency ) * ( 1.0f / 2.5f ), 0.0f, 1.0f );

				old_num = g_cl.m_cmd->m_command_number;
			}

			if ( !( g_cl.m_local->m_fFlags ( ) & FL_ONGROUND ) )
				g_cl.m_local->m_flVelocityModifier ( ) = g_inputpred.stored.m_velocity_modifier;
		}
	}

	//g_inputpred.FixViewmodel ( false );
	g_hooks.m_prediction.GetOldMethod< RunCommand_t >( CPrediction::RUNCOMMAND )( this, ent, cmd, movehelper );
	g_netdata.store ( );

	if ( ent )
		*( int* )( std::uintptr_t ( ent ) + 0x3238 ) = 0;

	//if ( g_cl.m_cmd->m_command_number > old_num ) {

	//	old_num = g_cl.m_cmd->m_command_number;
	//}
}