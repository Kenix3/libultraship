#include "resource/factory/ArrayFactory.h"
#include "resource/type/Array.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryArrayV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto array = std::make_shared<Array>(file->InitData);

    uint32_t dataSize = file->Reader->ReadUInt32();

    array->ArrayType = (ArrayResourceType)file->Reader->ReadUInt32();
    array->ArrayCount = file->Reader->ReadUInt32();

    for (uint32_t i = 0; i < array->ArrayCount; i++) {
        if (array->ArrayType == ArrayResourceType::Vertex) {
            // OTRTODO: Implement Vertex arrays as just a vertex resource.
            Vtx data;
            data.v.ob[0] = file->Reader->ReadInt16();
            data.v.ob[1] = file->Reader->ReadInt16();
            data.v.ob[2] = file->Reader->ReadInt16();
            data.v.flag = file->Reader->ReadUInt16();
            data.v.tc[0] = file->Reader->ReadInt16();
            data.v.tc[1] = file->Reader->ReadInt16();
            data.v.cn[0] = file->Reader->ReadUByte();
            data.v.cn[1] = file->Reader->ReadUByte();
            data.v.cn[2] = file->Reader->ReadUByte();
            data.v.cn[3] = file->Reader->ReadUByte();
            array->Vertices.push_back(data);
        } else {
            array->ArrayScalarType = (ScalarType)file->Reader->ReadUInt32();

            int iter = 1;

            if (array->ArrayType == ArrayResourceType::Vector) {
                iter = file->Reader->ReadUInt32();
            }

            for (int k = 0; k < iter; k++) {
                ScalarData data;

                switch (array->ArrayScalarType) {
                    case ScalarType::ZSCALAR_S16:
                        data.s16 = file->Reader->ReadInt16();
                        break;
                    case ScalarType::ZSCALAR_U16:
                        data.u16 = file->Reader->ReadUInt16();
                        break;
                    default:
                        // OTRTODO: IMPLEMENT OTHER TYPES!
                        break;
                }

                array->Scalars.push_back(data);
            }
        }
    }

    return array;
}
} // namespace LUS
