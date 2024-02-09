#include "ReaderBox.h"
#include <tinyxml2.h>
#include <memory>

namespace LUS {
class XMLReaderBox : public ReaderBox {
    public:
        XMLReaderBox(std::shared_ptr<tinyxml2::XMLDocument> reader) {
            mReader = reader;
        };
        std::shared_ptr<tinyxml2::XMLDocument> GetReader() {
            return mReader;
        };
        ~XMLReaderBox() {
            mReader = nullptr;
        };

    private:
        std::shared_ptr<tinyxml2::XMLDocument> mReader;
};
} // namespace LUS