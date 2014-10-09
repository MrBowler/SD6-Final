#ifndef include_Tank
#define include_Tank
#pragma once

//-----------------------------------------------------------------------------------------------
#include <string>
#include "../Engine/Color.hpp"
#include "../Engine/Camera.hpp"
#include "../Engine/Vector2.hpp"
#include "../Engine/DebugGraphics.hpp"


//-----------------------------------------------------------------------------------------------
class Tank
{
public:
	Tank();
	void FireLaser();
	void SetColor();
	void Update( float deltaSeconds );
	void Render();

	unsigned char		m_playerID;
	unsigned char		m_health;
	unsigned char		m_score;
	Color				m_color;
	float				m_yawOrientationDeg;
	Vector3				m_currentPosition;
	Vector2				m_lastUpdatePosition;
	Vector2				m_velocity;
	Vector2				m_acceleration;
	Camera				m_firstPersonCamera;
	double				m_timeOfLastUpdate;
	DebugGraphicsAABB3	m_tankBase;
	DebugGraphicsAABB3	m_tankBarrel;
	DebugGraphicsLine	m_laser;
};


#endif // include_Tank