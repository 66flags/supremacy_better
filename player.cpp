#include "includes.h"

void Hooks::DoExtraBoneProcessing ( int a2, int a3, int a4, int a5, int a6, int a7 ) {
	auto state = ( CCSGOPlayerAnimState * ) a2;

	const auto backup_on_ground = state->m_ground;

	state->m_ground = false;
	g_hooks.m_DoExtraBoneProcessing ( this, a2, a3, a4, a5, a6, a7 );
	state->m_ground = backup_on_ground;
}

void Hooks::BuildTransformations ( int a2, int a3, int a4, int a5, int a6, int a7 ) {
	// cast thisptr to player ptr.
	Player *player = ( Player * ) this;

	// get bone jiggle.
	int bone_jiggle = *reinterpret_cast< int * >( uintptr_t ( player ) + 0x291C );

	// null bone jiggle to prevent attachments from jiggling around.
	*reinterpret_cast< int * >( uintptr_t ( player ) + 0x291C ) = 0;

	// call og.
	g_hooks.m_BuildTransformations ( this, a2, a3, a4, a5, a6, a7 );

	// restore bone jiggle.
	*reinterpret_cast< int * >( uintptr_t ( player ) + 0x291C ) = bone_jiggle;
}

void Hooks::UpdateClientSideAnimation ( ) {
	auto player = ( Player * ) this;

	if ( !player || !player->alive ( ) )
		return g_hooks.m_UpdateClientSideAnimation ( this );

	if ( player == g_cl.m_local ) {
		auto state = ( CCSGOGamePlayerAnimState * ) g_cl.m_local->m_PlayerAnimState ( );

		if ( g_cl.m_animate ) {
			g_hooks.m_UpdateClientSideAnimation ( this );
		}
		else {
			memcpy ( g_cl.m_local->m_AnimOverlay ( ), g_cl.anim_data.m_last_layers, sizeof ( C_AnimationLayer ) * 13 );

			g_cl.m_local->SetPoseParameters ( g_cl.anim_data.m_poses );
			g_cl.m_local->SetAbsAngles ( ang_t ( 0.0f, g_cl.anim_data.m_rotation.y, 0.0f ) );
			state->m_flFootYaw = g_cl.m_rotation.y;

			// this will update the attachments origin
			static auto SetupBones_AttachmentHelper = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4 56 57 85 DB 0F 84" ) ).as < void ( __thiscall * ) ( void *, void * ) > ( );
			SetupBones_AttachmentHelper ( g_cl.m_local, g_cl.m_local->GetModelPtr ( ) );

			// force setupbones rebuild
			g_bones.Build ( player, nullptr, g_csgo.m_globals->m_curtime );
		}
	}
	
	if ( player != g_cl.m_local ) {
		player->SetAbsOrigin ( player->m_vecOrigin ( ) );

		if ( g_cl.m_update_ent ) {
			// @onetap
			if ( player->m_AnimOverlay ( ) ) {
				for ( int i = 0; i < ANIMATION_LAYER_COUNT; i++ ) {
					player->m_AnimOverlay ( ) [ i ].m_owner = player;
				}
			}

			// will force GetAbsVelocity in CCSGOPlayerAnimState::Update to not do shit to velocity
			// also, we hook IsHLTV so we know that we call GetAbsVelocity not EstimateAbsVelocity

			int iEFlags = player->m_iEFlags ( );

			player->m_iEFlags ( ) &= ~( 1 << 12 );

			g_hooks.m_UpdateClientSideAnimation ( this );

			// restore entity flags
			player->m_iEFlags ( ) = iEFlags;
		}
	}
	else
		g_hooks.m_UpdateClientSideAnimation ( this );
}

void Hooks::CalcView ( vec3_t &eye_origin, ang_t &eye_angles, float &z_near, float &z_far, float &fov ) {
	Player *player = ( Player * ) this;

	if ( !player )
		return g_hooks.m_CalcView ( this, eye_origin, eye_angles, z_near, z_far, fov );

	const auto old_use_new_animation_state = *( bool * ) ( std::uintptr_t ( player ) + 0x39E1 );

	// prevent calls to ModifyEyePosition
	*( bool * ) ( std::uintptr_t ( player ) + 0x39E1 ) = false; // 0x39E1

	// call og.
	g_hooks.m_CalcView ( this, eye_origin, eye_angles, z_near, z_far, fov );

	*( bool * ) ( std::uintptr_t ( player ) + 0x39E1 ) = old_use_new_animation_state;
}

void Hooks::NotifyOnLayerChangeWeight ( const C_AnimationLayer *layer, const float new_weight ) {
	return;
}

void Hooks::NotifyOnLayerChangeCycle ( const C_AnimationLayer *layer, const float new_cycle ) {
	return;
}

ang_t &Hooks::GetEyeAngles ( ) {
	Stack stack;

	static Address ret1 = pattern::find ( g_csgo.m_client_dll, XOR ( "8B CE F3 0F 10 00 8B 06 F3 0F 11 45 ? FF 90 ? ? ? ? F3 0F 10 55 ?" ) );
	static Address ret2 = pattern::find ( g_csgo.m_client_dll, XOR ( "F3 0F 10 55 ? 51 8B 8E ? ? ? ?" ) );

	if ( stack.ReturnAddress ( ) == ret1 || stack.ReturnAddress ( ) == ret2 )
		return g_cl.m_angle;

	return g_hooks.m_GetEyeAngles ( this );
}

