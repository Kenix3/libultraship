#include "PathFactory.h"

namespace Ship
{
    Path* PathFactory::ReadPath(BinaryReader* reader)
    {
        Path* path = new Path();
        Version version = (Version)reader->ReadUInt32();

        switch (version)
        {
        case Version::Deckard:
        {
            PathV0 otrPath;
            otrPath.ParseFileBinary(reader, path);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return path;
    }
};