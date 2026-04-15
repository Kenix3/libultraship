#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <variant>
#include <vector>

#include "ship/resource/File.h"
#include "ship/resource/Resource.h"
#include "ship/resource/ResourceLoader.h"
#include "ship/resource/ResourceType.h"
#include "ship/resource/factory/BlobFactory.h"
#include "ship/resource/factory/JsonFactory.h"
#include "ship/resource/factory/ShaderFactory.h"
#include "ship/resource/type/Blob.h"
#include "ship/resource/type/Json.h"
#include "ship/resource/type/Shader.h"
#include "ship/utils/binarytools/BinaryReader.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include "ship/utils/binarytools/endianness.h"

// ============================================================
// Helpers
// ============================================================

static std::shared_ptr<Ship::ResourceInitData> MakeBinaryInitData(bool isCustom = false) {
    auto id = std::make_shared<Ship::ResourceInitData>();
    id->Format = RESOURCE_FORMAT_BINARY;
    id->ByteOrder = Ship::Endianness::Native;
    id->Type = 0;
    id->ResourceVersion = 0;
    id->IsCustom = isCustom;
    id->Id = 0xDEADBEEFDEADBEEFULL;
    id->Path = "test/path";
    return id;
}

static std::shared_ptr<Ship::ResourceInitData> MakeXmlInitData() {
    auto id = MakeBinaryInitData();
    id->Format = RESOURCE_FORMAT_XML;
    return id;
}

// Build a File that already has a BinaryReader pointing at the given bytes
static std::shared_ptr<Ship::File> MakeBinaryFile(const std::vector<char>& body,
                                                  std::shared_ptr<Ship::ResourceInitData> initData) {
    auto file = std::make_shared<Ship::File>();
    file->Buffer = std::make_shared<std::vector<char>>(body);
    file->IsLoaded = true;
    auto stream = std::make_shared<Ship::MemoryStream>(file->Buffer);
    auto reader = std::make_shared<Ship::BinaryReader>(stream);
    reader->SetEndianness(initData->ByteOrder);
    file->Reader = reader;
    return file;
}

// Build a File that has an XML reader (to trigger "wrong reader" error path)
static std::shared_ptr<Ship::File> MakeXmlReaderFile() {
    auto file = std::make_shared<Ship::File>();
    file->Buffer = std::make_shared<std::vector<char>>();
    file->IsLoaded = true;
    // Reader variant holds an XMLDocument pointer, not a BinaryReader
    file->Reader = std::shared_ptr<tinyxml2::XMLDocument>(nullptr);
    return file;
}

// ============================================================
// IResource (via Blob default constructor)
// ============================================================

TEST(IResource, NewResourceIsNotDirty) {
    Ship::Blob blob;
    EXPECT_FALSE(blob.IsDirty());
}

TEST(IResource, DirtyMarksDirty) {
    Ship::Blob blob;
    blob.Dirty();
    EXPECT_TRUE(blob.IsDirty());
}

TEST(IResource, GetInitDataNullForDefaultConstructor) {
    Ship::Blob blob;
    EXPECT_EQ(blob.GetInitData(), nullptr);
}

TEST(IResource, GetInitDataReturnsSuppliedInitData) {
    auto initData = MakeBinaryInitData();
    Ship::Blob blob(initData);
    ASSERT_NE(blob.GetInitData(), nullptr);
    EXPECT_EQ(blob.GetInitData()->Path, "test/path");
    EXPECT_EQ(blob.GetInitData()->IsCustom, false);
}

// ============================================================
// Blob resource type
// ============================================================

TEST(BlobResource, DefaultConstructorHasEmptyData) {
    Ship::Blob blob;
    EXPECT_TRUE(blob.Data.empty());
}

TEST(BlobResource, GetPointerReturnsDataPointer) {
    Ship::Blob blob;
    EXPECT_EQ(blob.GetPointer(), blob.Data.data());
}

TEST(BlobResource, GetRawPointerEqualsGetPointer) {
    Ship::Blob blob;
    blob.Data = { 1, 2, 3 };
    EXPECT_EQ(blob.GetRawPointer(), static_cast<void*>(blob.GetPointer()));
}

