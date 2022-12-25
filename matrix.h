#pragma once

class matrix3x4a_t;

struct matrix3x4_t {
	matrix3x4_t ( ) { }
	matrix3x4_t (
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23 ) {
		m_flMatVal [ 0 ][ 0 ] = m00;	m_flMatVal [ 0 ][ 1 ] = m01; m_flMatVal [ 0 ][ 2 ] = m02; m_flMatVal [ 0 ][ 3 ] = m03;
		m_flMatVal [ 1 ][ 0 ] = m10;	m_flMatVal [ 1 ][ 1 ] = m11; m_flMatVal [ 1 ][ 2 ] = m12; m_flMatVal [ 1 ][ 3 ] = m13;
		m_flMatVal [ 2 ][ 0 ] = m20;	m_flMatVal [ 2 ][ 1 ] = m21; m_flMatVal [ 2 ][ 2 ] = m22; m_flMatVal [ 2 ][ 3 ] = m23;
	}

	/// Creates a matrix where the X axis = forward the Y axis = left, and the Z axis = up
	void InitXYZ ( const vec3_t &xAxis, const vec3_t &yAxis, const vec3_t &zAxis, const vec3_t &vecOrigin ) {
		m_flMatVal [ 0 ][ 0 ] = xAxis.x; m_flMatVal [ 0 ][ 1 ] = yAxis.x; m_flMatVal [ 0 ][ 2 ] = zAxis.x; m_flMatVal [ 0 ][ 3 ] = vecOrigin.x;
		m_flMatVal [ 1 ][ 0 ] = xAxis.y; m_flMatVal [ 1 ][ 1 ] = yAxis.y; m_flMatVal [ 1 ][ 2 ] = zAxis.y; m_flMatVal [ 1 ][ 3 ] = vecOrigin.y;
		m_flMatVal [ 2 ][ 0 ] = xAxis.z; m_flMatVal [ 2 ][ 1 ] = yAxis.z; m_flMatVal [ 2 ][ 2 ] = zAxis.z; m_flMatVal [ 2 ][ 3 ] = vecOrigin.z;
	}

	//-----------------------------------------------------------------------------
	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	//-----------------------------------------------------------------------------
	void Init ( const vec3_t &xAxis, const vec3_t &yAxis, const vec3_t &zAxis, const vec3_t &vecOrigin ) {
		m_flMatVal [ 0 ][ 0 ] = xAxis.x; m_flMatVal [ 0 ][ 1 ] = yAxis.x; m_flMatVal [ 0 ][ 2 ] = zAxis.x; m_flMatVal [ 0 ][ 3 ] = vecOrigin.x;
		m_flMatVal [ 1 ][ 0 ] = xAxis.y; m_flMatVal [ 1 ][ 1 ] = yAxis.y; m_flMatVal [ 1 ][ 2 ] = zAxis.y; m_flMatVal [ 1 ][ 3 ] = vecOrigin.y;
		m_flMatVal [ 2 ][ 0 ] = xAxis.z; m_flMatVal [ 2 ][ 1 ] = yAxis.z; m_flMatVal [ 2 ][ 2 ] = zAxis.z; m_flMatVal [ 2 ][ 3 ] = vecOrigin.z;
	}

	//-----------------------------------------------------------------------------
	// Creates a matrix where the X axis = forward
	// the Y axis = left, and the Z axis = up
	//-----------------------------------------------------------------------------
	matrix3x4_t ( const vec3_t &xAxis, const vec3_t &yAxis, const vec3_t &zAxis, const vec3_t &vecOrigin ) {
		Init ( xAxis, yAxis, zAxis, vecOrigin );
	}

	inline void SetToIdentity ( );

	/// multiply the scale/rot part of the matrix by a constant. This doesn't init the matrix ,
	/// just scale in place. So if you want to construct a scaling matrix, init to identity and
	/// then call this.
	FORCEINLINE void ScaleUpper3x3Matrix ( float flScale );

	/// modify the origin
	inline void SetOrigin ( vec3_t const &p ) {
		m_flMatVal [ 0 ][ 3 ] = p.x;
		m_flMatVal [ 1 ][ 3 ] = p.y;
		m_flMatVal [ 2 ][ 3 ] = p.z;
	}

	/// return the origin
	inline vec3_t GetOrigin ( void ) const {
		vec3_t vecRet ( m_flMatVal [ 0 ][ 3 ], m_flMatVal [ 1 ][ 3 ], m_flMatVal [ 2 ][ 3 ] );
		return vecRet;
	}

