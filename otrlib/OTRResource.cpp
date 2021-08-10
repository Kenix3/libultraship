#include "OTRResource.h"
#include "Utils/BinaryReader.h"

namespace OtrLib
{
    void OTRResourceFile::ParseFileBinary(BinaryReader* reader, OTRResource* res)
    {
        id = reader->ReadUInt64();
        res->id = id;
    }
    void OTRResourceFile::ParseFileXML(tinyxml2::XMLElement* reader, OTRResource* res)
    {
        id = reader->Int64Attribute("id", -1);
    }

    void OTRResourceFile::WriteFileBinary(BinaryWriter* writer, OTRResource* res)
    {

    }

    void OTRResourceFile::WriteFileXML(tinyxml2::XMLElement* writer, OTRResource* res)
    {

    }
}