TEST(BlobResource, GetPointerSizeReflectsDataSize) {
    Ship::Blob blob;
    EXPECT_EQ(blob.GetPointerSize(), 0u);
    blob.Data = { 0xAA, 0xBB, 0xCC };
    EXPECT_EQ(blob.GetPointerSize(), 3u);
}

// ============================================================
// Json resource type
// ============================================================

TEST(JsonResource, DefaultConstructorHasEmptyJsonData) {
    Ship::Json json;
    // DataSize is uninitialized in the default constructor; just verify
    // GetPointer() returns the address of the Data member
    EXPECT_EQ(json.GetPointer(), static_cast<void*>(&json.Data));
}

TEST(JsonResource, GetPointerReturnsAddressOfData) {
    Ship::Json json;
    EXPECT_EQ(json.GetPointer(), static_cast<void*>(&json.Data));
}

TEST(JsonResource, GetPointerSizeReflectsDataSize) {
    Ship::Json json;
    json.DataSize = 42;
    EXPECT_EQ(json.GetPointerSize(), 42u);
}

// ============================================================
// Shader resource type
// ============================================================

TEST(ShaderResource, DefaultConstructorHasEmptyData) {
    Ship::Shader shader;
    EXPECT_TRUE(shader.Data.empty());
}

TEST(ShaderResource, GetPointerReturnsAddressOfData) {
    Ship::Shader shader;
    EXPECT_EQ(shader.GetPointer(), static_cast<void*>(&shader.Data));
}

TEST(ShaderResource, GetPointerSizeReflectsDataSize) {
    Ship::Shader shader;
    shader.Data = "void main(){}";
    EXPECT_EQ(shader.GetPointerSize(), shader.Data.size());
}

// ============================================================
// ResourceFactoryBinaryBlobV0
// ============================================================

TEST(BlobFactory, WrongReaderTypeReturnsNull) {
    Ship::ResourceFactoryBinaryBlobV0 factory;
    auto initData = MakeBinaryInitData();
    auto file = MakeXmlReaderFile(); // has XMLDocument reader, not BinaryReader
    EXPECT_EQ(factory.ReadResource(file, initData), nullptr);
}

TEST(BlobFactory, ValidInputProducesBlob) {
    Ship::ResourceFactoryBinaryBlobV0 factory;
    auto initData = MakeBinaryInitData();

    // Body: uint32 dataSize=3, then 3 bytes
    std::vector<char> body;
    uint32_t size = 3;
    body.resize(sizeof(uint32_t));
    std::memcpy(body.data(), &size, sizeof(uint32_t));
    body.push_back(static_cast<char>(0x01));
    body.push_back(static_cast<char>(0x02));
    body.push_back(static_cast<char>(0x03));

    auto file = MakeBinaryFile(body, initData);
    auto resource = factory.ReadResource(file, initData);
    ASSERT_NE(resource, nullptr);

    auto blob = std::dynamic_pointer_cast<Ship::Blob>(resource);
    ASSERT_NE(blob, nullptr);
    ASSERT_GE(blob->Data.size(), 3u);
    EXPECT_EQ(blob->Data[0], 0x01);
    EXPECT_EQ(blob->Data[1], 0x02);
    EXPECT_EQ(blob->Data[2], 0x03);
}

TEST(BlobFactory, ZeroSizeBlobHasPaddingOnly) {
    Ship::ResourceFactoryBinaryBlobV0 factory;
    auto initData = MakeBinaryInitData();

    // Body: uint32 dataSize=0, no data bytes
    std::vector<char> body(sizeof(uint32_t), 0); // size = 0
    auto file = MakeBinaryFile(body, initData);
    auto resource = factory.ReadResource(file, initData);
    ASSERT_NE(resource, nullptr);

    auto blob = std::dynamic_pointer_cast<Ship::Blob>(resource);
    ASSERT_NE(blob, nullptr);
    // 0 data + 16-byte padding
    EXPECT_EQ(blob->Data.size(), 16u);
    for (uint8_t byte : blob->Data) {
        EXPECT_EQ(byte, 0);
    }
}

