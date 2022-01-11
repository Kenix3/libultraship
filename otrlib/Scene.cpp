#include "Scene.h"

namespace Ship
{
	Scene::~Scene()
	{
		int bp = 0;
	}

	void SceneV0::ParseFileBinary(BinaryReader* reader, Resource* res)
	{
		Scene* scene = (Scene*)res;

		ResourceFile::ParseFileBinary(reader, res);

		int cmdCnt = reader->ReadInt32();

		for (int i = 0; i < cmdCnt; i++)
			scene->commands.push_back(ParseSceneCommand(reader));
	}

	SceneCommand* SceneV0::ParseSceneCommand(BinaryReader* reader)
	{
		SceneCommandID cmdID = (SceneCommandID)reader->ReadInt32();

		reader->Seek(-4, SeekOffsetType::Current);

		printf("CMD: 0x%02X\n", cmdID);

		switch (cmdID)
		{
		case SceneCommandID::SetStartPositionList: return new SetStartPositionList(reader);
		case SceneCommandID::SetActorList: return new SetActorList(reader);
		case SceneCommandID::SetTransitionActorList: return new SetTransitionActorList(reader);
		case SceneCommandID::SetWind: return new SetWind(reader);
		case SceneCommandID::SetTimeSettings: return new SetTimeSettings(reader);
		case SceneCommandID::SetSkyboxModifier: return new SetSkyboxModifier(reader);
		case SceneCommandID::SetEchoSettings: return new SetEchoSettings(reader);
		case SceneCommandID::SetSoundSettings: return new SetSoundSettings(reader);
		case SceneCommandID::SetSkyboxSettings: return new SetSkyboxSettings(reader);
		case SceneCommandID::SetRoomBehavior: return new SetRoomBehavior(reader);
		case SceneCommandID::SetCsCamera: return new SetCsCamera(reader);
		case SceneCommandID::SetMesh: return new SetMesh(reader);
		case SceneCommandID::SetCameraSettings: return new SetCameraSettings(reader);
		case SceneCommandID::SetLightingSettings: return new SetLightingSettings(reader);
		case SceneCommandID::SetLightList: return new SetLightList(reader);
		case SceneCommandID::SetRoomList: return new SetRoomList(reader);
		case SceneCommandID::SetCollisionHeader: return new SetCollisionHeader(reader);
		case SceneCommandID::SetEntranceList: return new SetEntranceList(reader);
		case SceneCommandID::SetSpecialObjects: return new SetSpecialObjects(reader);
		case SceneCommandID::SetObjectList: return new SetObjectList(reader);
		case SceneCommandID::SetAlternateHeaders: return new SetAlternateHeaders(reader);
		case SceneCommandID::SetExitList: return new ExitList(reader);
		case SceneCommandID::SetCutscenes: return new SetCutscenes(reader);
		case SceneCommandID::EndMarker: return new EndMarker(reader);
		default:
			printf("UNIMPLEMENTED COMMAND: %i\n", (int)cmdID);
			reader->ReadInt32();
			break;
		}

		return nullptr;
	}

	SceneCommand::SceneCommand(BinaryReader* reader)
	{
		cmdID = (SceneCommandID)reader->ReadInt32();
	}

	SetWind::SetWind(BinaryReader* reader) : SceneCommand(reader)
	{
		windWest = reader->ReadByte();
		windVertical = reader->ReadByte();
		windSouth = reader->ReadByte();
		clothFlappingStrength = reader->ReadByte();
	}

	ExitList::ExitList(BinaryReader* reader) : SceneCommand(reader)
	{
		int numExits = reader->ReadInt32();

		for (int i = 0; i < numExits; i++)
			exits.push_back(reader->ReadUInt16());
	}

	SetTimeSettings::SetTimeSettings(BinaryReader* reader) : SceneCommand(reader)
	{
		hour = reader->ReadByte();
		min = reader->ReadByte();
		unk = reader->ReadByte();
	}

