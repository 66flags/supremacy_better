#include "includes.h"

LRESULT WINAPI Hooks::WndProc( HWND wnd, uint32_t msg, WPARAM wp, LPARAM lp ) {
	bool allow_input_processing = false;

	if ( g_gui.m_open ) {
		allow_input_processing = true;

		switch ( msg ) {
		case WM_MOUSEWHEEL: {
		} break;
		case WM_KEYDOWN: {
			if ( LOWORD ( wp ) == VK_SPACE || LOWORD ( wp ) == 0x57 || LOWORD ( wp ) == 0x41 || LOWORD ( wp ) == 0x53 || LOWORD ( wp ) == 0x44 )
				allow_input_processing = false;
		} break;
		case WM_KEYUP: {
			if ( LOWORD ( wp ) == VK_SPACE || LOWORD ( wp ) == 0x57 || LOWORD ( wp ) == 0x41 || LOWORD ( wp ) == 0x53 || LOWORD ( wp ) == 0x44 )
				allow_input_processing = false;
		} break;
		}
	}
	else
		allow_input_processing = false;

	switch( msg ) {
	case WM_LBUTTONDOWN:
		g_input.SetDown( VK_LBUTTON );
		break;

	case WM_LBUTTONUP:
		g_input.SetUp( VK_LBUTTON );
		break;

	case WM_RBUTTONDOWN:
		g_input.SetDown( VK_RBUTTON );
		break;

	case WM_RBUTTONUP:
		g_input.SetUp( VK_RBUTTON );
		break;

	case WM_MBUTTONDOWN:
		g_input.SetDown( VK_MBUTTON );
		break;

	case WM_MBUTTONUP:
		g_input.SetUp( VK_MBUTTON );
		break;

	case WM_XBUTTONDOWN:
		if( GET_XBUTTON_WPARAM( wp ) == XBUTTON1 )
			g_input.SetDown( VK_XBUTTON1 );

		else if( GET_XBUTTON_WPARAM( wp ) == XBUTTON2 )
			g_input.SetDown( VK_XBUTTON2 );

		break;

	case WM_XBUTTONUP:
		if( GET_XBUTTON_WPARAM( wp ) == XBUTTON1 )
			g_input.SetUp( VK_XBUTTON1 );

		else if( GET_XBUTTON_WPARAM( wp ) == XBUTTON2 )
			g_input.SetUp( VK_XBUTTON2 );

		break;

	case WM_KEYDOWN:
		if( ( size_t )wp < g_input.m_keys.size( ) )
			g_input.SetDown( wp );

		break;

	case WM_KEYUP:
		if( ( size_t )wp < g_input.m_keys.size( ) )
			g_input.SetUp( wp );

		break;

	case WM_SYSKEYDOWN:
		if( wp == VK_MENU )
			g_input.SetDown( VK_MENU );

		break;

	case WM_SYSKEYUP:
		if( wp == VK_MENU )
			g_input.SetUp( VK_MENU );

		break;

	case WM_CHAR:
		switch( wp ) {
		case VK_BACK:
			if( !g_input.m_buffer.empty( ) )
				g_input.m_buffer.pop_back( );
			break;

		case VK_ESCAPE:
		case VK_TAB:
		case VK_RETURN:
			break;

		default:
			if( g_input.m_use_str || std::isdigit( static_cast< char >( wp ) ) )
				g_input.m_buffer += static_cast< char >( wp );

			break;
		}

		break;

	default:
		break;
	}

	if ( allow_input_processing )
		return true;
	
	return g_winapi.CallWindowProcA( g_hooks.m_old_wndproc, wnd, msg, wp, lp );
}