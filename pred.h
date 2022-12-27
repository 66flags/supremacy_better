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

	struct {
		float m_old_velocity_modifier;
		float m_velocity_modifier;
	} stored;
public:
	void update( );
	void ForceUpdate ( bool error );
	void run( );
	void FixViewmodel ( bool store );
	void restore( );
};

extern InputPrediction g_inputpred;