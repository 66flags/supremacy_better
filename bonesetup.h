#pragma once

class Bones {

public:
	bool m_running;

public:
	bool setup( Player* player, BoneArray* out, float curtime, LagRecord *record );
	bool BuildBones( Player* target, int mask, BoneArray* out, LagRecord* record );
};

extern Bones g_bones;