#pragma once
#include "../OTRVtx.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
	class OTRVtxFactory
	{
	public:
		static OTRVtx* ReadVtx(BinaryReader* reader);
	};
}