#include "includes.h"

void Hooks::DoExtraBoneProcessing( int a2, int a3, int a4, int a5, int a6, int a7 ) {
	// call og.
	g_hooks.m_DoExtraBoneProcessing( this, a2, a3, a4, a5, a6, a7 );
}

void Hooks::BuildTransformations( int a2, int a3, int a4, int a5, int a6, int a7 ) {
	// cast thisptr to player ptr.
	Player* player = ( Player* )this;

	// get bone jiggle.
	int bone_jiggle = *reinterpret_cast< int* >( uintptr_t( player ) + 0x291C );

	// null bone jiggle to prevent attachments from jiggling around.
	*reinterpret_cast< int* >( uintptr_t( player ) + 0x291C ) = 0;

	// call og.
	g_hooks.m_BuildTransformations( this, a2, a3, a4, a5, a6, a7 );

	// restore bone jiggle.
	*reinterpret_cast< int* >( uintptr_t( player ) + 0x291C ) = bone_jiggle;
}

void __fastcall Hooks::CalcView ( void *ecx, void *edx, vec3_t &eye_origin, ang_t &eye_angles, float &z_near, float &z_far, float &fov ) {
	const auto player = reinterpret_cast< Player * >( ecx );

	if ( player != g_cl.m_local )
		return g_hooks.m_CalcView ( ecx, edx, eye_origin, eye_angles, z_near, z_far, fov );

	const auto old_use_new_animation_state = *( bool * ) ( std::uintptr_t ( player ) + 0x39E1 );

	// prevent calls to ModifyEyePosition
	*( bool * ) ( std::uintptr_t ( player ) + 0x39E1 ) = false; // 0x39E1

	g_hooks.m_CalcView ( ecx, edx, eye_origin, eye_angles, z_near, z_far, fov );

	*( bool * ) ( std::uintptr_t ( player ) + 0x39E1 ) = old_use_new_animation_state;
}

void Hooks::UpdateClientSideAnimation( ) {
	if( g_cl.m_processing )
		g_cl.SetAngles( );
	else {
		g_hooks.m_UpdateClientSideAnimation( this );
	}
}

Weapon *Hooks::GetActiveWeapon( ) {
    Stack stack;

    static Address ret_1 = pattern::find( g_csgo.m_client_dll, XOR( "85 C0 74 1D 8B 88 ? ? ? ? 85 C9" ) );

    // note - dex; stop call to CIronSightController::RenderScopeEffect inside CViewRender::RenderView.
    if( g_menu.main.visuals.noscope.get( ) ) {
        if( stack.ReturnAddress( ) == ret_1 )
            return nullptr;
    }

    return g_hooks.m_GetActiveWeapon( this );
}

void CustomEntityListener::OnEntityCreated( Entity *ent ) {
	const auto CalculateView_idx = pattern::find ( g_csgo.m_client_dll, XOR ( "FF 90 ? ? ? ? 8B 0D ? ? ? ? 8B 01 8B 80 ? ? ? ? FF D0 84 C0" ) ).as < std::size_t > ( ) / 4;

    if( ent ) {
        // player created.
        if( ent->IsPlayer( ) ) {
		    Player* player = ent->as< Player* >( );

		    // access out player data stucture and reset player data.
		    AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];
            if( data )
		        data->reset( );

		    // get ptr to vmt instance and reset tables.
		    VMT* vmt = &g_hooks.m_player[ player->index( ) - 1 ];
            if( vmt ) {
                // init vtable with new ptr.
		        vmt->reset( );
		        vmt->init( player );

		        // hook this on every player.
		        g_hooks.m_DoExtraBoneProcessing = vmt->add< Hooks::DoExtraBoneProcessing_t >( Player::DOEXTRABONEPROCESSING, util::force_cast( &Hooks::DoExtraBoneProcessing ) );

		        // local gets special treatment.
		        if( player->index( ) == g_csgo.m_engine->GetLocalPlayer( ) ) {
		        	g_hooks.m_UpdateClientSideAnimation = vmt->add< Hooks::UpdateClientSideAnimation_t >( Player::UPDATECLIENTSIDEANIMATION, util::force_cast( &Hooks::UpdateClientSideAnimation ) );
                    g_hooks.m_GetActiveWeapon           = vmt->add< Hooks::GetActiveWeapon_t >( Player::GETACTIVEWEAPON, util::force_cast( &Hooks::GetActiveWeapon ) );
                    g_hooks.m_BuildTransformations      = vmt->add< Hooks::BuildTransformations_t >( Player::BUILDTRANSFORMATIONS, util::force_cast( &Hooks::BuildTransformations ) );
                }
				
				//g_hooks.m_CalcView = vmt->add< Hooks::CalcView_t > ( CalculateView_idx, util::force_cast ( &Hooks::CalcView ) );
            }
        }

        // ragdoll created.
        // note - dex; sadly, it seems like m_hPlayer (GetRagdollPlayer) is null here... probably has to be done somewhere else.
        // if( ent->is( HASH( "CCSRagdoll" ) ) ) {
        //     Player *ragdoll_player{ ent->GetRagdollPlayer( ) };
        // 
        //     // note - dex;  ragdoll ents (DT_CSRagdoll) seem to contain some new netvars now, m_flDeathYaw and m_flAbsYaw.
        //     //              didnt test much but making a bot with bot_mimic look at yaws:
        //     //
        //     //              -107.98 gave me m_flDeathYaw=-16.919 and m_flAbsYaw=268.962
        //     //              46.05 gave me m_flDeathYaw=-21.685 and m_flAbsYaw=67.742
        //     //             
        //     //              these angles don't seem consistent... but i didn't test much.
        // 
        //     g_cl.print( "ragdoll 0x%p created on client at time %f, from player 0x%p\n", ent, g_csgo.m_globals->m_curtime, ragdoll_player );
        // }
	}
}

void CustomEntityListener::OnEntityDeleted( Entity *ent ) {
    // note; IsPlayer doesn't work here because the ent class is CBaseEntity.
	if( ent && ent->index( ) >= 1 && ent->index( ) <= 64 ) {
		Player* player = ent->as< Player* >( );

		// access out player data stucture and reset player data.
		AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];
        if( data )
		    data->reset( );

		// get ptr to vmt instance and reset tables.
		VMT* vmt = &g_hooks.m_player[ player->index( ) - 1 ];
		if( vmt )
		    vmt->reset( );
	}
}