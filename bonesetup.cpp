#include "includes.h"

// NOTE - hypnotic:
// Pretty much rebuilt server setupbones except there's some crashing issues
// For now you're better off just using client setupbones with a few fixes :P

CBoneSetup::CBoneSetup ( const CStudioHdr *studio_hdr, int bone_mask, float *pose_parameters )
	: m_pStudioHdr ( studio_hdr )
	, m_boneMask ( bone_mask )
	, m_flPoseParameter ( pose_parameters )
	, m_pPoseDebugger ( nullptr ) {
}

void CBoneSetup::InitPose ( vec3_t pos [ ], quaternion_t q [ ] ) {
	auto hdr = m_pStudioHdr->m_pStudioHdr;

	for ( int i = 0; i < hdr->m_num_bones; i++ ) {
		auto bone = hdr->GetBone ( i );

		if ( bone->m_flags & m_boneMask ) {
			pos [ i ] = bone->m_pos;
			q [ i ] = bone->m_quat;
		}
	}
}

void CBoneSetup::AccumulatePose ( vec3_t pos [ ], quaternion_t q [ ], int sequence, float cycle, float weight, float time, void *IKContext ) {
	using AccumulatePoseFn = void ( __thiscall * )( CBoneSetup *, vec3_t *, quaternion_t *, int, float, float, float, void * );
	static auto AccumulatePose = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? A1" ) ).as < AccumulatePoseFn  > ( );
	AccumulatePose ( this, pos, q, sequence, cycle, weight, time, IKContext );
}

void CBoneSetup::CalcAutoplaySequences ( vec3_t pos [ ], quaternion_t q [ ], float real_time, void *IKContext ) {
	using CalcAutoplaySequencesFn = void ( __thiscall * )( CBoneSetup *, vec3_t *, quaternion_t *, float, void * );
	CalcAutoplaySequencesFn CalcAutoplaySequences = pattern::find ( g_csgo.m_client_dll, XOR ( "E8 ? ? ? ? 8B 06 83 B8 ? ? ? ? ? 74 2B 8B 07 8D" ) ).rel32 ( 0x1 ).as < CalcAutoplaySequencesFn > ( );
	
	__asm {
		mov ecx, this
		movss xmm0, real_time
		push IKContext
		push q
		push pos
		call CalcAutoplaySequences
	}
}

void CBoneSetup::CalcBoneAdj ( vec3_t pos [ ], quaternion_t q [ ], const float controllers [ ] ) {
	using CalcBoneAdjFn = void ( __thiscall * )( CBoneSetup *, const CStudioHdr *, vec3_t *, quaternion_t *, const float *, int );
	CalcBoneAdjFn CalcBoneAdj = pattern::find ( g_csgo.m_client_dll, XOR ( "E8 ? ? ? ? 83 C4 0C F3 0F 10 45 ? 51 F3 0F 11 04 24 8B CF FF" ) ).rel32 ( 0x1 ).as < CalcBoneAdjFn > ( );
	
	__asm {
		mov ecx, m_pStudioHdr
		mov edx, this
		push m_boneMask
		push controllers
		push q
		push pos
		call CalcBoneAdj
	}
}

Bones g_bones {};;

