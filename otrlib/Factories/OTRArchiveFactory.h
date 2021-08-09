#include "../OTRArchive.h"
#include "../Lib/BinaryReader.h"

namespace OtrLib
{
    class OTRArchiveFactory
    {
    public:
        static OTRArchive* CreateArchive();
        static OTRArchive* ReadArchive(BinaryReader* reader);
        static void WriteArchive(BinaryWriter* writer, OTRArchive* archive);
    };
}