	SetSkyboxModifier::SetSkyboxModifier(BinaryReader* reader) : SceneCommand(reader)
	{
		disableSky = reader->ReadByte();
		disableSunMoon = reader->ReadByte();
	}

	SetEchoSettings::SetEchoSettings(BinaryReader* reader) : SceneCommand(reader)
	{
		echo = reader->ReadByte();
	}

	SetSoundSettings::SetSoundSettings(BinaryReader* reader) : SceneCommand(reader)
	{
		reverb = reader->ReadByte();
		nightTimeSFX = reader->ReadByte();
		musicSequence = reader->ReadByte();
	}

	SetSkyboxSettings::SetSkyboxSettings(BinaryReader* reader) : SceneCommand(reader)
	{
		unk1 = reader->ReadByte();
		skyboxNumber = reader->ReadByte();
		cloudsType = reader->ReadByte();
		isIndoors = reader->ReadByte();
	}

	SetRoomBehavior::SetRoomBehavior(BinaryReader* reader) : SceneCommand(reader)
	{
		gameplayFlags = reader->ReadByte();
		gameplayFlags2 = reader->ReadInt32();
	}

	SetCsCamera::SetCsCamera(BinaryReader* reader) : SceneCommand(reader)
	{
		reader->ReadByte(); // camSize
		reader->ReadInt32(); // segOffset

		// LUSTODO: FINISH!
	}

	MeshData::MeshData()
	{
		x = 0;
		y = 0;
		z = 0;
		unk_06 = 0;
		opa = "";
		xlu = "";
	}

	SetMesh::SetMesh(BinaryReader* reader) : SceneCommand(reader)
	{
		data = reader->ReadByte();
		meshHeaderType = reader->ReadByte();

		int numPoly = 1;

		if (meshHeaderType != 1)
			numPoly = reader->ReadByte();

		for (int i = 0; i < numPoly; i++)
		{
			MeshData mesh;

			// LUSTODO: FINISH THIS!
			if (meshHeaderType == 0)
			{
				int polyType = reader->ReadByte();
				mesh.x = 0;
				mesh.y = 0;
				mesh.z = 0;
				mesh.unk_06 = 0;
			}
			else if (meshHeaderType == 2)
			{
				int polyType = reader->ReadByte();
				mesh.x = reader->ReadInt16();
				mesh.y = reader->ReadInt16();
				mesh.z = reader->ReadInt16();
				mesh.unk_06 = reader->ReadInt16();
			}
			else
			{
				mesh.imgFmt = reader->ReadUByte();
				mesh.imgOpa = reader->ReadString();
				mesh.imgXlu = reader->ReadString();


				int imgCnt = reader->ReadUInt32();

				for (int i = 0; i < imgCnt; i++)
				{
					BGImage img;

					img.unk_00 = reader->ReadUInt16();
					img.id = reader->ReadUByte();
					img.sourceBackground = reader->ReadString();
					img.unk_0C = reader->ReadUInt32();
					img.tlut = reader->ReadUInt32();
					img.width = reader->ReadUInt16();
					img.height = reader->ReadUInt16();
					img.fmt = reader->ReadUByte();
					img.siz = reader->ReadUByte();
					img.mode0 = reader->ReadUInt16();
					img.tlutCount = reader->ReadUInt16();

					mesh.images.push_back(img);
				}

				int polyType = reader->ReadByte();

				int bp = 0;
			}


			//if (meshHeaderType == 0 || meshHeaderType == 2)
			{
				mesh.opa = reader->ReadString();
				mesh.xlu = reader->ReadString();
			}
			//else
			//{
				//mesh.opa = "";
				//mesh.xlu = "";
			//}

			meshes.push_back(mesh);
		}
	}

	SetCameraSettings::SetCameraSettings(BinaryReader* reader) : SceneCommand(reader)
	{
		cameraMovement = reader->ReadByte();
		mapHighlights = reader->ReadInt32();
	}