TEST(BlobFactory, LargerPayloadReadCorrectly) {
    Ship::ResourceFactoryBinaryBlobV0 factory;
    auto initData = MakeBinaryInitData();

    const uint32_t count = 128;
    std::vector<char> body(sizeof(uint32_t) + count);
    std::memcpy(body.data(), &count, sizeof(uint32_t));
    for (uint32_t i = 0; i < count; i++) {
        body[sizeof(uint32_t) + i] = static_cast<char>(i & 0xFF);
    }

    auto file = MakeBinaryFile(body, initData);
    auto resource = factory.ReadResource(file, initData);
    ASSERT_NE(resource, nullptr);
    auto blob = std::dynamic_pointer_cast<Ship::Blob>(resource);
    ASSERT_NE(blob, nullptr);
    ASSERT_GE(blob->Data.size(), count);
    for (uint32_t i = 0; i < count; i++) {
        EXPECT_EQ(blob->Data[i], i & 0xFF) << "at index " << i;
    }
}

// ============================================================
// ResourceFactoryBinaryJsonV0
// ============================================================

TEST(JsonFactory, WrongReaderTypeReturnsNull) {
    Ship::ResourceFactoryBinaryJsonV0 factory;
    auto initData = MakeBinaryInitData();
    auto file = MakeXmlReaderFile();
    EXPECT_EQ(factory.ReadResource(file, initData), nullptr);
}

TEST(JsonFactory, ValidJsonObjectProducesResource) {
    Ship::ResourceFactoryBinaryJsonV0 factory;
    auto initData = MakeBinaryInitData();

    std::string jsonStr = R"({"key":"value","count":42})";
    std::vector<char> body(jsonStr.begin(), jsonStr.end());
    body.push_back('\0'); // ReadCString expects null terminator

    auto file = MakeBinaryFile(body, initData);
    auto resource = factory.ReadResource(file, initData);
    ASSERT_NE(resource, nullptr);

    auto json = std::dynamic_pointer_cast<Ship::Json>(resource);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->Data["key"].get<std::string>(), "value");
    EXPECT_EQ(json->Data["count"].get<int>(), 42);
}

TEST(JsonFactory, JsonArrayProducesResource) {
    Ship::ResourceFactoryBinaryJsonV0 factory;
    auto initData = MakeBinaryInitData();

    std::string jsonStr = R"([1,2,3])";
    std::vector<char> body(jsonStr.begin(), jsonStr.end());
    body.push_back('\0');

    auto file = MakeBinaryFile(body, initData);
    auto resource = factory.ReadResource(file, initData);
    ASSERT_NE(resource, nullptr);

    auto json = std::dynamic_pointer_cast<Ship::Json>(resource);
    ASSERT_NE(json, nullptr);
    ASSERT_TRUE(json->Data.is_array());
    EXPECT_EQ(json->Data.size(), 3u);
}

TEST(JsonFactory, DataSizeMatchesBufferSize) {
    Ship::ResourceFactoryBinaryJsonV0 factory;
    auto initData = MakeBinaryInitData();

    std::string jsonStr = R"({"x":1})";
    std::vector<char> body(jsonStr.begin(), jsonStr.end());
    body.push_back('\0');

    auto file = MakeBinaryFile(body, initData);
    auto resource = factory.ReadResource(file, initData);
    auto json = std::dynamic_pointer_cast<Ship::Json>(resource);
    ASSERT_NE(json, nullptr);
    EXPECT_EQ(json->DataSize, body.size());
}

// ============================================================
// ResourceFactoryBinaryShaderV0
// ============================================================

TEST(ShaderFactory, WrongReaderTypeReturnsNull) {
    Ship::ResourceFactoryBinaryShaderV0 factory;
    auto initData = MakeBinaryInitData();
    auto file = MakeXmlReaderFile();
    EXPECT_EQ(factory.ReadResource(file, initData), nullptr);
}

TEST(ShaderFactory, ValidShaderSourceProducesResource) {
    Ship::ResourceFactoryBinaryShaderV0 factory;
    auto initData = MakeBinaryInitData();

    std::string src = "void main() { gl_Position = vec4(0.0); }";
    std::vector<char> body(src.begin(), src.end());
    body.push_back('\0');

    auto file = MakeBinaryFile(body, initData);
    auto resource = factory.ReadResource(file, initData);
    ASSERT_NE(resource, nullptr);

    auto shader = std::dynamic_pointer_cast<Ship::Shader>(resource);
    ASSERT_NE(shader, nullptr);
    // ReadCString includes the null terminator in the returned string;
    // compare the content up to (but not including) the trailing '\0'.
    ASSERT_GE(shader->Data.size(), src.size());
    EXPECT_EQ(shader->Data.substr(0, src.size()), src);
}