bool Bones::Build ( Player *player, BoneArray *out, float curtime ) {
#if 1
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
//	const auto backup_abs_origin = player->GetAbsOrigin ( );
	const auto backup_setup_time = player->m_flLastBoneSetupTime ( );
	const auto backup_maintain_seq_transitions_value = *( bool * ) ( std::uintptr_t ( player ) + 0x9F0 );

	*( bool * ) ( std::uintptr_t ( player ) + 0x9F0 ) = false; // skip call to MaintainSequenceTransitions
	player->m_fEffects ( ) |= EF_NOINTERP;
//	player->SetAbsOrigin ( player->m_vecOrigin ( ) );
	player->m_flLastBoneSetupTime ( ) = 0;
	player->InvalidateBoneCache ( );

	m_running = true;
	bool setup = player->SetupBones ( out, 128, 0x7FF00, curtime );
	m_running = false;

	player->m_fEffects ( ) = backup_effects;
	player->m_flLastBoneSetupTime ( ) = backup_setup_time;
	//player->SetAbsOrigin ( backup_abs_origin );
	*( bool * ) ( std::uintptr_t ( player ) + 0x9F0 ) = backup_maintain_seq_transitions_value;

	g_csgo.m_globals->m_curtime = cur_time;
	g_csgo.m_globals->m_frametime = frame_time;
	g_csgo.m_globals->m_abs_frametime = abs_frame_time;
	g_csgo.m_globals->m_frame = frame_count;
	g_csgo.m_globals->m_tick_count = tick_count;
#else
	// build server bones.
	SetupBones ( player, out, 0x7FF00 );
#endif

	return setup;
}

void Bones::Studio_BuildMatrices (
	const CStudioHdr *pStudioHdr,
	const ang_t &angles,
	const vec3_t &origin,
	const vec3_t pos [ ],
	const quaternion_t q [ ],
	int iBone,
	float flScale,
	BoneArray bonetoworld [ 128 ],
	int boneMask
) {
	//BONE_PROFILE_FUNC ( );
	int i, j;

	int					chain [ 128 ] = {};
	int					chainlength = 0;

	if ( iBone < -1 || iBone >= pStudioHdr->m_pStudioHdr->m_num_bones )
		iBone = 0;

	// build list of what bones to use
	if ( iBone == -1 ) {
		// all bones
		chainlength = pStudioHdr->m_pStudioHdr->m_num_bones;
		for ( i = 0; i < pStudioHdr->m_pStudioHdr->m_num_bones; i++ ) {
			chain [ chainlength - i - 1 ] = i;
		}
	}
	else {
		// only the parent bones
		i = iBone;
		while ( i != -1 ) {
			auto bone = pStudioHdr->m_pStudioHdr->GetBone ( i );
			chain [ chainlength++ ] = i;
			i = bone->m_parent;
		}
	}

	matrix3x4_t bonematrix;
	matrix3x4_t rotationmatrix; // model to world transformation
	math::AngleMatrix ( angles, origin, rotationmatrix );

	// Account for a change in scale
	if ( flScale < 1.0f - FLT_EPSILON || flScale > 1.0f + FLT_EPSILON ) {
		vec3_t vecOffset;
		math::MatrixGetColumn ( rotationmatrix, 3, vecOffset );
		vecOffset -= origin;
		vecOffset *= flScale;
		vecOffset += origin;
		math::MatrixSetColumn ( vecOffset, 3, rotationmatrix );

		// Scale it uniformly
		math::VectorScale ( rotationmatrix [ 0 ], flScale, rotationmatrix [ 0 ] );
		math::VectorScale ( rotationmatrix [ 1 ], flScale, rotationmatrix [ 1 ] );
		math::VectorScale ( rotationmatrix [ 2 ], flScale, rotationmatrix [ 2 ] );
	}

	// check for 16 byte alignment
	if ( ( ( ( size_t ) bonetoworld ) % 16 ) != 0 ) {
		for ( j = chainlength - 1; j >= 0; j-- ) {
			i = chain [ j ];
			auto bone = pStudioHdr->m_pStudioHdr->GetBone ( i );
			if ( bone->m_flags & boneMask ) {
				math::QuaternionMatrix ( q [ i ], pos [ i ], bonematrix );

				if ( bone->m_parent == -1 ) {
					ConcatTransforms ( rotationmatrix, bonematrix, &bonetoworld [ i ] );
				}
				else {
					ConcatTransforms ( bonetoworld [ bone->m_parent ], bonematrix, &bonetoworld [ i ] );
				}
			}
		}
	}
	else {
		for ( j = chainlength - 1; j >= 0; j-- ) {
			i = chain [ j ];
			auto bone = pStudioHdr->m_pStudioHdr->GetBone ( i );
			if ( bone->m_flags & boneMask ) {
				math::QuaternionMatrix ( q [ i ], pos [ i ], bonematrix );

				if ( bone->m_parent == -1 ) {
					ConcatTransforms ( rotationmatrix, bonematrix, &bonetoworld [ i ] );
				}
				else {
					ConcatTransforms ( bonetoworld [ bone->m_parent ], bonematrix, &bonetoworld [ i ] );
				}
			}
		}
	}
}

