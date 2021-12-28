#include "OTRScene.h"

namespace OtrLib
{
	OTRScene::~OTRScene()
	{
		int bp = 0;
	}

	void OTRSceneV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
	{
		OTRScene* scene = (OTRScene*)res;

		OTRResourceFile::ParseFileBinary(reader, res);

		int cmdCnt = reader->ReadInt32();

		for (int i = 0; i < cmdCnt; i++)
			scene->commands.push_back(ParseSceneCommand(reader));
	}

	OTRSceneCommand* OTRSceneV0::ParseSceneCommand(BinaryReader* reader)
	{
		OTRSceneCommandID cmdID = (OTRSceneCommandID)reader->ReadInt32();

		reader->Seek(-4, SeekOffsetType::Current);

		printf("CMD: 0x%02X\n", cmdID);

		switch (cmdID)
		{
		case OTRSceneCommandID::SetStartPositionList: return new OTRSetStartPositionList(reader);
		case OTRSceneCommandID::SetActorList: return new OTRSetActorList(reader);
		case OTRSceneCommandID::SetWind: return new OTRSetWind(reader);
		case OTRSceneCommandID::SetTimeSettings: return new OTRSetTimeSettings(reader);
		case OTRSceneCommandID::SetSkyboxModifier: return new OTRSetSkyboxModifier(reader);
		case OTRSceneCommandID::SetEchoSettings: return new OTRSetEchoSettings(reader);
		case OTRSceneCommandID::SetSoundSettings: return new OTRSetSoundSettings(reader);
		case OTRSceneCommandID::SetSkyboxSettings: return new OTRSetSkyboxSettings(reader);
		case OTRSceneCommandID::SetRoomBehavior: return new OTRSetRoomBehavior(reader);
		case OTRSceneCommandID::SetCsCamera: return new OTRSetCsCamera(reader);
		case OTRSceneCommandID::SetMesh: return new OTRSetMesh(reader);
		case OTRSceneCommandID::SetCameraSettings: return new OTRSetCameraSettings(reader);
		case OTRSceneCommandID::SetLightingSettings: return new OTRSetLightingSettings(reader);
		case OTRSceneCommandID::SetRoomList: return new OTRSetRoomList(reader);
		case OTRSceneCommandID::SetCollisionHeader: return new OTRSetCollisionHeader(reader);
		case OTRSceneCommandID::SetEntranceList: return new OTRSetEntranceList(reader);
		case OTRSceneCommandID::SetSpecialObjects: return new OTRSetSpecialObjects(reader);
		case OTRSceneCommandID::SetObjectList: return new OTRSetObjectList(reader);
		case OTRSceneCommandID::EndMarker: return new OTREndMarker(reader);
		default:
			printf("UNIMPLEMENTED COMMAND: %i\n", (int)cmdID);
			reader->ReadInt32();
			break;
		}

		return nullptr;
	}

	OTRSceneCommand::OTRSceneCommand(BinaryReader* reader)
	{
		cmdID = (OTRSceneCommandID)reader->ReadInt32();
	}

	OTRSetWind::OTRSetWind(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		windWest = reader->ReadByte();
		windVertical = reader->ReadByte();
		windSouth = reader->ReadByte();
		clothFlappingStrength = reader->ReadByte();
	}

	OTRSetTimeSettings::OTRSetTimeSettings(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		hour = reader->ReadByte();
		min = reader->ReadByte();
		unk = reader->ReadByte();
	}

	OTRSetSkyboxModifier::OTRSetSkyboxModifier(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		disableSky = reader->ReadByte();
		disableSunMoon = reader->ReadByte();
	}

	OTRSetEchoSettings::OTRSetEchoSettings(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		echo = reader->ReadByte();
	}

	OTRSetSoundSettings::OTRSetSoundSettings(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		reverb = reader->ReadByte();
		nightTimeSFX = reader->ReadByte();
		musicSequence = reader->ReadByte();
	}

	OTRSetSkyboxSettings::OTRSetSkyboxSettings(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		unk1 = reader->ReadByte();
		skyboxNumber = reader->ReadByte();
		cloudsType = reader->ReadByte();
		isIndoors = reader->ReadByte();
	}

	OTRSetRoomBehavior::OTRSetRoomBehavior(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		gameplayFlags = reader->ReadByte();
		gameplayFlags2 = reader->ReadInt32();
	}

	OTRSetCsCamera::OTRSetCsCamera(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		reader->ReadByte(); // camSize
		reader->ReadInt32(); // segOffset

		// TODO: FINISH!
	}

	OTRMeshData::OTRMeshData()
	{
		x = 0;
		y = 0;
		z = 0;
		unk_06 = 0;
		opa = "";
		xlu = "";
	}

