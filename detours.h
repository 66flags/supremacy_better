#pragma once

namespace detours {
	bool init ( );
	
	void __fastcall Paint ( void *ecx, void *edx, PaintModes_t mode );
	void __fastcall PacketEnd ( void *ecx, void *edx );
	bool __fastcall ShouldSkipAnimationFrame ( void *ecx, void *edx, float time );
	void __fastcall CheckForSequenceChange ( void *ecx, void *edx, void *hdr, int sequence, bool force_new_sequence, bool interpolate );
	void __fastcall StandardBlendingRules ( void *ecx, void *edx, void *hdr, void *pos, void *q, float current_time, int bone_mask );
	void __fastcall CalcView ( void *ecx, void *edx, vec3_t &eye_origin, ang_t &eye_angles, float &z_near, float &z_far, float &fov );

	namespace old {
		inline decltype ( &detours::Paint ) Paint;
		inline decltype ( &detours::PacketEnd ) PacketEnd;
		inline decltype ( &detours::ShouldSkipAnimationFrame ) ShouldSkipAnimationFrame;
		inline decltype ( &detours::CheckForSequenceChange ) CheckForSequenceChange;
		inline decltype ( &detours::StandardBlendingRules ) StandardBlendingRules;
		inline decltype ( &detours::CalcView ) CalcView;
	}
}