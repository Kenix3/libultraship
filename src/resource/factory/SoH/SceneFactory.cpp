#include "SceneFactory.h"

namespace Ship {
Scene* SceneFactory::ReadScene(BinaryReader* reader) {
    Scene* scene = new Scene();

    Version version = (Version)reader->ReadUInt32();

    switch (version) {
        case Version::Deckard: {
            SceneV0 sceneFac = SceneV0();
            sceneFac.ParseFileBinary(reader, scene);
        } break;
        default:
            // VERSION NOT SUPPORTED
            break;
    }

    return scene;
}
} // namespace Ship