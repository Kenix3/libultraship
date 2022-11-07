#include "MaterialFactory.h"

namespace Ship {
Material* MaterialFactory::ReadMaterial(BinaryReader* reader) {
    Material* mat = new Material();

    Version version = (Version)reader->ReadUInt32();

    switch (version) {
        case Version::Deckard: {
            MaterialV0 matFac = MaterialV0();
            matFac.ParseFileBinary(reader, mat);
        } break;
        default:
            // VERSION NOT SUPPORTED
            break;
    }

    return mat;
}
} // namespace Ship