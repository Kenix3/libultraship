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
        case ResourceType::Material:
            result = MaterialFactory::ReadMaterial(reader);
            break;
        case ResourceType::Room:
            result = SceneFactory::ReadScene(reader);
            break;
        case ResourceType::CollisionHeader:
            result = CollisionHeaderFactory::ReadCollisionHeader(reader);
            break;
        case ResourceType::DisplayList:
            result = DisplayListFactory::ReadDisplayList(reader);
            break;
        case ResourceType::PlayerAnimation:
            result = PlayerAnimationFactory::ReadPlayerAnimation(reader);
            break;
        case ResourceType::Skeleton:
            result = SkeletonFactory::ReadSkeleton(reader);
            break;
        case ResourceType::SkeletonLimb:
            result = SkeletonLimbFactory::ReadSkeletonLimb(reader);
            break;
        case ResourceType::Vertex:
            result = VertexFactory::ReadVtx(reader);
            break;
        case ResourceType::Animation:
            result = AnimationFactory::ReadAnimation(reader);
            break;
        case ResourceType::Cutscene:
            result = CutsceneFactory::ReadCutscene(reader);
            break;
        case ResourceType::Array:
            result = ArrayFactory::ReadArray(reader);
            break;
        default:
            // RESOURCE TYPE NOT SUPPORTED
            break;
        }

        return result;
    }
}