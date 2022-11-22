#include "resource/factory/DisplayListFactory.h"
#include "resource/type/DisplayList.h"
#include "spdlog/spdlog.h"

namespace Ship {
std::shared_ptr<Resource> DisplayListFactory::ReadResource(std::shared_ptr<BinaryReader> reader) {
    auto resource = std::make_shared<DisplayList>();
    std::shared_ptr<ResourceVersionFactory> factory = nullptr;

    uint32_t version = reader->ReadUInt32();
    switch (version) {
        case 0:
            factory = std::make_shared<DisplayListFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load DisplayList with version {}", version);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void DisplayListFactoryV0::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) {
    std::shared_ptr<DisplayList> displayList = std::static_pointer_cast<DisplayList>(resource);
    ResourceVersionFactory::ParseFileBinary(reader, displayList);

    while (reader->GetBaseAddress() % 8 != 0) {
        reader->ReadInt8();
    }

    while (true) {
        Gfx command;
        command.words.w0 = reader->ReadUInt32();
        command.words.w1 = reader->ReadUInt32();

        displayList->Instructions.push_back(command);

        uint8_t opcode = (uint8_t)(command.words.w0 >> 24);

        // These are 128-bit commands, so read an extra 64 bits...
        if (opcode == G_SETTIMG_OTR || opcode == G_DL_OTR || opcode == G_VTX_OTR || opcode == G_BRANCH_Z_OTR ||
            opcode == G_MARKER || opcode == G_MTX_OTR) {
            command.words.w0 = reader->ReadUInt32();
            command.words.w1 = reader->ReadUInt32();

            displayList->Instructions.push_back(command);
        }

        if (opcode == G_ENDDL) {
            break;
        }
    }
}
} // namespace Ship
