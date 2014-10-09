#ifndef include_World
#define include_World
#pragma once

//-----------------------------------------------------------------------------------------------
#include <set>
#include <string>
#include <vector>
#include "Tank.hpp"
#include "Color3b.hpp"
#include "GameInfo.hpp"
#include "UDPClient.hpp"
#include "GameCommon.hpp"
#include "FinalPacket.hpp"
#include "../Engine/Clock.hpp"
#include "../Engine/Mouse.hpp"
#include "../Engine/Button.hpp"
#include "../Engine/Camera.hpp"
#include "../Engine/Vertex.hpp"
#include "../Engine/pugixml.hpp"
#include "../Engine/Texture.hpp"
#include "../Engine/Vector2.hpp"
#include "../Engine/Keyboard.hpp"
#include "../Engine/Material.hpp"
#include "../Engine/BitmapFont.hpp"
#include "../Engine/DebugGraphics.hpp"
#include "../Engine/OpenGLRenderer.hpp"
#include "../Engine/NamedProperties.hpp"
#include "../Engine/ConsoleCommandArgs.hpp"
#include "../Engine/XMLParsingFunctions.hpp"
#include "../Engine/ErrorWarningAssertions.hpp"


//-----------------------------------------------------------------------------------------------
const int MAX_NUMBER_OF_ROOMS = 8;
const int MAX_NUMBER_OF_PLAYERS_PER_ROOM = 8;
const float ARENA_FLOOR_SIZE_X = 500.f;
const float ARENA_FLOOR_SIZE_Y = 500.f;
const float ARENA_WALL_WIDTH = 20.f;
const float ARENA_WALL_HEIGHT = 100.f;
const float ARENA_SCALE = 1.f;
const float HUD_FONT_CELL_HEIGHT = 50.f;
const float TANK_SPEED_UNITS_PER_SECOND = 100.f;
const float TANK_ROTATION_DEGREES_PER_SECOND = 90.f;
const double SECONDS_BEFORE_RESEND_GUARANTEED_PACKET = 0.25;
const double SECONDS_BEFORE_SEND_UPDATE_PACKET = 0.05;
const double SECONDS_BEFORE_TIMEOUT_REMOVE = 5.0;
const unsigned short PORT_NUMBER = 5000;
//const std::string IP_ADDRESS = "129.119.247.159";
const std::string IP_ADDRESS = "127.0.0.1";
const std::string JOIN_BUTTON_UP = "Data/Images/JoinButtonUp.png";
const std::string JOIN_BUTTON_OVER = "Data/Images/JoinButtonOver.png";
const std::string JOIN_BUTTON_DOWN = "Data/Images/JoinButtonDown.png";
const std::string WALL_TEXTURE_FILE_NAME = "Data/Images/Wall.png";
const std::string FLOOR_TEXTURE_FILE_NAME = "Data/Images/Floor.png";
const std::string HUD_FONT_GLYPH_SHEET_FILE_NAME = "Data/Fonts/MainFont_EN_00.png";
const std::string HUD_FONT_META_DATA_FILE_NAME = "Data/Fonts/MainFont_EN.FontDef.xml";


//-----------------------------------------------------------------------------------------------
class World
{
public:
	World( float worldWidth, float worldHeight );
	void Initialize();
	void Destruct();
	void ChangeIPAddress( const std::string& ipAddrString );
	void ChangePortNumber( unsigned short portNumber );
	bool IsInGame();
	Camera GetFirstPersonCamera();
	void Update( float deltaSeconds, const Keyboard& keyboard, const Mouse& mouse );
	void RenderObjects3D();
	void RenderObjects2D();

private:
	void InitializeButtons();
	Color GetIndividualTankColor( unsigned char playerID );
	void JoinOrCreateRoom( const NamedProperties& params );
	void SendPacket( const FinalPacket& packet );
	void SendJoinLobbyPacket();
	void SendKeepAlivePacket();
	void SendCreateGamePacket( unsigned char roomNumber );
	void SendJoinGamePacket( unsigned char roomNumber );
	void SendUpdate();
	void SendGameUpdate();
	void SendFire();
	void ProcessAckPackets( const FinalPacket& ackPacket );
	void ProcessNakPackets( const FinalPacket& nakPacket );
	void UpdateLobby( const FinalPacket& lobbyUpdatePacket );
	void UpdateTank( const FinalPacket& updatePacket );
	void UpdateTankHit( const FinalPacket& hitPacket );
	void UpdateTankFire( const FinalPacket& firePacket );
	void RespawnTank( const FinalPacket& respawnPacket );
	void ReturnToLobby( const FinalPacket& lobbyReturnPacket );
	void UpdateFromInput( const Keyboard& keyboard, const Mouse& mouse, float deltaSeconds );
	void RemoveTimedOutPlayers();
	void ApplyDeadReckoning();
	void ResetGame( const FinalPacket& resetPacket );
	void ShowButtons();
	void HideButtons();
	void ReceivePackets();
	void ResendGuaranteedPackets();
	void RenderLobby();
	void RenderWorld();
	void RenderFloor();
	void RenderWalls();
	void RenderTanks();

	Camera						m_camera;
	Vector2						m_size;
	Texture*					m_wallTexture;
	Texture*					m_floorTexture;
	UDPClient					m_client;
	BitmapFont					m_hudFont;
	Tank*						m_mainPlayer;
	Button						m_roomButtons[ MAX_NUMBER_OF_ROOMS ];
	unsigned int				m_nextPacketNumber;
	bool						m_isInLobby;
	bool						m_isInGame;
	double						m_timeOfLastUpdateSend;
	char						m_playersPerRoom[ MAX_NUMBER_OF_ROOMS ];
	Vector2						m_mainPlayerVelocity;
	float						m_mainPlayerOrientation;
	std::vector< Tank* >		m_tanks;
	std::vector< Color >		m_tankColors;
	std::vector< FinalPacket >	m_sentPackets;
};


#endif // include_World