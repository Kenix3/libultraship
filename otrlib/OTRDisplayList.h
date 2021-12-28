#pragma once

#include <vector>
#include <string>
#include "OTRResource.h"
#include "Vec2f.h"
#include "Vec3f.h"
#include "Color3b.h"

namespace OtrLib
{
	class OTRDisplayListV0 : public OTRResourceFile
	{
	public:
		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};

    class OTRDisplayList : public OTRResource
    {
    public:
		std::vector<uint64_t> instructions;
    };
}