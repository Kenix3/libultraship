#include "resource/factory/LightFactory.h"
#include "resource/type/Light.h"

std::shared_ptr<Ship::IResource>
Fast::ResourceFactoryBinaryLightV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                 std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    std::shared_ptr<Light> light = std::make_shared<Light>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    reader->Read((char*)light->GetPointer(), sizeof(LightEntry));

    return light;
}