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
#include "PathFactory.h"
#include "TextFactory.h"
#include "TextureFactory.h"
#include "BlobFactory.h"
#include <Utils/MemoryStream.h>

namespace Ship
{
    Resource* ResourceLoader::LoadResource(std::shared_ptr<File> FileToLoad)
    {
        auto memStream = std::make_shared<MemoryStream>(FileToLoad->buffer.get(), FileToLoad->dwBufferSize);
        BinaryReader reader = BinaryReader(memStream);

        Endianess endianess = (Endianess)reader.ReadByte();
        
        // TODO: Setup the binaryreader to use the resource's endianess
        
        ResourceType resourceType = (ResourceType)reader.ReadUInt32();
        Resource* result = nullptr;

        switch (resourceType)
        {
        case ResourceType::Material:
            result = MaterialFactory::ReadMaterial(&reader);
            break;
        case ResourceType::Texture:
            result = TextureFactory::ReadTexture(&reader);
            break;
        case ResourceType::Room:
            result = SceneFactory::ReadScene(&reader);
            break;
        case ResourceType::CollisionHeader:
            result = CollisionHeaderFactory::ReadCollisionHeader(&reader);
            break;
        case ResourceType::DisplayList:
            result = DisplayListFactory::ReadDisplayList(&reader);
            break;
        case ResourceType::PlayerAnimation:
            result = PlayerAnimationFactory::ReadPlayerAnimation(&reader);
            break;
        case ResourceType::Skeleton:
            result = SkeletonFactory::ReadSkeleton(&reader);
            break;
        case ResourceType::SkeletonLimb:
            result = SkeletonLimbFactory::ReadSkeletonLimb(&reader);
            break;
        case ResourceType::Vertex:
            result = VertexFactory::ReadVtx(&reader);
            break;
        case ResourceType::Animation:
            result = AnimationFactory::ReadAnimation(&reader);
            break;
        case ResourceType::Cutscene:
            result = CutsceneFactory::ReadCutscene(&reader);
            break;
        case ResourceType::Array:
            result = ArrayFactory::ReadArray(&reader);
            break;
        case ResourceType::Path:
            result = PathFactory::ReadPath(&reader);
            break;
        case ResourceType::Text:
            result = TextFactory::ReadText(&reader);
            break;
        case ResourceType::Blob:
            result = BlobFactory::ReadBlob(&reader);
            break;
        default:
            // RESOURCE TYPE NOT SUPPORTED
            break;
        }

        if (result != nullptr) {
            result->File = FileToLoad;
            result->resType = resourceType;
        }

        return result;
    }
}