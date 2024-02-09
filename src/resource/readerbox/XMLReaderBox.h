#include "ReaderBox.h"
#include <tinyxml2.h>
#include <memory>

namespace LUS {
class XMLReaderBox : public ReaderBox {
    public:
        XMLReaderBox(std::shared_ptr<tinyxml2::XMLElement> reader) {
            mReader = reader;
        };
        std::shared_ptr<tinyxml2::XMLElement> GetReader() {
            return mReader;
        };
        ~XMLReaderBox() {
            mReader = nullptr;
        };

    private:
        std::shared_ptr<tinyxml2::XMLElement> mReader;
};
} // namespace LUS