void Hooks::AccumulateLayers ( void *setup, vec3_t &pos, void *q, float time ) {
	auto player = ( Player * ) this;

	static auto iks_off = pattern::find ( g_csgo.m_client_dll, XOR ( "8D 47 FC 8B 8F" ) ).add ( 5 ).to ( ).add ( 4 ).as< uint32_t > ( );
	static auto accumulate_pose = pattern::find ( g_csgo.m_server_dll, XOR ( "E8 ? ? ? ? 83 BF ? ? ? ? ? 0F 84 ? ? ? ? 8D" ) ).rel32 ( 0x1 ).as<void ( __thiscall * )( void *, vec3_t &, void *, int, float, float, float, void * )> ( );

	if ( !player || !player->IsPlayer ( ) || player->m_iHealth ( ) <= 0 || !player->m_AnimOverlay ( ) || !*reinterpret_cast< void ** >( reinterpret_cast< uintptr_t >( player ) + iks_off ) )
		return g_hooks.m_AccumulateLayers ( this, setup, pos, q, time );

	for ( auto animLayerIndex = 0; animLayerIndex < 13; animLayerIndex++ ) {
		auto &layer = player->m_AnimOverlay ( ) [ animLayerIndex ];

		if ( layer.m_weight > 0.0f && layer.m_order >= 0 && layer.m_order < 13 )
			accumulate_pose ( *reinterpret_cast< void ** >( setup ), pos, q, layer.m_sequence, layer.m_cycle, layer.m_weight, time, *reinterpret_cast< void ** >( reinterpret_cast< uintptr_t >( player ) + iks_off ) );
	}
}

Weapon *Hooks::GetActiveWeapon ( ) {
	Stack stack;

	static Address ret_1 = pattern::find ( g_csgo.m_client_dll, XOR ( "85 C0 74 1D 8B 88 ? ? ? ? 85 C9" ) );

	// note - dex; stop call to CIronSightController::RenderScopeEffect inside CViewRender::RenderView.
	if ( g_menu.main.visuals.noscope.get ( ) ) {
		if ( stack.ReturnAddress ( ) == ret_1 )
			return nullptr;
	}

	return g_hooks.m_GetActiveWeapon ( this );
}

void CustomEntityListener::OnEntityCreated ( Entity *ent ) {
	if ( ent ) {
		// player created.
		if ( ent->IsPlayer ( ) ) {
			Player *player = ent->as< Player * > ( );

			// access out player data stucture and reset player data.
			AimPlayer *data = &g_aimbot.m_players [ player->index ( ) - 1 ];
			if ( data )
				data->reset ( );

			// get ptr to vmt instance and reset tables.
			VMT *vmt = &g_hooks.m_player [ player->index ( ) - 1 ];
			if ( vmt ) {
				// init vtable with new ptr.
				vmt->reset ( );
				vmt->init ( player );

				// hook this on every player.
				g_hooks.m_DoExtraBoneProcessing = vmt->add< Hooks::DoExtraBoneProcessing_t > ( Player::DOEXTRABONEPROCESSING, util::force_cast ( &Hooks::DoExtraBoneProcessing ) );
					g_hooks.m_UpdateClientSideAnimation = vmt->add< Hooks::UpdateClientSideAnimation_t > ( Player::UPDATECLIENTSIDEANIMATION, util::force_cast ( &Hooks::UpdateClientSideAnimation ) );
				
				g_hooks.m_NotifyOnLayerChangeCycle = vmt->add< Hooks::NotifyOnLayerChangeCycle_t > ( Player::NOTIFYONLAYERCHANGECYCLE, util::force_cast ( &Hooks::NotifyOnLayerChangeCycle ) );
				g_hooks.m_NotifyOnLayerChangeWeight = vmt->add< Hooks::NotifyOnLayerChangeWeight_t > ( Player::NOTIFYONLAYERCHANGEWEIGHT, util::force_cast ( &Hooks::NotifyOnLayerChangeWeight ) );

				// local gets special treatment.
				if ( player->index ( ) == g_csgo.m_engine->GetLocalPlayer ( ) ) {
					g_hooks.m_GetActiveWeapon = vmt->add< Hooks::GetActiveWeapon_t > ( Player::GETACTIVEWEAPON, util::force_cast ( &Hooks::GetActiveWeapon ) );
					g_hooks.m_BuildTransformations = vmt->add< Hooks::BuildTransformations_t > ( Player::BUILDTRANSFORMATIONS, util::force_cast ( &Hooks::BuildTransformations ) );
					g_hooks.m_CalcView = vmt->add< Hooks::CalcView_t > ( Player::CALCVIEW, util::force_cast ( &Hooks::CalcView ) );
					g_hooks.m_GetEyeAngles = vmt->add< Hooks::GetEyeAngles_t > ( Player::GETEYEANGLES, util::force_cast ( &Hooks::GetEyeAngles ) );
					//g_hooks.m_AccumulateLayers = vmt->add< Hooks::AccumulateLayers_t > ( Player::ACCUMULATELAYERS, util::force_cast ( &Hooks::AccumulateLayers ) );
				}
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

void CustomEntityListener::OnEntityDeleted ( Entity *ent ) {
	// note; IsPlayer doesn't work here because the ent class is CBaseEntity.
	if ( ent && ent->index ( ) >= 1 && ent->index ( ) <= 64 ) {
		Player *player = ent->as< Player * > ( );

		// access out player data stucture and reset player data.
		AimPlayer *data = &g_aimbot.m_players [ player->index ( ) - 1 ];
		if ( data )
			data->reset ( );

		// get ptr to vmt instance and reset tables.
		VMT *vmt = &g_hooks.m_player [ player->index ( ) - 1 ];
		if ( vmt )
			vmt->reset ( );
	}
}