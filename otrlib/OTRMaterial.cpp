#include "OTRMaterial.h"

namespace OtrLib
{
    void OTRMaterialV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
    {
        OTRMaterial* mat = (OTRMaterial*)res;

        OTRResourceFile::ParseFileBinary(reader, res);

        cmtH = (OTRMaterialCmt)reader->ReadUByte();
        cmtV = (OTRMaterialCmt)reader->ReadUByte();

        clrR = reader->ReadUByte();
        clrG = reader->ReadUByte();
        clrB = reader->ReadUByte();
        clrA = reader->ReadUByte();
        clrM = reader->ReadUByte();
        clrL = reader->ReadUByte();

        shaderID = reader->ReadUInt32();
        shaderParamsCnt = reader->ReadUInt32();
        offsetToShaderEntries = reader->ReadUInt32();

        mat->shaderID = shaderID;
        for (int i = 0; i < shaderParamsCnt; i++)
        {
            mat->shaderParams.push_back(new OTRShaderParam(reader));
        }

        mat->cmtH = cmtH;
        mat->cmtV = cmtV;

        mat->clrR = clrR;
        mat->clrG = clrG;
        mat->clrB = clrB;
        mat->clrA = clrA;
        mat->clrM = clrM;
        mat->clrL = clrL;
    }

    OTRShaderParam::OTRShaderParam(BinaryReader* reader)
    {
        name = reader->ReadUInt32();
        dataType = (DataType)reader->ReadByte();

        switch (dataType)
        {
        case DataType::U8:
            value = reader->ReadUByte();
            break;
        case DataType::S8:
            value = reader->ReadByte();
            break;
        case DataType::U16:
            value = reader->ReadUInt16();
            break;
        case DataType::S16:
            value = reader->ReadInt16();
            break;
        case DataType::U32:
            value = reader->ReadUInt32();
            break;
        case DataType::S32:
            value = reader->ReadInt32();
            break;
        default:
            // TODO: ERROR GOES HERE
            break;
        }
    }
}