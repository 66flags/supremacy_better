#include "includes.h"

bool Hooks::IsConnected( ) {
	Stack stack;

	static Address IsLoadoutAllowed{ pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 04 B0 01 5F" ) ) };

	if( g_menu.main.misc.unlock.get( ) && stack.ReturnAddress( ) == IsLoadoutAllowed )
		return false;

	return g_hooks.m_engine.GetOldMethod< IsConnected_t >( IVEngineClient::ISCONNECTED )( this );
}

bool Hooks::IsHLTV( ) {
	Stack stack;

	static Address SetupVelocity{ pattern::find( g_csgo.m_client_dll, XOR( "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80" ) ) };

	// AccumulateLayers
	if( g_bones.m_running )
		return true;

	// fix for animstate velocity.
	if( stack.ReturnAddress( ) == SetupVelocity )
		return true;

	static Address AccumulateLayers { pattern::find ( g_csgo.m_client_dll, XOR ( "84 C0 75 0D F6 87" ) ) };

	// PVS fix
	if ( stack.ReturnAddress ( ) == AccumulateLayers )
		return true;

	return g_hooks.m_engine.GetOldMethod< IsHLTV_t >( IVEngineClient::ISHLTV )( this );
}

float Hooks::GetScreenAspectRatio ( int viewportWidth, int viewportHeight ) {
	auto ratio = g_menu.main.visuals.aspect_ratio_value.get ( );
	auto ret = g_hooks.m_engine.GetOldMethod< GetScreenAspectRatio_t > ( IVEngineClient::GETSCREENASPECTRATIO )( this, viewportWidth, viewportHeight );

	if ( !( ratio > 0 ) || !g_menu.main.visuals.override_aspect_ratio.get ( ) )
		return ret;

	return ratio;
}

void Hooks::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char* pSoundEntry, unsigned int nSoundEntryHash, const char* pSample, float flVolume, float flAttenuation, int nSeed, int iFlags, int iPitch, const vec3_t* pOrigin, const vec3_t* pDirection, void* pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity ) {
	g_hooks.m_engine_sound.GetOldMethod<EmitSound_t>( IEngineSound::EMITSOUND )( this, filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample, flVolume, flAttenuation, nSeed, iFlags, iPitch, pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity );
}