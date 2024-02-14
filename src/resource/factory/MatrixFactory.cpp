#include "resource/factory/MatrixFactory.h"
#include "resource/type/Matrix.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryMatrixV0::ReadResource(std::shared_ptr<File> file) {
    if (!FileHasValidFormatAndReader(file)) {
        return nullptr;
    }

    auto matrix = std::make_shared<Matrix>(file->InitData);
    auto reader = std::get<std::shared_ptr<BinaryReader>>(file->Reader);

    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            matrix->Matrx.m[i][j] = reader->ReadInt32();
        }
    }

    return matrix;
}
} // namespace LUS
