#pragma once

#include "OTRResource.h"

namespace OtrLib
{
	class OTRCutsceneV0 : public OTRResourceFile
	{
	public:
		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};

	class CutsceneCommand
	{
	public:
		uint32_t commandID;
		uint32_t commandIndex;
	};

	class OTRCutscene : public OTRResource
	{
	public:
		//int32_t endFrame;
		std::vector<CutsceneCommand*> commands;
	};
}