	inline void Invalidate ( void ) {
		for ( int i = 0; i < 3; i++ ) {
			for ( int j = 0; j < 4; j++ ) {
				m_flMatVal [ i ][ j ] = 0xFFFFFFF;
			}
		}
	}

	/// check all components for invalid floating point values
	inline bool IsValid ( void ) const {
		for ( int i = 0; i < 3; i++ ) {
			for ( int j = 0; j < 4; j++ ) {
				if ( !std::isfinite ( m_flMatVal [ i ][ j ] ) )
					return false;
			}
		}
		return true;
	}

	bool operator==( const matrix3x4_t &other ) const {
		return memcmp ( this, &other, sizeof ( matrix3x4_t ) ) == 0;
	}

	bool operator!=( const matrix3x4_t &other ) const {
		return memcmp ( this, &other, sizeof ( matrix3x4_t ) ) != 0;
	}

	__forceinline matrix3x4_t operator* ( matrix3x4_t other ) const {
		matrix3x4_t ret;
		math::ConcatTransforms ( *this, other, ret );
		return ret;
	}

	// get float* from 

	inline bool IsEqualTo ( const matrix3x4_t &other, float flTolerance = 1e-5f ) const;

	inline void InverseTR ( matrix3x4_t &out ) const;
	inline matrix3x4_t InverseTR ( ) const;


	float *operator[]( int i ) { assert ( ( i >= 0 ) && ( i < 3 ) ); return m_flMatVal [ i ]; }
	const float *operator[]( int i ) const { assert ( ( i >= 0 ) && ( i < 3 ) ); return m_flMatVal [ i ]; }
	float *Base ( ) { return &m_flMatVal [ 0 ][ 0 ]; }
	const float *Base ( ) const { return &m_flMatVal [ 0 ][ 0 ]; }

	float m_flMatVal [ 3 ][ 4 ];
};

class BoneArray : public matrix3x4_t {
public:
	bool get_bone( vec3_t & out, int bone = 0 ) {
		if( bone < 0 || bone >= 128 )
			return false;

		matrix3x4_t* bone_matrix = &this[ bone ];

		if( !bone_matrix )
			return false;

		out = { bone_matrix->m_flMatVal[ 0 ][ 3 ], bone_matrix->m_flMatVal[ 1 ][ 3 ], bone_matrix->m_flMatVal[ 2 ][ 3 ] };

		return true;
	}
};

class __declspec( align( 16 ) ) matrix3x4a_t : public matrix3x4_t {
public:
	/*
	matrix3x4a_t() { if (((size_t)Base()) % 16 != 0) { Error( "matrix3x4a_t missaligned" ); } }
	*/
	matrix3x4a_t ( const matrix3x4_t &src ) { *this = src; };
	matrix3x4a_t &operator=( const matrix3x4_t &src ) { memcpy ( Base ( ), src.Base ( ), sizeof ( float ) * 3 * 4 ); return *this; };

	matrix3x4a_t (
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23 ) {
		assert ( ( ( size_t ) Base ( ) & 0xf ) == 0 );
		m_flMatVal [ 0 ][ 0 ] = m00;	m_flMatVal [ 0 ][ 1 ] = m01; m_flMatVal [ 0 ][ 2 ] = m02; m_flMatVal [ 0 ][ 3 ] = m03;
		m_flMatVal [ 1 ][ 0 ] = m10;	m_flMatVal [ 1 ][ 1 ] = m11; m_flMatVal [ 1 ][ 2 ] = m12; m_flMatVal [ 1 ][ 3 ] = m13;
		m_flMatVal [ 2 ][ 0 ] = m20;	m_flMatVal [ 2 ][ 1 ] = m21; m_flMatVal [ 2 ][ 2 ] = m22; m_flMatVal [ 2 ][ 3 ] = m23;
	}
	matrix3x4a_t ( ) { }

	float *operator[]( int i ) { assert ( ( i >= 0 ) && ( i < 3 ) ); return m_flMatVal [ i ]; }
	const float *operator[]( int i ) const { assert ( ( i >= 0 ) && ( i < 3 ) ); return m_flMatVal [ i ]; }
	float *Base ( ) { return &m_flMatVal [ 0 ][ 0 ]; }
	const float *Base ( ) const { return &m_flMatVal [ 0 ][ 0 ]; }

};

class VMatrix {
public:
	__forceinline float* operator[]( int i ) {
		return m[ i ];
	}
	__forceinline const float* operator[]( int i ) const {
		return m[ i ];
	}

	__forceinline float* Base( ) {
		return &m[ 0 ][ 0 ];
	}
	__forceinline const float* Base( ) const {
		return &m[ 0 ][ 0 ];
	}

public:
	float m[ 4 ][ 4 ];
};