TEST(ShaderFactory, EmptyShaderSourceProducesResourceWithNullByte) {
    Ship::ResourceFactoryBinaryShaderV0 factory;
    auto initData = MakeBinaryInitData();

    // ReadCString stops at (and includes) the first '\0'.
    // An input of just '\0' produces a one-character string containing '\0'.
    std::vector<char> body = { '\0' };
    auto file = MakeBinaryFile(body, initData);
    auto resource = factory.ReadResource(file, initData);
    ASSERT_NE(resource, nullptr);

    auto shader = std::dynamic_pointer_cast<Ship::Shader>(resource);
    ASSERT_NE(shader, nullptr);
    // Data contains exactly the null byte
    EXPECT_EQ(shader->Data.size(), 1u);
    EXPECT_EQ(shader->Data[0], '\0');
}

// ============================================================
// ResourceLoader
// ============================================================

TEST(ResourceLoader, DefaultLoaderHasJsonRegistered) {
    Ship::ResourceLoader loader;
    EXPECT_EQ(loader.GetResourceType("Json"), static_cast<uint32_t>(Ship::ResourceType::Json));
}

TEST(ResourceLoader, DefaultLoaderHasShaderRegistered) {
    Ship::ResourceLoader loader;
    EXPECT_EQ(loader.GetResourceType("Shader"), static_cast<uint32_t>(Ship::ResourceType::Shader));
}

TEST(ResourceLoader, UnknownTypeReturnsNone) {
    Ship::ResourceLoader loader;
    EXPECT_EQ(loader.GetResourceType("NonExistentType"), static_cast<uint32_t>(Ship::ResourceType::None));
}

TEST(ResourceLoader, RegisterNewFactorySucceeds) {
    Ship::ResourceLoader loader;
    auto factory = std::make_shared<Ship::ResourceFactoryBinaryBlobV0>();
    bool ok = loader.RegisterResourceFactory(factory, RESOURCE_FORMAT_BINARY, "Blob",
                                             static_cast<uint32_t>(Ship::ResourceType::Blob), 0);
    EXPECT_TRUE(ok);
    EXPECT_EQ(loader.GetResourceType("Blob"), static_cast<uint32_t>(Ship::ResourceType::Blob));
}

TEST(ResourceLoader, RegisterDuplicateKeyRejected) {
    Ship::ResourceLoader loader;
    // Json already registered by constructor with version 0
    auto factory = std::make_shared<Ship::ResourceFactoryBinaryJsonV0>();
    bool ok = loader.RegisterResourceFactory(factory, RESOURCE_FORMAT_BINARY, "Json",
                                             static_cast<uint32_t>(Ship::ResourceType::Json), 0);
    EXPECT_FALSE(ok); // same format+type+version key already exists
}

TEST(ResourceLoader, RegisterConflictingTypeNameRejected) {
    Ship::ResourceLoader loader;
    // "Json" name already maps to ResourceType::Json; try to register it with a different type value
    auto factory = std::make_shared<Ship::ResourceFactoryBinaryBlobV0>();
    bool ok = loader.RegisterResourceFactory(factory, RESOURCE_FORMAT_BINARY, "Json",
                                             static_cast<uint32_t>(Ship::ResourceType::Blob), 99);
    EXPECT_FALSE(ok); // name conflicts with existing type mapping
}

TEST(ResourceLoader, RegisterMultipleVersionsOfSameTypePossible) {
    Ship::ResourceLoader loader;
    // Register Blob at two different versions
    bool ok0 =
        loader.RegisterResourceFactory(std::make_shared<Ship::ResourceFactoryBinaryBlobV0>(), RESOURCE_FORMAT_BINARY,
                                       "Blob", static_cast<uint32_t>(Ship::ResourceType::Blob), 0);
    bool ok1 =
        loader.RegisterResourceFactory(std::make_shared<Ship::ResourceFactoryBinaryBlobV0>(), RESOURCE_FORMAT_BINARY,
                                       "Blob", static_cast<uint32_t>(Ship::ResourceType::Blob), 1);
    EXPECT_TRUE(ok0);
    EXPECT_TRUE(ok1);
}
