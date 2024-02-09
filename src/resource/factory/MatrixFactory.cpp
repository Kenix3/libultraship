#include "resource/factory/MatrixFactory.h"
#include "resource/type/Matrix.h"
#include "resource/readerbox/BinaryReaderBox.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryMatrixV0::ReadResource(std::shared_ptr<File> file) {
    if (file->InitData->Format != RESOURCE_FORMAT_BINARY) {
        SPDLOG_ERROR("resource file format does not match factory format.");
        return nullptr;
    }

    if (file->Reader == nullptr) {
        SPDLOG_ERROR("Failed to load resource: File has Reader ({} - {})", file->InitData->Type,
                        file->InitData->Path);
        return nullptr;
    }

    auto matrix = std::make_shared<Matrix>(file->InitData);

    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            matrix->Matrx.m[i][j] = file->Reader->ReadInt32();
        }
    }

    return matrix;
}
} // namespace LUS
