#include "Array.h"
#include "spdlog/spdlog.h"

namespace Ship {
std::shared_ptr<Resource> ArrayFactory::ReadResource(std::shared_ptr<BinaryReader> reader) {
    auto resource = std::make_shared<Array>();
    std::shared_ptr<ResourceVersionFactory> factory = nullptr;

    uint32_t version = reader->ReadUInt32();
    switch (version) {
        case 0:
            factory = std::make_shared<ArrayFactoryV0>();
            break;
    }

    if (factory == nullptr) {
        SPDLOG_ERROR("Failed to load Array with version {}", version);
        return nullptr;
    }

    factory->ParseFileBinary(reader, resource);

    return resource;
}

void ArrayFactoryV0::ParseFileBinary(std::shared_ptr<BinaryReader> reader, std::shared_ptr<Resource> resource) {
    std::shared_ptr<Array> array = std::static_pointer_cast<Array>(resource);
    ResourceVersionFactory::ParseFileBinary(reader, array);

    array->ArrayType = (ArrayResourceType)reader->ReadUInt32();
    array->ArrayCount = reader->ReadUInt32();

    for (uint32_t i = 0; i < array->ArrayCount; i++) {
        if (array->ArrayType == ArrayResourceType::Vertex) {
            // OTRTODO: Implement Vertex arrays as just a vertex resource.
            Vtx data;
            data.v.ob[0] = reader->ReadInt16();
            data.v.ob[1] = reader->ReadInt16();
            data.v.ob[2] = reader->ReadInt16();
            data.v.flag = reader->ReadUInt16();
            data.v.tc[0] = reader->ReadInt16();
            data.v.tc[1] = reader->ReadInt16();
            data.v.cn[0] = reader->ReadUByte();
            data.v.cn[1] = reader->ReadUByte();
            data.v.cn[2] = reader->ReadUByte();
            data.v.cn[3] = reader->ReadUByte();
            array->Vertices.push_back(data);
        } else {
            array->ArrayScalarType = (ScalarType)reader->ReadUInt32();

            int iter = 1;

            if (array->ArrayType == ArrayResourceType::Vector) {
                iter = reader->ReadUInt32();
            }

            for (int k = 0; k < iter; k++) {
                ScalarData data;

                switch (array->ArrayScalarType) {
                    case ScalarType::ZSCALAR_S16:
                        data.s16 = reader->ReadInt16();
                        break;
                    case ScalarType::ZSCALAR_U16:
                        data.u16 = reader->ReadUInt16();
                        break;
                    default:
                        // OTRTODO: IMPLEMENT OTHER TYPES!
                        break;
                }

                array->Scalars.push_back(data);
            }
        }
    }
}

void* Array::GetPointer() {
    void* dataPointer = nullptr;
    switch (ArrayType) {
        case ArrayResourceType::Vertex:
            dataPointer = Vertices.data();
            break;
        case ArrayResourceType::Scalar:
        default:
            dataPointer = Scalars.data();
            break;
    }

    return dataPointer;
}

size_t Array::GetPointerSize() {
    size_t typeSize = 0;
    switch (ArrayType) {
        case ArrayResourceType::Vertex:
            typeSize = sizeof(Vtx);
            break;
        case ArrayResourceType::Scalar:
        default:
            switch (ArrayScalarType) {
                case ScalarType::ZSCALAR_S16:
                    typeSize = sizeof(int16_t);
                    break;
                case ScalarType::ZSCALAR_U16:
                    typeSize = sizeof(uint16_t);
                    break;
                default:
                    // OTRTODO: IMPLEMENT OTHER TYPES!
                    break;
            }
            break;
    }
    return ArrayCount * typeSize;
}
} // namespace Ship