	SetLightingSettings::SetLightingSettings(BinaryReader* reader) : SceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			LightingSettings entry = LightingSettings();

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

			entry.fogNear = reader->ReadInt16();
			entry.fogFar = reader->ReadUInt16();

			settings.push_back(entry);
		}
	}

	SetRoomList::SetRoomList(BinaryReader* reader) : SceneCommand(reader)
	{
		uint32_t numRooms = reader->ReadInt32();

		for (int i = 0; i < numRooms; i++)
		{
			std::string room = reader->ReadString();
			rooms.push_back(room);
		}
	}

	SetCollisionHeader::SetCollisionHeader(BinaryReader* reader) : SceneCommand(reader)
	{
		filePath = reader->ReadString();
	}

	SetEntranceList::SetEntranceList(BinaryReader* reader) : SceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			EntranceEntry entry = EntranceEntry();
			entry.startPositionIndex = reader->ReadByte();
			entry.roomToLoad = reader->ReadByte();
			
			entrances.push_back(entry);
		}
	}

	SetSpecialObjects::SetSpecialObjects(BinaryReader* reader) : SceneCommand(reader)
	{
		elfMessage = reader->ReadByte();
		globalObject = reader->ReadInt16();
	}

	SetObjectList::SetObjectList(BinaryReader* reader) : SceneCommand(reader)
	{
		int numEntries = reader->ReadInt32();

		for (int i = 0; i < numEntries; i++)
			objects.push_back(reader->ReadUInt16());
	}

	SetStartPositionList::SetStartPositionList(BinaryReader* reader) : SceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			ActorSpawnEntry entry = ActorSpawnEntry();

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

	SetActorList::SetActorList(BinaryReader* reader) : SceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			ActorSpawnEntry entry = ActorSpawnEntry();

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

	SetTransitionActorList::SetTransitionActorList(BinaryReader* reader) : SceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			TransitionActorEntry entry = TransitionActorEntry();

			entry.frontObjectRoom = reader->ReadUByte();
			entry.frontTransitionReaction = reader->ReadUByte();
			entry.backObjectRoom = reader->ReadUByte();
			entry.backTransitionReaction = reader->ReadUByte();
			entry.actorNum = reader->ReadInt16();
			entry.posX = reader->ReadInt16();
			entry.posY = reader->ReadInt16();
			entry.posZ = reader->ReadInt16();
			entry.rotY = reader->ReadInt16();
			entry.initVar = reader->ReadUInt16();

			entries.push_back(entry);
		}
	}

	EndMarker::EndMarker(BinaryReader* reader) : SceneCommand(reader)
	{
	}

	LightInfo::LightInfo()
	{
	}

	SetLightList::SetLightList(BinaryReader* reader) : SceneCommand(reader)
	{
		int cnt = reader->ReadInt32();

		for (int i = 0; i < cnt; i++)
		{
			LightInfo light = LightInfo();

			light.type = reader->ReadUByte();
			
			light.x = reader->ReadInt16();
			light.y = reader->ReadInt16();			
			light.z = reader->ReadInt16();

			light.r = reader->ReadUByte();
			light.g = reader->ReadUByte();
			light.b = reader->ReadUByte();
			
			light.drawGlow = reader->ReadUByte();
			light.radius = reader->ReadInt16();
			
			lights.push_back(light);
		}
	}

	SetAlternateHeaders::SetAlternateHeaders(BinaryReader* reader) : SceneCommand(reader)
	{
		int numHeaders = reader->ReadInt32();

		for (int i = 0; i < numHeaders; i++)
			headers.push_back(reader->ReadString());
	}

	SetCutscenes::SetCutscenes(BinaryReader* reader) : SceneCommand(reader)
	{
		cutscenePath = reader->ReadString();
	}
}