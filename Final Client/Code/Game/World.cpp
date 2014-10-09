#include "World.hpp"
#include "../Engine/Time.hpp"
#include "../Engine/EventSystem.hpp"
#include "../Engine/DeveloperConsole.hpp"
#include "../Engine/NewMacroDef.hpp"

//-----------------------------------------------------------------------------------------------
World::World( float worldWidth, float worldHeight )
	: m_size( worldWidth, worldHeight )
	, m_mainPlayer( nullptr )
	, m_nextPacketNumber( 0 )
	, m_isInLobby( false )
	, m_isInGame( false )
{

}


//-----------------------------------------------------------------------------------------------
void World::Initialize()
{
	InitializeTime();
	m_client.ConnectToServer( IP_ADDRESS, PORT_NUMBER );
	m_timeOfLastUpdateSend = GetCurrentTimeSeconds();

	m_hudFont = BitmapFont( HUD_FONT_GLYPH_SHEET_FILE_NAME, HUD_FONT_META_DATA_FILE_NAME );
	InitializeButtons();

	m_wallTexture = Texture::CreateOrGetTexture( WALL_TEXTURE_FILE_NAME );
	m_floorTexture = Texture::CreateOrGetTexture( FLOOR_TEXTURE_FILE_NAME );

	m_mainPlayer = new Tank();
	m_mainPlayer->m_currentPosition = Vector3( ARENA_FLOOR_SIZE_X * 0.5f, ARENA_FLOOR_SIZE_Y * 0.5f, 1.f );
	m_tanks.push_back( m_mainPlayer );

	m_tankColors.push_back( Color( 1.f, 1.f, 1.f ) );
	m_tankColors.push_back( Color( 1.f, 0.f, 0.f ) );
	m_tankColors.push_back( Color( 0.f, 1.f, 0.f ) );
	m_tankColors.push_back( Color( 0.f, 0.f, 1.f ) );
	m_tankColors.push_back( Color( 1.f, 1.f, 0.f ) );
	m_tankColors.push_back( Color( 1.f, 0.f, 1.f ) );
	m_tankColors.push_back( Color( 0.f, 1.f, 1.f ) );
	m_tankColors.push_back( Color( 0.5f, 0.f, 0.f ) );
	m_tankColors.push_back( Color( 0.5f, 0.f, 0.5f ) );

	SendJoinLobbyPacket();
}


//-----------------------------------------------------------------------------------------------
void World::Destruct()
{
	m_client.DisconnectFromServer();
}


//-----------------------------------------------------------------------------------------------
void World::ChangeIPAddress( const std::string& ipAddrString )
{
	m_client.SetServerIPAddress( ipAddrString );
}


//-----------------------------------------------------------------------------------------------
void World::ChangePortNumber( unsigned short portNumber )
{
	m_client.SetServerPortNumber( portNumber );
}


//-----------------------------------------------------------------------------------------------
bool World::IsInGame()
{
	return m_isInGame;
}


//-----------------------------------------------------------------------------------------------
Camera World::GetFirstPersonCamera()
{
	return m_mainPlayer->m_firstPersonCamera;
}


//-----------------------------------------------------------------------------------------------
void World::Update( float deltaSeconds, const Keyboard& keyboard, const Mouse& mouse )
{
	ReceivePackets();
	RemoveTimedOutPlayers();
	UpdateFromInput( keyboard, mouse, deltaSeconds );
	ApplyDeadReckoning();
	SendUpdate();
	ResendGuaranteedPackets();

	Widget::UpdateAllWidgets( deltaSeconds, mouse, keyboard );
}


//-----------------------------------------------------------------------------------------------
void World::RenderObjects3D()
{
	if( m_isInGame )
	{
		OpenGLRenderer::PushMatrix();

		OpenGLRenderer::Scalef( ARENA_SCALE, ARENA_SCALE, ARENA_SCALE );

		RenderWorld();
		RenderTanks();

		OpenGLRenderer::PopMatrix();
	}
}


