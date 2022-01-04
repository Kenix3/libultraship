#pragma once

#include "OTRResource.h"

namespace OtrLib 
{
	class Vtx 
	{
	public:
		int16_t x, y, z;
		uint16_t flag;
		int16_t s, t;
		uint8_t r, g, b, a;
	};

	class OTRVtxV0 : public OTRResourceFile
	{
	public:
		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};

	class OTRVtx : public OTRResource
	{
	public:
		std::vector<Vtx> vtxList;
	};
}
