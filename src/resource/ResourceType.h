#pragma once

#include <unordered_map>
#include <string>

namespace LUS {

enum class ResourceType {
    // Not set
    None = 0x00000000,

    // Common
    Archive = 0x4F415243,     // OARC (UNUSED)
    DisplayList = 0x4F444C54, // ODLT
    Vertex = 0x4F565458,      // OVTX
    Matrix = 0x4F4D5458,      // OMTX
    Array = 0x4F415252,       // OARR
    Blob = 0x4F424C42,        // OBLB
    Texture = 0x4F544558,     // OTEX

    // LUS of Harkinian
    SOH_Animation = 0x4F414E4D,       // OANM
    SOH_PlayerAnimation = 0x4F50414D, // OPAM
    SOH_Room = 0x4F524F4D,            // OROM
    SOH_CollisionHeader = 0x4F434F4C, // OCOL
    SOH_Skeleton = 0x4F534B4C,        // OSKL
    SOH_SkeletonLimb = 0x4F534C42,    // OSLB
    SOH_Path = 0x4F505448,            // OPTH
    SOH_Cutscene = 0x4F435654,        // OCUT
    SOH_Text = 0x4F545854,            // OTXT
    SOH_Audio = 0x4F415544,           // OAUD
    SOH_AudioSample = 0x4F534D50,     // OSMP
    SOH_AudioSoundFont = 0x4F534654,  // OSFT
    SOH_AudioSequence = 0x4F534551,   // OSEQ
    SOH_Background = 0x4F424749,      // OBGI
    SOH_SceneCommand = 0x4F52434D,    // ORCM
};

inline std::unordered_map<std::string, uint32_t> mResourceTypes {
    { "None", static_cast<uint32_t>(ResourceType::None) },
    { "Archive", static_cast<uint32_t>(ResourceType::Archive) },
    { "DisplayList", static_cast<uint32_t>(ResourceType::DisplayList) },
    { "Vertex", static_cast<uint32_t>(ResourceType::Vertex) },
    { "Matrix", static_cast<uint32_t>(ResourceType::Matrix) },
    { "Array", static_cast<uint32_t>(ResourceType::Array) },
    { "Blob", static_cast<uint32_t>(ResourceType::Blob) },
    { "Texture", static_cast<uint32_t>(ResourceType::Texture) },
    { "SOH_Animation", static_cast<uint32_t>(ResourceType::SOH_Animation) },
    { "SOH_PlayerAnimation", static_cast<uint32_t>(ResourceType::SOH_PlayerAnimation) },
    { "SOH_Room", static_cast<uint32_t>(ResourceType::SOH_Room) },
    { "SOH_CollisionHeader", static_cast<uint32_t>(ResourceType::SOH_CollisionHeader) },
    { "SOH_Skeleton", static_cast<uint32_t>(ResourceType::SOH_Skeleton) },
    { "SOH_SkeletonLimb", static_cast<uint32_t>(ResourceType::SOH_SkeletonLimb) },
    { "SOH_Path", static_cast<uint32_t>(ResourceType::SOH_Path) },
    { "SOH_Cutscene", static_cast<uint32_t>(ResourceType::SOH_Cutscene) },
    { "SOH_Text", static_cast<uint32_t>(ResourceType::SOH_Text) },
    { "SOH_Audio", static_cast<uint32_t>(ResourceType::SOH_Audio) },
    { "SOH_AudioSample", static_cast<uint32_t>(ResourceType::SOH_AudioSample) },
    { "SOH_AudioSoundFont", static_cast<uint32_t>(ResourceType::SOH_AudioSoundFont) },
    { "SOH_AudioSequence", static_cast<uint32_t>(ResourceType::SOH_AudioSequence) },
    { "SOH_Background", static_cast<uint32_t>(ResourceType::SOH_Background) },
    { "SOH_SceneCommand", static_cast<uint32_t>(ResourceType::SOH_SceneCommand) },
};
} // namespace LUS