//-----------------------------------------------------------------------------------------------
void World::RenderObjects2D()
{
	if( m_isInLobby )
		RenderLobby();

	if( m_isInGame )
	{
		OpenGLRenderer::RenderText( "Score: " + ConvertNumberToString( m_mainPlayer->m_score ), m_hudFont, HUD_FONT_CELL_HEIGHT, Vector2( 50.f, 800.f ) );
	}

	Widget::RenderAllWidgets();
}


//-----------------------------------------------------------------------------------------------
void World::InitializeButtons()
{
	for( unsigned int buttonIndex = 0; buttonIndex < MAX_NUMBER_OF_ROOMS; ++buttonIndex )
	{
		unsigned char roomNumber = (unsigned char) buttonIndex + 1;
		m_roomButtons[ buttonIndex ] = Button( Vector2( 150.f, m_size.y - ( 110.f * roomNumber ) ), 150, 75, "joinOrCreateRoom", JOIN_BUTTON_UP, JOIN_BUTTON_OVER, JOIN_BUTTON_DOWN );
		m_roomButtons[ buttonIndex ].m_params.SetProperty( "roomNumber", roomNumber );
	}

	HideButtons();

	EventSystem::RegisterEventCallbackForObject( "joinOrCreateRoom", this, &World::JoinOrCreateRoom );
}


//-----------------------------------------------------------------------------------------------
Color World::GetIndividualTankColor( unsigned char playerID )
{
	if( playerID > m_tankColors.size() )
		return Color::Black;

	return m_tankColors[ playerID ];
}


//-----------------------------------------------------------------------------------------------
void World::JoinOrCreateRoom( const NamedProperties& params )
{
	unsigned char roomNumber;
	unsigned char numPlayersInRoom;
	params.GetProperty( "roomNumber", roomNumber );

	numPlayersInRoom = m_playersPerRoom[ roomNumber - 1 ];
	if( numPlayersInRoom == MAX_NUMBER_OF_PLAYERS_PER_ROOM )
		return;

	if( numPlayersInRoom == 0 )
		SendCreateGamePacket( roomNumber );
	else
		SendJoinGamePacket( roomNumber );
}


//-----------------------------------------------------------------------------------------------
void World::SendPacket( const FinalPacket& packet )
{
	m_client.SendPacketToServer( (const char*) &packet, sizeof( packet ) );
	++m_nextPacketNumber;

	if( packet.IsGuaranteed() )
	{
		m_sentPackets.push_back( packet );
	}
}


//-----------------------------------------------------------------------------------------------
void World::SendJoinLobbyPacket()
{
	FinalPacket joinLobbyPacket;
	joinLobbyPacket.type = TYPE_JoinRoom;
	joinLobbyPacket.number = m_nextPacketNumber;
	joinLobbyPacket.timestamp = GetCurrentTimeSeconds();
	joinLobbyPacket.data.joining.room = ROOM_Lobby;

	SendPacket( joinLobbyPacket );
}


//-----------------------------------------------------------------------------------------------
void World::SendKeepAlivePacket()
{
	FinalPacket keepAlivePacket;
	keepAlivePacket.type = TYPE_KeepAlive;
	keepAlivePacket.number = m_nextPacketNumber;
	keepAlivePacket.timestamp = GetCurrentTimeSeconds();
	
	SendPacket( keepAlivePacket );
}


//-----------------------------------------------------------------------------------------------
void World::SendCreateGamePacket( unsigned char roomNumber )
{
	FinalPacket createPacket;
	createPacket.type = TYPE_CreateRoom;
	createPacket.number = m_nextPacketNumber;
	createPacket.timestamp = GetCurrentTimeSeconds();
	createPacket.data.creating.room = roomNumber;

	SendPacket( createPacket );
}


