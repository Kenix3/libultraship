#include "CollisionHeaderFactory.h"

namespace Ship
{
    CollisionHeader* CollisionHeaderFactory::ReadCollisionHeader(BinaryReader* reader)
    {
        CollisionHeader* colHeader = new CollisionHeader();

        Version version = (Version)reader->ReadUInt32();

        switch (version)
        {
        case Version::Deckard:
        {
            CollisionHeaderV0 otrCol = CollisionHeaderV0();
            otrCol.ParseFileBinary(reader, colHeader);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return colHeader;
    }
};