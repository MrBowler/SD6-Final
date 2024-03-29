#include "Mouse.hpp"
#include "OpenGLRenderer.hpp"


//-----------------------------------------------------------------------------------------------
Mouse::Mouse()
	: m_leftButtonDown( false )
	, m_rightButtonDown( false )
	, m_isIdle( false )
	, m_secondsSinceLastMovement( 0.f )
	, m_screenPosition( 0.f, 0.f )
	, m_windowPosition( 0.f, 0.f )
	, m_cursorTexture( nullptr )
{
	
}


//-----------------------------------------------------------------------------------------------
Mouse::Mouse( const std::string& cursorTextureFileName )
	: m_leftButtonDown( false )
	, m_rightButtonDown( false )
	, m_isIdle( false )
	, m_secondsSinceLastMovement( 0.f )
	, m_screenPosition( 0.f, 0.f )
	, m_windowPosition( 0.f, 0.f )
	, m_cursorTexture( Texture::CreateOrGetTexture( cursorTextureFileName ) )
{

}


//-----------------------------------------------------------------------------------------------
void Mouse::SetHWND( HWND hWnd )
{
	m_hWnd = hWnd;
}


//-----------------------------------------------------------------------------------------------
bool Mouse::IsLeftButtonClicked() const
{
	if( ( GetKeyState( VK_LBUTTON ) & 0x8000 ) && !m_leftButtonDown )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------
bool Mouse::IsRightButtonClicked() const
{
	if( ( GetKeyState( VK_RBUTTON ) & 0x8000 ) && !m_rightButtonDown )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------
bool Mouse::IsLeftButtonDown() const
{
	if( GetKeyState( VK_LBUTTON ) & 0x8000 )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------
bool Mouse::IsRightButtonDown() const
{
	if( GetKeyState( VK_RBUTTON ) & 0x8000 )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------
bool Mouse::IsLeftButtonReleased() const
{
	if( !( GetKeyState( VK_LBUTTON ) & 0x8000 ) && m_leftButtonDown )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------
bool Mouse::IsRightButtonReleased() const
{
	if( !( GetKeyState( VK_RBUTTON ) & 0x8000 ) && m_rightButtonDown )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------
bool Mouse::IsIdle() const
{
	return m_isIdle;
}


//-----------------------------------------------------------------------------------------------
Vector2 Mouse::GetCursorPositionOnScreen() const
{
	return m_screenPosition;
}


//-----------------------------------------------------------------------------------------------
Vector2 Mouse::GetCursorPositionOnWindow() const
{
	return m_windowPosition;
}


//-----------------------------------------------------------------------------------------------
void Mouse::SetCursorPositionOnScreen( const Vector2& setPosition )
{
	SetCursorPos( (int) setPosition.x, (int) setPosition.y );
}


//-----------------------------------------------------------------------------------------------
void Mouse::SetCursorPositionOnWindow( const Vector2& setPosition )
{
	RECT windowRect;
	GetWindowRect( m_hWnd, &windowRect );

	POINT windowPoint;
	windowPoint.x = (LONG) m_windowPosition.x;
	windowPoint.y = ( windowRect.bottom - windowRect.top - 45 ) - (LONG) m_windowPosition.y;
	float inverseYPosition = (float) windowPoint.y;
	ClientToScreen( m_hWnd, &windowPoint );

	int positionX = (int) ( setPosition.x + ( m_screenPosition.x - m_windowPosition.x ) );
	int positionY = (int) ( ( windowPoint.y - inverseYPosition ) + ( windowRect.bottom - windowRect.top - 45 ) - setPosition.y );
	
	SetCursorPos( positionX, positionY );
}


//-----------------------------------------------------------------------------------------------
void Mouse::Update( float deltaSeconds)
{
	if( IsLeftButtonDown() )
		m_leftButtonDown = true;
	else
		m_leftButtonDown = false;

	if( IsRightButtonDown() )
		m_rightButtonDown = true;
	else
		m_rightButtonDown = false;

	Vector2 prevPos = m_screenPosition;

	UpdateCursorScreenPosition();
	UpdateCursorWindowPosition();

	if( m_screenPosition == prevPos )
	{
		m_secondsSinceLastMovement += deltaSeconds;
	}
	else
	{
		m_secondsSinceLastMovement = 0.f;
	}

	if( m_secondsSinceLastMovement > SECONDS_BEFORE_MOUSE_IS_IDLE )
	{
		m_isIdle = true;
	}
	else
	{
		m_isIdle = false;
	}
}


//-----------------------------------------------------------------------------------------------
void Mouse::RenderCursor()
{
	if( m_cursorTexture == nullptr )
		return;

	OpenGLRenderer::PushMatrix();

	OpenGLRenderer::EnableTexture2D();
	OpenGLRenderer::BindTexture2D( m_cursorTexture->m_openglTextureID );
	OpenGLRenderer::Translatef( m_windowPosition.x, m_windowPosition.y, 0.f );
	OpenGLRenderer::SetColor3f( 1.f, 1.f, 1.f );

	Vector2 textureSize( (float) m_cursorTexture->m_size.x, (float) m_cursorTexture->m_size.y );

	OpenGLRenderer::BeginRender( QUADS );
	{
		OpenGLRenderer::SetTexCoords2f( 0.f, 1.f );
		OpenGLRenderer::SetVertex2f( 0.f, -textureSize.y );

		OpenGLRenderer::SetTexCoords2f( 1.f, 1.f );
		OpenGLRenderer::SetVertex2f( textureSize.x, -textureSize.y );

		OpenGLRenderer::SetTexCoords2f( 1.f, 0.f );
		OpenGLRenderer::SetVertex2f( textureSize.x, 0.f );

		OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
		OpenGLRenderer::SetVertex2f( 0.f, 0.f );
	}
	OpenGLRenderer::EndRender();

	OpenGLRenderer::BindTexture2D( 0 );
	OpenGLRenderer::DisableTexture2D();

	OpenGLRenderer::PopMatrix();
}


//-----------------------------------------------------------------------------------------------
void Mouse::UpdateCursorScreenPosition()
{
	POINT cursorPoint;

	if( GetCursorPos( &cursorPoint ) )
	{
		m_screenPosition.x = (float) cursorPoint.x;
		m_screenPosition.y = (float) cursorPoint.y;
	}
}


//-----------------------------------------------------------------------------------------------
void Mouse::UpdateCursorWindowPosition()
{
	POINT screenMouseCoordsInts;
	GetCursorPos( &screenMouseCoordsInts );

	POINT clientPosition = screenMouseCoordsInts;
	ScreenToClient( m_hWnd, &clientPosition );

	RECT windowRect;
	GetWindowRect( m_hWnd, &windowRect );

	Vector2 invertedYClientMouseCoords( (float) clientPosition.x, (float) ( windowRect.bottom - windowRect.top - clientPosition.y ) );
	invertedYClientMouseCoords.y -= 45.f;

	m_windowPosition = invertedYClientMouseCoords;
}