//-----------------------------------------------------------------------------------------------
void World::SendJoinGamePacket( unsigned char roomNumber )
{
	FinalPacket joinPacket;
	joinPacket.type = TYPE_JoinRoom;
	joinPacket.number = m_nextPacketNumber;
	joinPacket.timestamp = GetCurrentTimeSeconds();
	joinPacket.data.joining.room = roomNumber;

	SendPacket( joinPacket );
}


//-----------------------------------------------------------------------------------------------
void World::SendUpdate()
{
	if( ( GetCurrentTimeSeconds() - m_timeOfLastUpdateSend ) < SECONDS_BEFORE_SEND_UPDATE_PACKET )
		return;

	if( m_isInLobby )
		SendKeepAlivePacket();
	else if( m_isInGame )
		SendGameUpdate();

	m_timeOfLastUpdateSend = GetCurrentTimeSeconds();
}


//-----------------------------------------------------------------------------------------------
void World::SendGameUpdate()
{
	FinalPacket updatePacket;
	updatePacket.type = TYPE_GameUpdate;
	updatePacket.number = m_nextPacketNumber;
	updatePacket.clientID = m_mainPlayer->m_playerID;
	updatePacket.timestamp = GetCurrentTimeSeconds();
	updatePacket.data.updatedGame.health = 1;
	updatePacket.data.updatedGame.orientationDegrees = m_mainPlayerOrientation;
	updatePacket.data.updatedGame.score = 0;
	updatePacket.data.updatedGame.xAcceleration = m_mainPlayer->m_acceleration.x;
	updatePacket.data.updatedGame.yAcceleration = m_mainPlayer->m_acceleration.y;
	updatePacket.data.updatedGame.xVelocity = m_mainPlayerVelocity.x;
	updatePacket.data.updatedGame.yVelocity = m_mainPlayerVelocity.y;
	updatePacket.data.updatedGame.xPosition = m_mainPlayer->m_currentPosition.x;
	updatePacket.data.updatedGame.yPosition = m_mainPlayer->m_currentPosition.y;

	SendPacket( updatePacket );
}


//-----------------------------------------------------------------------------------------------
void World::SendFire()
{
	m_mainPlayer->FireLaser();

	FinalPacket firePacket;
	firePacket.type = TYPE_Fire;
	firePacket.number = m_nextPacketNumber;
	firePacket.clientID = m_mainPlayer->m_playerID;
	firePacket.timestamp = GetCurrentTimeSeconds();
	firePacket.data.gunfire.instigatorID = m_mainPlayer->m_playerID;

	SendPacket( firePacket );
}


