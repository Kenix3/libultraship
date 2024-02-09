#include "resource/factory/MatrixFactory.h"
#include "resource/type/Matrix.h"
#include "resource/readerbox/BinaryReaderBox.h"
#include "spdlog/spdlog.h"

namespace LUS {
std::shared_ptr<IResource> ResourceFactoryBinaryMatrixV0::ReadResource(std::shared_ptr<ResourceInitData> initData,
                                                        std::shared_ptr<ReaderBox> readerBox) {
    auto binaryReaderBox = std::dynamic_pointer_cast<BinaryReaderBox>(readerBox);
    if (binaryReaderBox == nullptr) {
        SPDLOG_ERROR("ReaderBox must be a BinaryReaderBox.");
        return nullptr;
    }

    auto reader = binaryReaderBox->GetReader();
    if (reader == nullptr) {
        SPDLOG_ERROR("null reader in box.");
        return nullptr;
    }

    auto matrix = std::make_shared<Matrix>(initData);

    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            matrix->Matrx.m[i][j] = reader->ReadInt32();
        }
    }

    return matrix;
}
} // namespace LUS