void Bones::SetupBones ( Player *player, BoneArray *pBoneToWorld, int boneMask ) {
	g_csgo.m_model_cache->BeginLock ( );

	auto studio_hdr = player->GetModelPtr ( );

	matrix3x4a_t aligned_pBoneToWorld [ 128 ];

	if ( studio_hdr ) {
		alignas( 16 ) vec3_t pos [ 128 ];
		alignas( 16 ) quaternion_t q [ 128 ];

		CIKContext *ik = new CIKContext;

		CIKContext *backup = player->m_pIK ( );
		*( CIKContext ** ) ( std::uintptr_t ( player ) + 0x2660 ) = ik;

		uint8_t bone_computed [ 32 ] {};
		memset ( &bone_computed, 0, sizeof ( bone_computed ) );

		ik->Init ( studio_hdr, player->GetAbsAngles ( ), player->GetAbsOrigin ( ), player->m_flSimulationTime ( ), g_csgo.m_globals->m_tick_count, boneMask );
		GetSkeleton ( player, studio_hdr, pos, q, boneMask, ik );

		ik->UpdateTargets ( pos, q, pBoneToWorld, &bone_computed );
		player->CalculateIKLocks ( player->m_flSimulationTime ( ) );
		ik->SolveDependencies ( pos, q, pBoneToWorld, &bone_computed );

		*( CIKContext ** ) ( std::uintptr_t ( player ) + 0x2660 ) = backup;
		delete ik;

		//BuildMatrices ( player, studio_hdr, pos, q, player->m_BoneCache ( ).m_pCachedBones, boneMask );
		Studio_BuildMatrices ( studio_hdr, player->GetAbsAngles ( ), player->m_vecOrigin ( ), pos, q, -1, 1.0f, pBoneToWorld, boneMask );
	}

	if ( pBoneToWorld && boneMask >= player->m_BoneCache ( ).m_CachedBoneCount )
		memcpy ( pBoneToWorld, player->m_BoneCache ( ).m_pCachedBones, sizeof ( matrix3x4a_t ) * player->m_iBoneCount ( ) );

	// copy aligned matrix to our output.
	//memcpy ( out, mat, sizeof ( BoneArray ) * 128 );

	g_csgo.m_model_cache->EndLock ( );
}

