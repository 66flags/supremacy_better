#pragma once

class InputPrediction {
public:
	float m_curtime;
	float m_frametime;
	bool m_has_error;
	bool m_first_time_predicted;
	CMoveData data;
	bool m_in_prediction;
	float m_cycle;
	float m_weapon_cycle = 0.f;
	float m_weapon_sequence = 0.f;
	float m_weapon_anim_time = 0.f;

	struct {
		float m_old_velocity_modifier;
		float m_velocity_modifier;
	} stored;
public:
	void update( );
	void ForceUpdate ( bool error );
	void run( );
	void restore( );
};

extern InputPrediction g_inputpred;