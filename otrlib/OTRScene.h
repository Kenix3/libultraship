#pragma once

#include "OTRResource.h"
#include <vector>
#include <string>

namespace OtrLib
{
	enum class OTRSceneCommandID : uint8_t
	{
		SetStartPositionList = 0x00,
		SetActorList = 0x01,
		SetCsCamera = 0x02,
		SetCollisionHeader = 0x03,
		SetRoomList = 0x04,
		SetWind = 0x05,
		SetEntranceList = 0x06,
		SetSpecialObjects = 0x07,
		SetRoomBehavior = 0x08,
		Unused09 = 0x09,
		SetMesh = 0x0A,
		SetObjectList = 0x0B,
		SetLightList = 0x0C,
		SetPathways = 0x0D,
		SetTransitionActorList = 0x0E,
		SetLightingSettings = 0x0F,
		SetTimeSettings = 0x10,
		SetSkyboxSettings = 0x11,
		SetSkyboxModifier = 0x12,
		SetExitList = 0x13,
		EndMarker = 0x14,
		SetSoundSettings = 0x15,
		SetEchoSettings = 0x16,
		SetCutscenes = 0x17,
		SetAlternateHeaders = 0x18,
		SetCameraSettings = 0x19,

		// MM Commands
		SetWorldMapVisited = 0x19,
		SetAnimatedMaterialList = 0x1A,
		SetActorCutsceneList = 0x1B,
		SetMinimapList = 0x1C,
		Unused1D = 0x1D,
		SetMinimapChests = 0x1E,

		Error = 0xFF
	};

	class OTRSceneCommand
	{
	public:
		OTRSceneCommandID cmdID;

		// Data for a given class goes here...
		OTRSceneCommand(BinaryReader* reader);
	};

	class OTRLightingSettings
	{
	public:
		uint8_t ambientClrR, ambientClrG, ambientClrB;
		uint8_t diffuseClrA_R, diffuseClrA_G, diffuseClrA_B;
		uint8_t diffuseDirA_X, diffuseDirA_Y, diffuseDirA_Z;
		uint8_t diffuseClrB_R, diffuseClrB_G, diffuseClrB_B;
		uint8_t diffuseDirB_X, diffuseDirB_Y, diffuseDirB_Z;
		uint8_t fogClrR, fogClrG, fogClrB;
		uint16_t unk;
		uint16_t drawDistance;
	};


	class OTRSetWind : public OTRSceneCommand
	{
	public:
		uint8_t windWest;
		uint8_t windVertical;
		uint8_t windSouth;
		uint8_t clothFlappingStrength;

		OTRSetWind(BinaryReader* reader);
	};

	class OTRSetTimeSettings : public OTRSceneCommand
	{
	public:
		uint8_t hour;
		uint8_t min;
		uint8_t unk;

		OTRSetTimeSettings(BinaryReader* reader);
	};

	class OTRSetSkyboxModifier : public OTRSceneCommand
	{
	public:
		uint8_t disableSky;
		uint8_t disableSunMoon;

		OTRSetSkyboxModifier(BinaryReader* reader);
	};

	class OTRSetEchoSettings : public OTRSceneCommand
	{
	public:
		uint8_t echo;

		OTRSetEchoSettings(BinaryReader* reader);
	};

	class OTRSetSoundSettings : public OTRSceneCommand
	{
	public:
		uint8_t reverb;
		uint8_t nightTimeSFX;
		uint8_t musicSequence;

		OTRSetSoundSettings(BinaryReader* reader);
	};

	class OTRSetSkyboxSettings : public OTRSceneCommand
	{
	public:
		uint8_t unk1;  // (MM Only)
		uint8_t skyboxNumber;
		uint8_t cloudsType;
		uint8_t isIndoors;

		OTRSetSkyboxSettings(BinaryReader* reader);
	};

	class OTRSetRoomBehavior : public OTRSceneCommand
	{
	public:
		uint8_t gameplayFlags;
		uint32_t gameplayFlags2;

		uint8_t currRoomUnk2;

		uint8_t showInvisActors;
		uint8_t currRoomUnk5;

		uint8_t msgCtxUnk;

		uint8_t enablePosLights;
		uint8_t kankyoContextUnkE2;

		OTRSetRoomBehavior(BinaryReader* reader);
	};

	class OTRSetCsCamera : public OTRSceneCommand
	{
	public:
		OTRSetCsCamera(BinaryReader* reader);
	};

	class OTRSetMesh : public OTRSceneCommand
	{
	public:
		uint8_t data;
		uint8_t meshHeaderType;
		//std::shared_ptr<PolygonTypeBase> polyType;

		OTRSetMesh(BinaryReader* reader);
	};

	class OTRSetCameraSettings : public OTRSceneCommand
	{
	public:
		uint8_t cameraMovement;
		uint32_t mapHighlights;

		OTRSetCameraSettings(BinaryReader* reader);
	};

	class OTRSetLightingSettings : public OTRSceneCommand
	{
	public:
		std::vector<OTRLightingSettings> settings;

		OTRSetLightingSettings(BinaryReader* reader);
	};

	class OTRSetRoomList : public OTRSceneCommand
	{
	public:
		//std::vector<OTRLightingSettings> settings;

		OTRSetRoomList(BinaryReader* reader);
	};

	class OTRSetCollisionHeader : public OTRSceneCommand
	{
	public:
		//std::vector<OTRLightingSettings> settings;

		OTRSetCollisionHeader(BinaryReader* reader);
	};

	class OTREntranceEntry
	{
	public:
		uint8_t startPositionIndex;
		uint8_t roomToLoad;
	};

	class OTRSetEntranceList : public OTRSceneCommand
	{
	public:
		std::vector<OTREntranceEntry> entrances;

		OTRSetEntranceList(BinaryReader* reader);
	};

	class OTRSetSpecialObjects : public OTRSceneCommand
	{
	public:
		uint8_t elfMessage;
		uint16_t globalObject;
		
		OTRSetSpecialObjects(BinaryReader* reader);
	};

	class OTRActorSpawnEntry
	{
	public:
		uint16_t actorNum;
		int16_t posX;
		int16_t posY;
		int16_t posZ;
		int16_t rotX;
		int16_t rotY;
		int16_t rotZ;
		uint16_t initVar;
	};


	class OTRSetStartPositionList : public OTRSceneCommand
	{
	public:
		std::vector<OTRActorSpawnEntry> entries;

		OTRSetStartPositionList(BinaryReader* reader);
	};

	class OTRSetActorList : public OTRSceneCommand
	{
	public:
		std::vector<OTRActorSpawnEntry> entries;

		OTRSetActorList(BinaryReader* reader);
	};

	class OTREndMarker : public OTRSceneCommand
	{
	public:
		OTREndMarker(BinaryReader* reader);
	};

	class OTRScene : public OTRResource
	{
	public:
		std::vector<OTRSceneCommand*> commands;
	};

	class OTRSceneV0 : public OTRResourceFile
	{
	public:
		OTRSceneCommand* ParseSceneCommand(BinaryReader* reader);
		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};
}