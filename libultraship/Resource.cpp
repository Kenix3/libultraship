#include "Resource.h"
#include "Utils/BinaryReader.h"
#include "lib/tinyxml2/tinyxml2.h"

namespace Ship
{
    void ResourceFile::ParseFileBinary(BinaryReader* reader, Resource* res)
    {
        id = reader->ReadUInt64();
        res->id = id;
    }
    void ResourceFile::ParseFileXML(tinyxml2::XMLElement* reader, Resource* res)
    {
        id = reader->Int64Attribute("id", -1);
    }

    void ResourceFile::WriteFileBinary(BinaryWriter* writer, Resource* res)
    {
        
    }

    void ResourceFile::WriteFileXML(tinyxml2::XMLElement* writer, Resource* res)
    {

    }

    Resource::~Resource()
    {
        free(cachedGameAsset); 
        cachedGameAsset = nullptr;

#if _DEBUG
        printf("Deconstructor called on file %s\n", File->path.c_str());
#endif
    }
}