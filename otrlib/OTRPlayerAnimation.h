#pragma once

#include <vector>
#include <string>
#include "OTRResource.h"
#include "Vec2f.h"
#include "Vec3f.h"
#include "Color3b.h"

namespace OtrLib
{
	class OTRPlayerAnimationV0 : public OTRResourceFile
	{
	public:
		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};

    class OTRPlayerAnimation : public OTRResource
    {
    public:
		std::vector<uint16_t> limbRotData;
    };
}