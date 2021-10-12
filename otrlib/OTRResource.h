#pragma once

#include <stdint.h>
#include "Utils/BinaryReader.h"
#include "Utils/BinaryWriter.h"
#include "StrHash.h"
#include "../lib/tinyxml2/tinyxml2.h"

namespace OtrLib
{
    enum class ResourceType
    {
        OTRArchive      =   0x4F415243, // OARC
        OTRModel        =   0x4F4D444C, // OMDL
        OTRTexture      =   0x4F544558, // OTEX
        OTRMaterial     =   0x4F4D4154, // OMAT
        OTRAnimation    =   0x4F414E4D, // OANM
        OTRDisplayList  =   0x4F444C54, // ODLT
    };

    enum class DataType
    {
        U8 = 0,
        S8 = 1,
        U16 = 2,
        S16 = 3,
        U32 = 4,
        S32 = 5,
        U64 = 6,
        S64 = 7,
        F16 = 8,
        F32 = 9,
        F64 = 10
    };

    enum class Endianess
    {
        Little = 0,
        Big = 1,
    };
    
    /*
        - Blade Runner:        1982
        - Tron:                1982
        - The Thing:           1982
        - Star Wars ROJ:       1983
        - Predator:            1984
        - Terminator:          1984
        - Mad Max Thunderdome: 1985
        - Back to the Future:  1985
        - Aliens:              1986
        - Star Trek TNG:       1987
        - Robocop:             1987
        - Spaceballs:          1987
        - Terminator 2:        1991
        - Jurassic Park:       1993
        - Demolition Man:      1993
        - Judge Dredd:         1995
        - Star Trek Voyager:   1995
        - The Fifth Element:   1995
        - Waterworld:          1995
        - Stargate:            1997
        - Starship Troopers:   1997
        - Men in Black:        1997
        - Armageddon:          1998
        - Blade:               1998
        - Matrix:              1999
        - Star Wars TPM:       1999
    */

    enum class OTRVersion
    {
        // Blade Runner
        Deckard = 0,
        // ...
    };

    class OTRResource
    {
    public:
        uint64_t id; // Unique Resource ID
        bool isDirty = false;
    };

    class OTRResourceFile
    {
    public:
        Endianess endianess;    // 0x00 - Endianess of the file
        uint32_t resourceType;  // 0x01 - 4 byte MAGIC
        OTRVersion version;     // 0x05 - Based on OTR release numbers
        uint64_t id;            // 0x09 - Unique Resource ID

        virtual void ParseFileBinary(BinaryReader* reader, OTRResource* res);
        virtual void ParseFileXML(tinyxml2::XMLElement* reader, OTRResource* res);
        virtual void WriteFileBinary(BinaryWriter* writer, OTRResource* res);
        virtual void WriteFileXML(tinyxml2::XMLElement* writer, OTRResource* res);
    };
}