	OTRSetMesh::OTRSetMesh(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		data = reader->ReadByte();
		meshHeaderType = reader->ReadByte();

		int numPoly = reader->ReadByte();

		for (int i = 0; i < numPoly; i++)
		{
			OTRMeshData mesh;

			int polyType = reader->ReadByte();

			// OTRTODO: FINISH THIS!
			if (meshHeaderType == 0)
			{
				mesh.x = 0;
				mesh.y = 0;
				mesh.z = 0;
				mesh.unk_06 = 0;
			}
			else if (meshHeaderType == 2)
			{
				mesh.x = reader->ReadInt16();
				mesh.y = reader->ReadInt16();
				mesh.z = reader->ReadInt16();
				mesh.unk_06 = reader->ReadInt16();
			}
			else
			{
				int bp = 0;
			}


			mesh.opa = reader->ReadString();
			mesh.xlu = reader->ReadString();

			meshes.push_back(mesh);
		}
	}

	OTRSetCameraSettings::OTRSetCameraSettings(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		cameraMovement = reader->ReadByte();
		mapHighlights = reader->ReadInt32();
	}

	OTRSetLightingSettings::OTRSetLightingSettings(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			OTRLightingSettings entry = OTRLightingSettings();

			entry.ambientClrR = reader->ReadByte();
			entry.ambientClrG = reader->ReadByte();
			entry.ambientClrB = reader->ReadByte();

			entry.diffuseClrA_R = reader->ReadByte();
			entry.diffuseClrA_G = reader->ReadByte();
			entry.diffuseClrA_B = reader->ReadByte();

			entry.diffuseDirA_X = reader->ReadByte();
			entry.diffuseDirA_Y = reader->ReadByte();
			entry.diffuseDirA_Z = reader->ReadByte();

			entry.diffuseClrB_R = reader->ReadByte();
			entry.diffuseClrB_G = reader->ReadByte();
			entry.diffuseClrB_B = reader->ReadByte();

			entry.diffuseDirB_X = reader->ReadByte();
			entry.diffuseDirB_Y = reader->ReadByte();
			entry.diffuseDirB_Z = reader->ReadByte();

			entry.fogClrR = reader->ReadByte();
			entry.fogClrG = reader->ReadByte();
			entry.fogClrB = reader->ReadByte();

			entry.unk = reader->ReadInt16();
			entry.drawDistance = reader->ReadInt16();

			settings.push_back(entry);
		}
	}

	OTRSetRoomList::OTRSetRoomList(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		uint32_t numRooms = reader->ReadInt32();

		for (int i = 0; i < numRooms; i++)
		{
			std::string room = reader->ReadString();
			rooms.push_back(room);
		}
	}

	OTRSetCollisionHeader::OTRSetCollisionHeader(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		// TODO: FINISH
		filePath = reader->ReadString();
	}

	OTRSetEntranceList::OTRSetEntranceList(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			OTREntranceEntry entry = OTREntranceEntry();
			entry.startPositionIndex = reader->ReadByte();
			entry.roomToLoad = reader->ReadByte();
			
			entrances.push_back(entry);
		}
	}

	OTRSetSpecialObjects::OTRSetSpecialObjects(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		elfMessage = reader->ReadByte();
		globalObject = reader->ReadInt16();
	}

	OTRSetObjectList::OTRSetObjectList(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		int numEntries = reader->ReadInt32();

		for (int i = 0; i < numEntries; i++)
			objects.push_back(reader->ReadUInt16());
	}

	OTRSetStartPositionList::OTRSetStartPositionList(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			OTRActorSpawnEntry entry = OTRActorSpawnEntry();

			entry.actorNum = reader->ReadUInt16();
			entry.posX = reader->ReadInt16();
			entry.posY = reader->ReadInt16();
			entry.posZ = reader->ReadInt16();
			entry.rotX = reader->ReadInt16();
			entry.rotY = reader->ReadInt16();
			entry.rotZ = reader->ReadInt16();
			entry.initVar = reader->ReadUInt16();

			entries.push_back(entry);
		}
	}

	OTRSetActorList::OTRSetActorList(BinaryReader* reader) : OTRSceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			OTRActorSpawnEntry entry = OTRActorSpawnEntry();

			entry.actorNum = reader->ReadUInt16();
			entry.posX = reader->ReadInt16();
			entry.posY = reader->ReadInt16();
			entry.posZ = reader->ReadInt16();
			entry.rotX = reader->ReadInt16();
			entry.rotY = reader->ReadInt16();
			entry.rotZ = reader->ReadInt16();
			entry.initVar = reader->ReadUInt16();

			entries.push_back(entry);
		}
	}

	OTREndMarker::OTREndMarker(BinaryReader* reader) : OTRSceneCommand(reader)
	{
	}
}