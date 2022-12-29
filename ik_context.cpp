#include "includes.h"

void *CIKContext::operator new( size_t size ) {
    CIKContext *ptr = ( CIKContext * ) g_csgo.m_mem_alloc->Alloc ( size );
    Construct ( ptr );

    return ptr;
}

void CIKContext::operator delete( void *ptr ) {
    g_csgo.m_mem_alloc->Free ( ptr );
}

void CIKContext::Construct ( CIKContext *ik ) {
    using ConstructFn = CIKContext * ( __fastcall * )( CIKContext * );
    static auto Construct = pattern::find ( g_csgo.m_client_dll, XOR ( "56 8B F1 6A 00 6A 00 C7 86 ? ? ? ? ? ? ? ? 89 B6 ? ? ? ? C7 86" ) ).as < ConstructFn > ( );
    Construct ( ik );
}

void CIKContext::Init ( const CStudioHdr *hdr, const ang_t &local_angles, const vec3_t &local_origin, float current_time, int frame_count, int bone_mask ) {
    using InitFn = void ( __thiscall * )( void*, const CStudioHdr *, const ang_t &, const vec3_t &, float, int, int );
    static auto Init = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D 8F ? ? ? ? 89 7D" ) ).as < InitFn > ( );
    Init ( this, hdr, local_angles, local_origin, current_time, frame_count, bone_mask );
}

void CIKContext::UpdateTargets ( vec3_t *pos, quaternion_t *q, matrix3x4_t *bone_cache, uint8_t *computed ) {
    using UpdateTargetsFn = void ( __thiscall * )( void *, vec3_t *, quaternion_t *, matrix3x4_t *, uint8_t * );
    static auto UpdateTargets = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 83 E4 F0 81 ? ? ? ? ? 33 D2 89" ) ).as < UpdateTargetsFn > ( );
    UpdateTargets ( this, pos, q, bone_cache, computed );
}

void CIKContext::SolveDependencies ( vec3_t *pos, quaternion_t *q, matrix3x4_t *bone_cache, uint8_t *computed ) {
    using SolveDependenciesFn = void ( __thiscall * )( void *, vec3_t *, quaternion_t *, matrix3x4_t *, uint8_t * );
	static auto SolveDependencies = pattern::find ( g_csgo.m_client_dll, XOR ( "55 8B EC 81 EC ? ? ? ? 53 56 57 8B F9 0F 28 CB F3 0F 11 4D ? 8B 8F ? ? ? ? 8B 01 83 B8" ) ).as < SolveDependenciesFn > ( );
    SolveDependencies ( this, pos, q, bone_cache, computed );
}

struct CIKTarget {
    int m_iFrameCount;

private:
    char pad_00004 [ 0x51 ];
};

void CIKContext::ClearTargets ( ) {
    int v49 = 0;
	
    if ( *( int * )( std::uintptr_t ( this ) + 4080 ) > 0 ) {
       int* iFramecounter = ( int * ) ( std::uintptr_t ( this ) + 0xD0 );
        do {
            *iFramecounter = -9999;
            iFramecounter += 85;
            ++v49;
        } while ( v49 < *( int * ) ( std::uintptr_t ( this ) + 0xFF0 ) );
    }
}

void CIKContext::AddDependencies ( void* seqdesc, int iSequence, float flCycle, const float poseParameters [ ], float flWeight ) {
    using AddDependenciesFn = void ( __thiscall * )( void *,void*, int, float, const float [ ], float );
	static auto AddDependencies = pattern::find ( g_csgo.m_server_dll, XOR ( "55 8B EC 81 EC ? ? ? ? 53 56 57 8B F9 0F 28 CB F3 0F 11 4D ? 8B 8F" ) ).as < AddDependenciesFn > ( );
    AddDependencies ( this, seqdesc, iSequence, flCycle, poseParameters, flWeight );
}

void CIKContext::CopyTo ( CIKContext *other, const unsigned short *iRemapping ) {
    using CopyToFn = void ( __thiscall * )( void *, CIKContext *, const unsigned short * );
    static auto CopyTo = pattern::find ( g_csgo.m_server_dll, XOR ( "55 8B EC 83 EC 24 8B 45 08 57 8B F9 89 7D F4 85 C0 0F 84" ) ).as < CopyToFn > ( );
    CopyTo ( this, other, iRemapping );
}