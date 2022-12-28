#include "includes.h"

bool Hooks::TempEntities( void *msg ) {
	if( !g_cl.m_local->alive ( ) ) {
		return g_hooks.m_client_state.GetOldMethod< TempEntities_t >( CClientState::TEMPENTITIES )( this, msg );
	}

	const bool ret = g_hooks.m_client_state.GetOldMethod< TempEntities_t >( CClientState::TEMPENTITIES )( this, msg );

	return ret;
}