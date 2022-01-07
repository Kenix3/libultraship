#include "OTRArray.h"

namespace OtrLib
{
	void OTRArrayV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
	{
		OTRArray* arr = (OTRArray*)res;

		OTRResourceFile::ParseFileBinary(reader, res);

		uint32_t arrayCnt = reader->ReadUInt32();

		for (int i = 0; i < arrayCnt; i++)
		{
			ScalarType scalType = (ScalarType)reader->ReadUInt32();
			ScalarData data;

			switch (scalType)
			{
			case ScalarType::ZSCALAR_S16:
				data.s16 = reader->ReadInt16();
				break;
			case ScalarType::ZSCALAR_U16:
				data.u16 = reader->ReadUInt16();
				break;
				// OTRTODO: IMPLEMENT OTHER TYPES!
			}

			arr->scalars.push_back(data);
		}
	}
}