//-----------------------------------------------------------------------------------------------
void World::ProcessAckPackets( const FinalPacket& ackPacket )
{
	for( unsigned int packetIndex = 0; packetIndex < m_sentPackets.size(); ++packetIndex )
	{
		FinalPacket packet = m_sentPackets[ packetIndex ];
		if( packet.number == ackPacket.data.acknowledged.number )
		{
			m_sentPackets.erase( m_sentPackets.begin() + packetIndex );
			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::ProcessNakPackets( const FinalPacket& nakPacket )
{
	for( unsigned int packetIndex = 0; packetIndex < m_sentPackets.size(); ++packetIndex )
	{
		FinalPacket packet = m_sentPackets[ packetIndex ];
		if( packet.number == nakPacket.data.refused.number )
		{
			m_sentPackets.erase( m_sentPackets.begin() + packetIndex );
			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::UpdateLobby( const FinalPacket& lobbyUpdatePacket )
{
	if( !m_isInLobby )
		ShowButtons();

	m_isInLobby = true;
	m_isInGame = false;

	for( unsigned int roomIndex = 0; roomIndex < MAX_NUMBER_OF_ROOMS; ++roomIndex )
	{
		m_playersPerRoom[ roomIndex ] = lobbyUpdatePacket.data.updatedLobby.playersInRoomNumber[ roomIndex ];
	}
}


//-----------------------------------------------------------------------------------------------
void World::UpdateTank( const FinalPacket& updatePacket )
{
	Tank* tank = nullptr;
	for( unsigned int tankIndex = 0; tankIndex < m_tanks.size(); ++tankIndex )
	{
		Tank* checkTank = m_tanks[ tankIndex ];
		if( checkTank->m_playerID == updatePacket.clientID )
		{
			tank = checkTank;
			break;
		}
	}

	if( tank == nullptr )
	{
		tank = new Tank();
		tank->m_playerID = updatePacket.clientID;
		m_tanks.push_back( tank );
	}

	if( tank->m_color == Color::Black )
	{
		tank->m_color = GetIndividualTankColor( tank->m_playerID );
		tank->SetColor();
	}

	Vector2 prevPosition = tank->m_lastUpdatePosition;
	Vector2 prevVelocity = tank->m_velocity;
	Vector2 prevAcceleration = tank->m_acceleration;

	tank->m_currentPosition.x = updatePacket.data.updatedGame.xPosition;
	tank->m_currentPosition.y = updatePacket.data.updatedGame.yPosition;
	tank->m_lastUpdatePosition.x = tank->m_currentPosition.x;
	tank->m_lastUpdatePosition.y = tank->m_currentPosition.y;
	tank->m_velocity.x = updatePacket.data.updatedGame.xVelocity;
	tank->m_velocity.y = updatePacket.data.updatedGame.yVelocity;
	tank->m_acceleration.x = updatePacket.data.updatedGame.xAcceleration;
	tank->m_acceleration.y = updatePacket.data.updatedGame.yAcceleration;
	tank->m_yawOrientationDeg = updatePacket.data.updatedGame.orientationDegrees;
	tank->m_score = updatePacket.data.updatedGame.score;
	tank->m_health = updatePacket.data.updatedGame.health;

	if( prevPosition != tank->m_lastUpdatePosition || prevVelocity != tank->m_velocity || prevAcceleration != tank->m_acceleration )
	{
		tank->m_timeOfLastUpdate = GetCurrentTimeSeconds();
	}
}


//-----------------------------------------------------------------------------------------------
void World::UpdateTankHit( const FinalPacket& hitPacket )
{
	FinalPacket ackPacket;
	ackPacket.type = TYPE_Ack;
	ackPacket.number = m_nextPacketNumber;
	ackPacket.clientID = m_mainPlayer->m_playerID;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.type = TYPE_Hit;
	ackPacket.data.acknowledged.number = hitPacket.number;

	SendPacket( ackPacket );

	for( unsigned int tankIndex = 0; tankIndex < m_tanks.size(); ++tankIndex )
	{
		Tank* tank = m_tanks[ tankIndex ];
		if( tank->m_playerID == hitPacket.data.hit.targetID )
		{
			tank->m_health -= hitPacket.data.hit.damageDealt;
			return;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::UpdateTankFire( const FinalPacket& firePacket )
{
	FinalPacket ackPacket;
	ackPacket.type = TYPE_Ack;
	ackPacket.number = m_nextPacketNumber;
	ackPacket.clientID = m_mainPlayer->m_playerID;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.type = TYPE_Fire;
	ackPacket.data.acknowledged.number = firePacket.number;

	SendPacket( ackPacket );

	for( unsigned int tankIndex = 0; tankIndex < m_tanks.size(); ++tankIndex )
	{
		Tank* tank = m_tanks[ tankIndex ];
		if( tank->m_playerID == firePacket.data.gunfire.instigatorID )
		{
			tank->FireLaser();
			return;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::RespawnTank( const FinalPacket& respawnPacket )
{
	FinalPacket ackPacket;
	ackPacket.type = TYPE_Ack;
	ackPacket.number = m_nextPacketNumber;
	ackPacket.clientID = m_mainPlayer->m_playerID;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.type = TYPE_Respawn;
	ackPacket.data.acknowledged.number = respawnPacket.number;

	SendPacket( ackPacket );

	m_mainPlayer->m_currentPosition.x = respawnPacket.data.respawn.xPosition;
	m_mainPlayer->m_currentPosition.y = respawnPacket.data.respawn.yPosition;
	m_mainPlayer->m_lastUpdatePosition.x = m_mainPlayer->m_currentPosition.x;
	m_mainPlayer->m_lastUpdatePosition.y = m_mainPlayer->m_currentPosition.y;
	m_mainPlayer->m_yawOrientationDeg = respawnPacket.data.respawn.orientationDegrees;
	m_mainPlayerOrientation = m_mainPlayer->m_yawOrientationDeg;
	m_mainPlayer->m_timeOfLastUpdate = GetCurrentTimeSeconds();
}


//-----------------------------------------------------------------------------------------------
void World::ReturnToLobby( const FinalPacket& lobbyReturnPacket )
{
	FinalPacket ackPacket;
	ackPacket.type = TYPE_Ack;
	ackPacket.number = m_nextPacketNumber;
	ackPacket.clientID = m_mainPlayer->m_playerID;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.type = TYPE_ReturnToLobby;
	ackPacket.data.acknowledged.number = lobbyReturnPacket.number;

	SendPacket( ackPacket );

	while( m_tanks.size() > 0 )
	{
		Tank* tank = m_tanks.back();
		if( tank != m_mainPlayer )
			delete tank;

		m_tanks.pop_back();
	}

	m_tanks.push_back( m_mainPlayer );

	m_isInGame = false;
	m_isInLobby = true;
	ShowButtons();
}


//-----------------------------------------------------------------------------------------------
void World::UpdateFromInput( const Keyboard& keyboard, const Mouse&, float deltaSeconds )
{
	if( !m_isInGame )
		return;

	if( keyboard.IsKeyPressedDownAndWasNotBefore( KEY_SPACE ) )
	{
		SendFire();
	}

	if( keyboard.IsKeyPressedDown( KEY_A ) )
	{
		m_mainPlayerOrientation += TANK_ROTATION_DEGREES_PER_SECOND * deltaSeconds;
	}
	else if( keyboard.IsKeyPressedDown( KEY_D ) )
	{
		m_mainPlayerOrientation -= TANK_ROTATION_DEGREES_PER_SECOND * deltaSeconds;
	}

	if( m_mainPlayerOrientation < -180.f )
	{
		m_mainPlayerOrientation += 360.f;
	}
	else if( m_mainPlayerOrientation >= 180.f )
	{
		m_mainPlayerOrientation -= 360.f;
	}

	float speed = 0.f;
	if( keyboard.IsKeyPressedDown( KEY_W ) )
	{
		speed = TANK_SPEED_UNITS_PER_SECOND;
	}
	else if( keyboard.IsKeyPressedDown( KEY_S ) )
	{
		speed = -TANK_SPEED_UNITS_PER_SECOND;
	}

	m_mainPlayerVelocity.x = cos( ConvertDegreesToRadians( m_mainPlayerOrientation ) );
	m_mainPlayerVelocity.y = sin( ConvertDegreesToRadians( m_mainPlayerOrientation ) );
	m_mainPlayerVelocity.Normalize();
	m_mainPlayerVelocity *= speed;
}


//-----------------------------------------------------------------------------------------------
void World::RemoveTimedOutPlayers()
{
	if( !m_isInGame )
		return;

	for( unsigned int tankIndex = 0; tankIndex < m_tanks.size(); ++tankIndex )
	{
		Tank* tank = m_tanks[ tankIndex ];
		if( ( GetCurrentTimeSeconds() - tank->m_timeOfLastUpdate ) > SECONDS_BEFORE_TIMEOUT_REMOVE )
		{
			if( tank == m_mainPlayer )
				continue;

			delete tank;
			m_tanks.erase( m_tanks.begin() + tankIndex );
			--tankIndex;
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::ApplyDeadReckoning()
{
	if( !m_isInGame )
		return;

	for( unsigned int tankIndex = 0; tankIndex < m_tanks.size(); ++tankIndex )
	{
		Tank* tank = m_tanks[ tankIndex ];

		float deltaSeconds = (float) ( GetCurrentTimeSeconds() - tank->m_timeOfLastUpdate );
		tank->m_currentPosition.x = tank->m_lastUpdatePosition.x + ( tank->m_velocity.x * deltaSeconds ) + ( 0.5f * tank->m_acceleration.x * deltaSeconds * deltaSeconds );
		tank->m_currentPosition.y = tank->m_lastUpdatePosition.y + ( tank->m_velocity.y * deltaSeconds ) + ( 0.5f * tank->m_acceleration.y * deltaSeconds * deltaSeconds );
		
		tank->m_currentPosition.x = ClampFloat( tank->m_currentPosition.x, 0.f, ARENA_FLOOR_SIZE_X );
		tank->m_currentPosition.y = ClampFloat( tank->m_currentPosition.y, 0.f, ARENA_FLOOR_SIZE_Y );

		tank->Update( deltaSeconds );
	}
}


//-----------------------------------------------------------------------------------------------
void World::ResetGame( const FinalPacket& resetPacket )
{
	if( !m_isInGame )
		HideButtons();

	m_isInLobby = false;
	m_isInGame = true;

	FinalPacket ackPacket;
	ackPacket.type = TYPE_Ack;
	ackPacket.number = m_nextPacketNumber;
	ackPacket.clientID = resetPacket.data.reset.id;
	ackPacket.timestamp = GetCurrentTimeSeconds();
	ackPacket.data.acknowledged.type = TYPE_GameReset;
	ackPacket.data.acknowledged.number = resetPacket.number;

	SendPacket( ackPacket );

	m_mainPlayer->m_playerID = resetPacket.data.reset.id;
	m_mainPlayer->m_currentPosition.x = resetPacket.data.reset.xPosition;
	m_mainPlayer->m_currentPosition.y = resetPacket.data.reset.yPosition;
	m_mainPlayer->m_lastUpdatePosition.x = m_mainPlayer->m_currentPosition.x;
	m_mainPlayer->m_lastUpdatePosition.y = m_mainPlayer->m_currentPosition.y;
	m_mainPlayer->m_velocity = Vector2( 0.f, 0.f );
	m_mainPlayer->m_yawOrientationDeg = resetPacket.data.reset.orientationDegrees;
	m_mainPlayerOrientation = m_mainPlayer->m_yawOrientationDeg;
	m_mainPlayer->m_timeOfLastUpdate = GetCurrentTimeSeconds();
}


//-----------------------------------------------------------------------------------------------
void World::ShowButtons()
{
	for( unsigned int buttonIndex = 0; buttonIndex < MAX_NUMBER_OF_ROOMS; ++buttonIndex )
	{
		m_roomButtons[ buttonIndex ].SetHidden( false );
	}
}


//-----------------------------------------------------------------------------------------------
void World::HideButtons()
{
	for( unsigned int buttonIndex = 0; buttonIndex < MAX_NUMBER_OF_ROOMS; ++buttonIndex )
	{
		m_roomButtons[ buttonIndex ].SetHidden( true );
	}
}


//-----------------------------------------------------------------------------------------------
void World::ReceivePackets()
{
	FinalPacket packet;
	std::set< FinalPacket > recvPackets;

	while( m_client.ReceivePacketFromServer( (char*) &packet, sizeof( packet ) ) )
	{
		recvPackets.insert( packet );
	}

	std::set< FinalPacket >::iterator setIter;
	for( setIter = recvPackets.begin(); setIter != recvPackets.end(); ++setIter )
	{
		FinalPacket orderedPacket = *setIter;

		if( orderedPacket.type == TYPE_Ack )
		{
			ProcessAckPackets( orderedPacket );
		}
		else if( orderedPacket.type == TYPE_Nack )
		{
			ProcessNakPackets( orderedPacket );
		}
		else if( orderedPacket.type == TYPE_LobbyUpdate )
		{
			UpdateLobby( orderedPacket );
		}
		else if( orderedPacket.type == TYPE_GameReset )
		{
			ResetGame( orderedPacket );
		}
		else if( orderedPacket.type == TYPE_GameUpdate )
		{
			UpdateTank( orderedPacket );
		}
		else if( orderedPacket.type == TYPE_Hit )
		{
			UpdateTankHit( orderedPacket );
		}
		else if( orderedPacket.type == TYPE_Fire )
		{
			UpdateTankFire( orderedPacket );
		}
		else if( orderedPacket.type == TYPE_Respawn )
		{
			RespawnTank( orderedPacket );
		}
		else if( orderedPacket.type == TYPE_ReturnToLobby )
		{
			ReturnToLobby( orderedPacket );
		}
	}
}


//-----------------------------------------------------------------------------------------------
void World::ResendGuaranteedPackets()
{
	std::vector< FinalPacket > resendPackets;

	for( unsigned int packetIndex = 0; packetIndex < m_sentPackets.size(); ++packetIndex )
	{
		FinalPacket packet = m_sentPackets[ packetIndex ];
		if( ( GetCurrentTimeSeconds() - packet.timestamp ) > SECONDS_BEFORE_RESEND_GUARANTEED_PACKET )
		{
			packet.number = m_nextPacketNumber;
			packet.timestamp = GetCurrentTimeSeconds();
			resendPackets.push_back( packet );
			m_sentPackets.erase( m_sentPackets.begin() + packetIndex );
			--packetIndex;
		}
	}

	for( unsigned int packetIndex = 0; packetIndex < resendPackets.size(); ++packetIndex )
	{
		SendPacket( resendPackets[ packetIndex ] );
	}
}


//-----------------------------------------------------------------------------------------------
void World::RenderLobby()
{
	std::string maxNumPlayerString = ConvertNumberToString( MAX_NUMBER_OF_PLAYERS_PER_ROOM );
	for( unsigned int roomIndex = 0; roomIndex < MAX_NUMBER_OF_ROOMS; ++roomIndex )
	{
		char numPlayersInRoom = m_playersPerRoom[ roomIndex ];
		std::string roomText = "Room #" + ConvertNumberToString( roomIndex + 1 ) + ": ";
		if( numPlayersInRoom == 0 )
			roomText += "This room is empty";
		else if( numPlayersInRoom == MAX_NUMBER_OF_PLAYERS_PER_ROOM )
			roomText += "This room is full";
		else
			roomText += ConvertNumberToString( m_playersPerRoom[ roomIndex ] ) + " / " + maxNumPlayerString + " players in room";

		OpenGLRenderer::RenderText( roomText, m_hudFont, HUD_FONT_CELL_HEIGHT, Vector2( 350.f, m_size.y - ( 110.f * ( roomIndex + 1 ) - 20.f ) ), Color::White );
	}
}


//-----------------------------------------------------------------------------------------------
void World::RenderWorld()
{
	RenderFloor();
	RenderWalls();
}


//-----------------------------------------------------------------------------------------------
void World::RenderFloor()
{
	OpenGLRenderer::EnableTexture2D();

	OpenGLRenderer::BindTexture2D( m_floorTexture->m_openglTextureID );
	OpenGLRenderer::SetColor3f( 1.f, 1.f, 1.f );

	OpenGLRenderer::BeginRender( QUADS );
	{
		OpenGLRenderer::SetTexCoords2f( 0.f, ARENA_FLOOR_SIZE_Y * 0.05f );
		OpenGLRenderer::SetVertex3f( 0.f, 0.f, 0.f );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, ARENA_FLOOR_SIZE_Y * 0.05f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, 0.f, 0.f );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, 0.f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, ARENA_FLOOR_SIZE_Y, 0.f );

		OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
		OpenGLRenderer::SetVertex3f( 0.f, ARENA_FLOOR_SIZE_Y, 0.f );
	}
	OpenGLRenderer::EndRender();

	OpenGLRenderer::BindTexture2D( 0 );
	OpenGLRenderer::DisableTexture2D();
}


//-----------------------------------------------------------------------------------------------
void World::RenderWalls()
{
	OpenGLRenderer::EnableTexture2D();

	OpenGLRenderer::BindTexture2D( m_wallTexture->m_openglTextureID );
	OpenGLRenderer::SetColor3f( 1.f, 1.f, 1.f );

	OpenGLRenderer::BeginRender( QUADS );
	{
		// north wall
		OpenGLRenderer::SetTexCoords2f( 0.f, ARENA_FLOOR_SIZE_Y * 0.01f );
		OpenGLRenderer::SetVertex3f( 0.f, ARENA_FLOOR_SIZE_Y, 0.f );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, ARENA_FLOOR_SIZE_Y * 0.01f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, ARENA_FLOOR_SIZE_Y, 0.f );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, 0.f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, ARENA_FLOOR_SIZE_Y, ARENA_WALL_HEIGHT );

		OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
		OpenGLRenderer::SetVertex3f( 0.f, ARENA_FLOOR_SIZE_Y, ARENA_WALL_HEIGHT );

		// south wall
		OpenGLRenderer::SetTexCoords2f( 0.f, ARENA_FLOOR_SIZE_Y * 0.01f );
		OpenGLRenderer::SetVertex3f( 0.f, 0.f, 0.f );

		OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
		OpenGLRenderer::SetVertex3f( 0.f, 0.f, ARENA_WALL_HEIGHT );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, 0.f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, 0.f, ARENA_WALL_HEIGHT );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, ARENA_FLOOR_SIZE_Y * 0.01f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, 0.f, 0.f );

		// east wall
		OpenGLRenderer::SetTexCoords2f( 0.f, ARENA_FLOOR_SIZE_Y * 0.01f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, 0.f, 0.f );

		OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, 0.f, ARENA_WALL_HEIGHT );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, 0.f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, ARENA_FLOOR_SIZE_Y, ARENA_WALL_HEIGHT );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, ARENA_FLOOR_SIZE_Y * 0.01f );
		OpenGLRenderer::SetVertex3f( ARENA_FLOOR_SIZE_X, ARENA_FLOOR_SIZE_Y, 0.f );

		// west wall
		OpenGLRenderer::SetTexCoords2f( 0.f, ARENA_FLOOR_SIZE_Y * 0.01f );
		OpenGLRenderer::SetVertex3f( 0.f, 0.f, 0.f );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, ARENA_FLOOR_SIZE_Y * 0.01f );
		OpenGLRenderer::SetVertex3f( 0.f, ARENA_FLOOR_SIZE_Y, 0.f );

		OpenGLRenderer::SetTexCoords2f( ARENA_FLOOR_SIZE_X * 0.05f, 0.f );
		OpenGLRenderer::SetVertex3f( 0.f, ARENA_FLOOR_SIZE_Y, ARENA_WALL_HEIGHT );

		OpenGLRenderer::SetTexCoords2f( 0.f, 0.f );
		OpenGLRenderer::SetVertex3f( 0.f, 0.f, ARENA_WALL_HEIGHT );
	}
	OpenGLRenderer::EndRender();

	OpenGLRenderer::BindTexture2D( 0 );
	OpenGLRenderer::DisableTexture2D();
}


//-----------------------------------------------------------------------------------------------
void World::RenderTanks()
{
	for( unsigned int tankIndex = 0; tankIndex < m_tanks.size(); ++tankIndex )
	{
		Tank* tank = m_tanks[ tankIndex ];
		tank->Render();
	}
}