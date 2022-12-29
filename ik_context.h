#pragma once

// forward declarations
class CStudioHdr;

class CIKContext {
	uint8_t pad [ 0x1070 ];
public:
	void *operator new( size_t size );
	void  operator delete( void *ptr );

	static void Construct ( CIKContext *ik );
	void Init ( const CStudioHdr *hdr, const ang_t &local_angles, const vec3_t &local_origin, float current_time, int frame_count, int bone_mask );
	void UpdateTargets ( vec3_t *pos, quaternion_t *q, matrix3x4_t *bone_cache, uint8_t *computed );
	void SolveDependencies ( vec3_t *pos, quaternion_t *q, matrix3x4_t *bone_cache, uint8_t *computed );
	void ClearTargets ( );
	void AddDependencies ( void* seqdesc, int iSequence, float flCycle, const float poseParameters [ ], float flWeight );
	void CopyTo ( CIKContext *other, const unsigned short *iRemapping );
};