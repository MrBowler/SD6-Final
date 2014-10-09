#include "Tank.hpp"


//-----------------------------------------------------------------------------------------------
Tank::Tank()
	: m_health( 1 )
	, m_score( 0 )
	, m_yawOrientationDeg( 0.f )
{
	m_tankBase = DebugGraphicsAABB3( Vector3( 0.f, 0.f, 5.f ), 10.f, 10.f, 10.f, m_color, m_color );
	m_tankBarrel = DebugGraphicsAABB3( Vector3( 4.f, 0.f, 5.f ), 10.f, 2.5f, 1.75f, m_color, m_color );
	m_laser = DebugGraphicsLine( Vector3( 4.f, 0.f, 5.f ), Vector3( 504.f, 0.f, 5.f ), m_color, m_color, 0.25f );
}


//-----------------------------------------------------------------------------------------------
void Tank::FireLaser()
{
	m_laser = DebugGraphicsLine( Vector3( 4.f, 0.f, 5.f ), Vector3( 504.f, 0.f, 5.f ), m_color, m_color, 0.25f );
}


//-----------------------------------------------------------------------------------------------
void Tank::SetColor()
{
	m_tankBase = DebugGraphicsAABB3( Vector3( 0.f, 0.f, 5.f ), 10.f, 10.f, 10.f, m_color, m_color );
	m_tankBarrel = DebugGraphicsAABB3( Vector3( 4.f, 0.f, 5.f ), 10.f, 2.5f, 1.75f, m_color, m_color );
	m_laser = DebugGraphicsLine( Vector3( 4.f, 0.f, 5.f ), Vector3( 504.f, 0.f, 5.f ), m_color, m_color, 0.25f );
}


//-----------------------------------------------------------------------------------------------
void Tank::Update( float deltaSeconds )
{
	m_firstPersonCamera.m_position = m_currentPosition;
	m_firstPersonCamera.m_position.z += 5.f;
	m_firstPersonCamera.m_orientation.yaw = m_yawOrientationDeg;
	m_laser.Update( deltaSeconds );
}


//-----------------------------------------------------------------------------------------------
void Tank::Render()
{
	OpenGLRenderer::PushMatrix();

	OpenGLRenderer::Translatef( m_currentPosition.x, m_currentPosition.y, m_currentPosition.z );
	OpenGLRenderer::Rotatef( m_yawOrientationDeg, 0.f, 0.f, 1.f );
	OpenGLRenderer::LoadMatrix();

	m_tankBase.Render();
	m_tankBarrel.Render();
	m_laser.Render();

	OpenGLRenderer::PopMatrix();
}