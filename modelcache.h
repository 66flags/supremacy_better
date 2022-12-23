#pragma once

class CModelCache {
public:
	enum indices : size_t {
		BEGINLOCK = 32,
		ENDLOCK = 33
	};
public:
    __forceinline void BeginLock ( ) {
        util::get_method < void ( __thiscall * )( void * ) > ( this, BEGINLOCK )( this );
    }

    __forceinline void EndLock ( ) {
        util::get_method < void ( __thiscall * )( void * ) > ( this, ENDLOCK )( this );
    }
};