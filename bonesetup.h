#pragma once

class Bones {

public:
	bool m_running;

public:
	bool setup( Player* player, BoneArray* out, float curtime, LagRecord *record );
	bool Build ( Player *player, BoneArray *out, float curtime );
    void Studio_BuildMatrices ( const CStudioHdr *pStudioHdr, const ang_t &angles, const vec3_t &origin, const vec3_t pos [ ], const quaternion_t q [ ], int iBone, float flScale, matrix3x4a_t bonetoworld [ 128 ], int boneMask );
	void SetupBones ( Player *player, matrix3x4a_t *pBoneToWorld, int boneMask );
    void *GetSeqDesc ( void *ptr, int i );
    void GetSkeleton ( Player *player, CStudioHdr *studio_hdr, vec3_t *pos, quaternion_t *q, int bone_mask, CIKContext *ik );
    void BuildMatrices ( Player *player, CStudioHdr *studio_hdr, vec3_t *pos, quaternion_t *q, matrix3x4_t *bone_to_world, int bone_mask );
    void ConcatTransforms ( const matrix3x4_t &m0, const matrix3x4_t &m1, matrix3x4_t &out );
	bool BuildBones( Player* target, int mask, BoneArray* out, LagRecord* record );
};

struct CBoneSetup {
    CBoneSetup ( const CStudioHdr *studio_hdr, int bone_mask, float *pose_parameters );
    void InitPose ( vec3_t pos [ ], quaternion_t q [ ] );
    void AccumulatePose ( vec3_t pos [ ], quaternion_t q [ ], int sequence, float cycle, float weight, float time, void *IKContext );
    void CalcAutoplaySequences ( vec3_t pos [ ], quaternion_t q [ ], float real_time, void *IKContext );
    void CalcBoneAdj ( vec3_t pos [ ], quaternion_t q [ ], const float controllers [ ] );

    const CStudioHdr *m_pStudioHdr;
    int m_boneMask;
    float *m_flPoseParameter;
    void *m_pPoseDebugger;
};

struct IBoneSetup {
    CBoneSetup *bone_setup;
};

extern Bones g_bones;