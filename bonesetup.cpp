#include "includes.h"

Bones g_bones{};;

bool Bones::setup( Player* player, BoneArray* out, float curtime, LagRecord* record ) {
	if ( !record->m_setup ) {
		const auto cur_time = g_csgo.m_globals->m_curtime;
		const auto frame_time = g_csgo.m_globals->m_frametime;
		const auto abs_frame_time = g_csgo.m_globals->m_abs_frametime;
		const auto interval_per_tick = curtime / ( g_csgo.m_globals->m_interval + 0.5f );
		const auto frame_count = g_csgo.m_globals->m_frame;
		const auto tick_count = g_csgo.m_globals->m_tick_count;

		g_csgo.m_globals->m_curtime = curtime;
		g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;
		g_csgo.m_globals->m_abs_frametime = g_csgo.m_globals->m_interval;
		g_csgo.m_globals->m_frame = interval_per_tick;
		g_csgo.m_globals->m_tick_count = interval_per_tick;

		const auto backup_effects = player->m_fEffects ( );
		const auto last_setup_time = player->m_flLastBoneSetupTime ( );

		player->m_fEffects ( ) |= EF_NOINTERP;
		player->m_flLastBoneSetupTime ( ) = 0;
		player->InvalidateBoneCache ( );

		BuildBones ( player, 0x7FF00, record->m_bones, record );

		player->m_fEffects ( ) = backup_effects;
		player->m_flLastBoneSetupTime ( ) = last_setup_time;

		g_csgo.m_globals->m_curtime = cur_time;
		g_csgo.m_globals->m_frametime = frame_time;
		g_csgo.m_globals->m_abs_frametime = abs_frame_time;
		g_csgo.m_globals->m_frame = frame_count;
		g_csgo.m_globals->m_tick_count = tick_count;

		// we have setup this record bones.
		record->m_setup = true;
	}

	// record is setup.
	if ( out && record->m_setup )
		std::memcpy ( out, record->m_bones, sizeof ( BoneArray ) * 128 );

	return true;
}

bool Bones::BuildBones( Player* target, int mask, BoneArray* out, LagRecord* record ) {
	vec3_t           backup_origin;
	ang_t            backup_angles;
	float            backup_poses[ 24 ];
	C_AnimationLayer backup_layers[ 13 ];

	// prevent the game from calling ShouldSkipAnimationFrame.
	//auto bSkipAnimationFrame = *reinterpret_cast< int* >( uintptr_t( target ) + 0x260 );
	//*reinterpret_cast< int* >( uintptr_t( target ) + 0x260 ) = NULL;

	// backup original.
	backup_origin  = target->GetAbsOrigin( );
	backup_angles  = target->GetAbsAngles( );
	target->GetPoseParameters( backup_poses );
	target->GetAnimLayers( backup_layers );

	// compute transform from raw data.
	matrix3x4_t transform;
	math::AngleMatrix( record->m_abs_ang, record->m_pred_origin, transform );

	// set non interpolated data.
	target->SetAbsOrigin( record->m_pred_origin );
	target->SetAbsAngles( record->m_abs_ang );
	target->SetPoseParameters( record->m_poses );
	target->SetAnimLayers( record->m_layers );

	// force game to call AccumulateLayers - pvs fix.
	m_running = true;

	target->SetupBones ( out, 128, mask, g_csgo.m_globals->m_curtime );

	// revert to old game behavior.
	m_running = false;

	// allow the game to call ShouldSkipAnimationFrame.
	//*reinterpret_cast< int* >( uintptr_t( target ) + 0x260 ) = bSkipAnimationFrame;

	return true;
}