void Bones::GetSkeleton ( Player *player, CStudioHdr *studio_hdr, vec3_t *pos, quaternion_t *q, int bone_mask, CIKContext *ik ) {
	static auto GetSeqDesc = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 79 04 00 75 25 8B 45 08 8B 09 85 C0 78 08 3B 81" ) ).as< void *( __thiscall * )( void *, int ) > ( );

	CBoneSetup boneSetup ( studio_hdr, bone_mask, player->m_flPoseParameter ( ) );
	boneSetup.InitPose ( pos, q );
	boneSetup.AccumulatePose ( pos, q, player->m_nSequence ( ), player->m_flCycle ( ), 1.f, player->m_flSimulationTime ( ), ik );

	constexpr int MAX_LAYER_COUNT = 15;
	int layer [ MAX_LAYER_COUNT ] {};
	for ( int i = 0; i < player->m_iNumOverlays ( ); i++ ) {
		layer [ i ] = MAX_LAYER_COUNT;
	}

	for ( int i = 0; i < player->m_iNumOverlays ( ); i++ ) {
		auto pLayer = &player->m_AnimOverlay ( ) [ i ];
		if ( ( pLayer->m_weight > 0.f ) && pLayer->m_sequence != -1 && pLayer->m_order >= 0 && pLayer->m_order < player->m_iNumOverlays ( ) ) {
			layer [ pLayer->m_order ] = i;
		}
	}

	Weapon *weapon = nullptr;
	Weapon *weapon_world_model = nullptr;
	C_BoneMergeCache *bone_merge_cache = nullptr;
	C_BoneMergeCache *bone_merge_backup = nullptr;

	bool do_weapon_setup = false;

	if ( player->GetClientClass ( )->m_ClassID == g_netvars.GetClientID ( HASH ( "CCSPlayer" ) ) ) {
		weapon = player->GetActiveWeapon ( );
		if ( weapon ) {
			weapon_world_model = weapon->GetWeaponWorldModel ( );
			if ( weapon_world_model && weapon_world_model->GetModelPtr ( ) ) {
				//bone_merge_backup = weapon_world_model->m_pBoneMergeCache ( );

				//bone_merge_cache = new C_BoneMergeCache;
				//*( C_BoneMergeCache ** ) ( std::uintptr_t ( weapon_world_model ) + 0x28FC ) = bone_merge_cache;
				//weapon_world_model->m_pBoneMergeCache ( )->Init ( weapon_world_model );

				if ( weapon_world_model->m_pBoneMergeCache ( ) )
					do_weapon_setup = true;
			}
		}
	}

	if ( do_weapon_setup ) {
		CStudioHdr *weapon_studio_hdr = weapon_world_model->GetModelPtr ( );

		weapon_world_model->m_pBoneMergeCache ( )->MergeMatchingPoseParams ( );

		CIKContext *weaponIK = new CIKContext;
		weaponIK->Init ( weapon_studio_hdr, player->GetAbsAngles ( ), player->GetAbsOrigin ( ), player->m_flSimulationTime ( ), 0, BONE_USED_BY_BONE_MERGE );

		CBoneSetup weaponSetup ( weapon_studio_hdr, BONE_USED_BY_BONE_MERGE, weapon_world_model->m_flPoseParameter ( ) );
		vec3_t weaponPos [ 256 ];
		quaternion_t weaponQ [ 256 ];

		weaponSetup.InitPose ( weaponPos, weaponQ );

		for ( int i = 0; i < player->m_iNumOverlays ( ); i++ ) {
			C_AnimationLayer *pLayer = &player->m_AnimOverlay ( ) [ i ];

			if ( pLayer->m_sequence <= 1 || pLayer->m_weight <= 0.f )
				continue;

			player->UpdateDispatchLayer ( pLayer, weapon_studio_hdr, pLayer->m_sequence );

			if ( pLayer->m_priority <= 0 || pLayer->m_priority >= studio_hdr->GetNumSeq ( ) ) {
				boneSetup.AccumulatePose ( pos, q, pLayer->m_sequence, pLayer->m_cycle, pLayer->m_weight, player->m_flSimulationTime ( ), ik );
			}
			else {
				weapon_world_model->m_pBoneMergeCache ( )->CopyFromFollow ( pos, q, BONE_USED_BY_BONE_MERGE, weaponPos, weaponQ );

				void *seqdesc = GetSeqDesc ( studio_hdr, pLayer->m_sequence );
				ik->AddDependencies ( seqdesc, pLayer->m_sequence, pLayer->m_cycle, player->m_flPoseParameter ( ), pLayer->m_weight );

				weaponSetup.AccumulatePose ( weaponPos, weaponQ, pLayer->m_priority, pLayer->m_cycle, pLayer->m_weight, player->m_flSimulationTime ( ), weaponIK );

				weapon_world_model->m_pBoneMergeCache ( )->CopyToFollow ( weaponPos, weaponQ, BONE_USED_BY_BONE_MERGE, pos, q );

				weaponIK->CopyTo ( ik, weapon_world_model->m_pBoneMergeCache ( )->m_iRawIndexMapping );
			}
		}

		delete weaponIK;
	}
	else {
		for ( int i = 0; i < player->m_iNumOverlays ( ); i++ ) {
			if ( layer [ i ] >= 0 && layer [ i ] < player->m_iNumOverlays ( ) ) {
				C_AnimationLayer pLayer = player->m_AnimOverlay ( ) [ layer [ i ] ];
				boneSetup.AccumulatePose ( pos, q, pLayer.m_sequence, pLayer.m_cycle, pLayer.m_weight, player->m_flSimulationTime ( ), ik );
			}
		}
	}

	CIKContext *auto_ik = new CIKContext;
	auto_ik->Init ( studio_hdr, player->GetAbsAngles ( ), player->GetAbsOrigin ( ), player->m_flSimulationTime ( ), 0, bone_mask );
	boneSetup.CalcAutoplaySequences ( pos, q, player->m_flSimulationTime ( ), auto_ik );
	delete auto_ik;

	if ( *( int* ) ( std::uintptr_t ( studio_hdr->m_pStudioHdr ) + 0xA4 ) > 0 )
		boneSetup.CalcBoneAdj ( pos, q, player->m_flEncodedController ( ) );

	//if ( weapon_world_model && bone_merge_cache ) {
	//	*( C_BoneMergeCache** )( std::uintptr_t ( weapon_world_model ) + 0x28FC ) = bone_merge_backup;
	//	delete bone_merge_cache;
	//}
}

