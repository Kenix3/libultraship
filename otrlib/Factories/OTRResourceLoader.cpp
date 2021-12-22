#include "OTRResourceLoader.h"
#include "OTRMaterialFactory.h"
#include "OTRSceneFactory.h"
#include "OTRCollisionHeaderFactory.h"
#include "OTRDisplayListFactory.h"

namespace OtrLib
{
    OTRResource* OTRResourceLoader::LoadResource(BinaryReader* reader)
    {
        Endianess endianess = (Endianess)reader->ReadByte();
        
        // TODO: Setup the binaryreader to use the resource's endianess
        
        ResourceType resourceType = (ResourceType)reader->ReadUInt32();
        OTRResource* result = nullptr;

        switch (resourceType)
        {
        case ResourceType::OTRMaterial:
            result = OTRMaterialFactory::ReadMaterial(reader);
            break;
        case ResourceType::OTRRoom:
            result = OTRSceneFactory::ReadScene(reader);
            break;
        case ResourceType::OTRCollisionHeader:
            result = OTRCollisionHeaderFactory::ReadCollisionHeader(reader);
            break;
        case ResourceType::OTRDisplayList:
            result = OTRDisplayListFactory::ReadDisplayList(reader);
            break;
        default:
            // RESOURCE TYPE NOT SUPPORTED
            break;
        }

        return result;
    }
}