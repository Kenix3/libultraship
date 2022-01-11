#include "ResourceLoader.h"
#include "MaterialFactory.h"
#include "SceneFactory.h"
#include "CollisionHeaderFactory.h"
#include "DisplayListFactory.h"
#include "PlayerAnimationFactory.h"
#include "SkeletonFactory.h"
#include "SkeletonLimbFactory.h"
#include "AnimationFactory.h"
#include "VtxFactory.h"
#include "CutsceneFactory.h"
#include "ArrayFactory.h"

namespace Ship
{
    Resource* ResourceLoader::LoadResource(BinaryReader* reader)
    {
        Endianess endianess = (Endianess)reader->ReadByte();
        
        // TODO: Setup the binaryreader to use the resource's endianess
        
        ResourceType resourceType = (ResourceType)reader->ReadUInt32();
        Resource* result = nullptr;

        switch (resourceType)
        {
        case ResourceType::OTRMaterial:
            result = MaterialFactory::ReadMaterial(reader);
            break;
        case ResourceType::OTRRoom:
            result = SceneFactory::ReadScene(reader);
            break;
        case ResourceType::OTRCollisionHeader:
            result = CollisionHeaderFactory::ReadCollisionHeader(reader);
            break;
        case ResourceType::OTRDisplayList:
            result = DisplayListFactory::ReadDisplayList(reader);
            break;
        case ResourceType::OTRPlayerAnimation:
            result = PlayerAnimationFactory::ReadPlayerAnimation(reader);
            break;
        case ResourceType::OTRSkeleton:
            result = SkeletonFactory::ReadSkeleton(reader);
            break;
        case ResourceType::OTRSkeletonLimb:
            result = SkeletonLimbFactory::ReadSkeletonLimb(reader);
            break;
        case ResourceType::OTRVtx:
            result = VertexFactory::ReadVtx(reader);
            break;
        case ResourceType::OTRAnimation:
            result = AnimationFactory::ReadAnimation(reader);
            break;
        case ResourceType::OTRCutscene:
            result = CutsceneFactory::ReadCutscene(reader);
            break;
        case ResourceType::OTRArray:
            result = ArrayFactory::ReadArray(reader);
            break;
        default:
            // RESOURCE TYPE NOT SUPPORTED
            break;
        }

        return result;
    }
}