#include "ReaderBox.h"
#include "utils/binarytools/BinaryReader.h"

namespace LUS {
class BinaryReaderBox : public ReaderBox {
    public:
        BinaryReaderBox(std::shared_ptr<BinaryReader> reader) {
            mReader = reader;
        };
        std::shared_ptr<BinaryReader> GetReader() {
            return mReader;
        };
        ~BinaryReaderBox() {
            mReader = nullptr;
        };

    private:
        std::shared_ptr<BinaryReader> mReader;
};
} // namespace LUS