void Bones::BuildMatrices ( Player *player, CStudioHdr *studio_hdr, vec3_t *pos, quaternion_t *q, BoneArray *bone_to_world, int bone_mask ) {
	int i = 0, j = 0;
	int chain [ 128 ] = { };
	int chain_length = studio_hdr->m_pStudioHdr->m_num_bones;

	for ( i = 0; i < studio_hdr->m_pStudioHdr->m_num_bones; i++ )
		chain [ chain_length - i - 1 ] = i;

	matrix3x4_t bone_matrix = {};
	matrix3x4_t rotation_matrix = {};
	math::AngleMatrix ( player->GetAbsAngles ( ), player->GetAbsOrigin ( ), rotation_matrix );

	for ( j = chain_length - 1; j >= 0; j-- ) {
		i = chain [ j ];

		auto bone = studio_hdr->m_pStudioHdr->GetBone ( i );
		if ( bone->m_flags & bone_mask ) {
			math::QuaternionMatrix ( q [ i ], pos [ i ], bone_matrix );

			int bone_parent = bone->m_parent;
			if ( bone_parent == -1 ) {
				math::ConcatTransforms ( bone_to_world [ bone_parent ], bone_matrix, bone_to_world [ i ] );
			}
			else {
				math::ConcatTransforms ( bone_to_world [ bone_parent ], bone_matrix, bone_to_world [ i ] );
			}
		}
	}
}

void Bones::ConcatTransforms ( const matrix3x4_t &m0, const matrix3x4_t &m1, matrix3x4_t *out ) {
	for ( int i = 0; i < 3; i++ ) {
		*out [ i ][ 3 ] = m1 [ 0 ][ 3 ] * m0 [ i ][ 0 ] + m1 [ 1 ][ 3 ] * m0 [ i ][ 1 ] + m1 [ 2 ][ 3 ] * m0 [ i ][ 2 ] + m0 [ i ][ 3 ];

		for ( int j = 0; j < 3; j++ ) {
			*out [ i ][ j ] = m0 [ i ][ 0 ] * m1 [ 0 ][ j ] + m0 [ i ][ 1 ] * m1 [ 1 ][ j ] + m0 [ i ][ 2 ] * m1 [ 2 ][ j ];
		}
	}
}

bool Bones::BuildBones ( Player *target, int mask, BoneArray *out, LagRecord *record ) {
	return true;
}