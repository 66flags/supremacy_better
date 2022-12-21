#pragma once

namespace detours {
	bool init ( );

	int __fastcall BaseInterpolatePart1 ( void *ecx, void *edx, float &curtime, vec3_t &old_origin, ang_t &old_angs, int &no_more_changes );	
	void __fastcall Paint ( void *ecx, void *edx, PaintModes_t mode );
	void __fastcall ModifyEyePosition ( void *ecx, void *edx, vec3_t &eye_pos );
	void __fastcall PacketEnd ( void *ecx, void *edx );
	bool __fastcall ShouldSkipAnimationFrame ( void *ecx, void *edx );
	void __fastcall CheckForSequenceChange ( void *ecx, void *edx, void *hdr, int sequence, bool force_new_sequence, bool interpolate );
	void __fastcall StandardBlendingRules ( void *ecx, void *edx, void *hdr, void *pos, void *q, float current_time, int bone_mask );

	namespace old {
		inline decltype ( &detours::Paint ) Paint;
		inline decltype ( &detours::PacketEnd ) PacketEnd;
		inline decltype ( &detours::ShouldSkipAnimationFrame ) ShouldSkipAnimationFrame;
		inline decltype ( &detours::CheckForSequenceChange ) CheckForSequenceChange;
		inline decltype ( &detours::StandardBlendingRules ) StandardBlendingRules;
		inline decltype ( &detours::ModifyEyePosition ) ModifyEyePosition;
		inline decltype ( &detours::BaseInterpolatePart1 ) BaseInterpolatePart1;
	}
}