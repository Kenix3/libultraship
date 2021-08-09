#include "OTRResourceLoader.h"
#include "OTRMaterialFactory.h"
#include "OTRArchiveFactory.h"

namespace OtrLib
{
    OTRResource* OTRResourceLoader::LoadResource(BinaryReader* reader)
    {
        Endianess endianess = (Endianess)reader->ReadInt32();
        
        // TODO: Setup the binaryreader to use the resource's endianess
        
        ResourceType resourceType = (ResourceType)reader->ReadUInt32();
        OTRResource* result = nullptr;

        switch (resourceType)
        {
        case ResourceType::OTRMaterial:
            result = OTRMaterialFactory::ReadMaterial(reader);
            break;
        case ResourceType::OTRArchive:
            result = OTRArchiveFactory::ReadArchive(reader);
            break;
        default:
            // RESOURCE TYPE NOT SUPPORTED
            break;
        }

        return result;
    }
}