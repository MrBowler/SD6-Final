#ifndef include_Mouse
#define include_Mouse
#pragma once

//-----------------------------------------------------------------------------------------------
#include "Texture.hpp"
#include "Vector2.hpp"
#include "EngineCommon.hpp"


//-----------------------------------------------------------------------------------------------
const float SECONDS_BEFORE_MOUSE_IS_IDLE = 1.f;


//-----------------------------------------------------------------------------------------------
class Mouse
{
public:
	Mouse();
	Mouse( const std::string& cursorTextureFileName );
	void SetHWND( HWND hWnd );
	bool IsLeftButtonClicked() const;
	bool IsRightButtonClicked() const;
	bool IsLeftButtonDown() const;
	bool IsRightButtonDown() const;
	bool IsLeftButtonReleased() const;
	bool IsRightButtonReleased() const;
	bool IsIdle() const;
	Vector2 GetCursorPositionOnScreen() const;
	Vector2 GetCursorPositionOnWindow() const;
	void SetCursorPositionOnScreen( const Vector2& setPosition );
	void SetCursorPositionOnWindow( const Vector2& setPosition );
	void Update( float deltaSeconds );
	void RenderCursor();

private:
	void UpdateCursorScreenPosition();
	void UpdateCursorWindowPosition();

	bool		m_leftButtonDown;
	bool		m_rightButtonDown;
	bool		m_isIdle;
	float		m_secondsSinceLastMovement;
	Vector2		m_screenPosition;
	Vector2		m_windowPosition;
	Texture*	m_cursorTexture;
	HWND		m_hWnd;
};


#endif // include_Mouse