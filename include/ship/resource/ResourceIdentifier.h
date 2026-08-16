#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <variant>

namespace Ship {
class Archive;

/**
 * @brief Uniquely identifies a cached resource by path/hash, owner, and source archive.
 */
class ResourceIdentifier {
    friend struct ResourceIdentifierHash;

  public:
    ResourceIdentifier() = default;

    ResourceIdentifier(std::string path, const uintptr_t owner = 0, const std::shared_ptr<Archive> parent = nullptr)
        : mPath(std::move(path)), mOwner(owner), mParent(parent) {
    }

    ResourceIdentifier(uint64_t hash, const uintptr_t owner = 0, const std::shared_ptr<Archive> parent = nullptr)
        : mPath(hash), mOwner(owner), mParent(parent) {
    }

    bool operator==(const ResourceIdentifier& rhs) const {
        return mOwner == rhs.mOwner && mPath == rhs.mPath && mParent == rhs.mParent;
    }

    bool IsPath() const {
        return std::holds_alternative<std::string>(mPath);
    }

    bool IsHash() const {
        return std::holds_alternative<uint64_t>(mPath);
    }

    const std::string& GetPath() const {
        static const std::string emptyPath = "";
        return IsPath() ? std::get<std::string>(mPath) : emptyPath;
    }

    uint64_t GetPathHash() const {
        return IsHash() ? std::get<uint64_t>(mPath) : 0;
    }

    const std::variant<std::string, uint64_t>& GetPathOrHash() const {
        return mPath;
    }

    uintptr_t GetOwner() const {
        return mOwner;
    }

    const std::shared_ptr<Archive>& GetParent() const {
        return mParent;
    }

    void SetPath(const std::string& path) {
        mPath = path;
    }

    void SetPath(std::string&& path) {
        mPath = std::move(path);
    }

    void SetHash(uint64_t hash) {
        mPath = hash;
    }

    void SetPathOrHash(std::variant<std::string, uint64_t> pathOrHash) {
        mPath = std::move(pathOrHash);
    }

    void SetOwner(uintptr_t owner) {
        mOwner = owner;
    }

    void SetParent(const std::shared_ptr<Archive>& parent) {
        mParent = parent;
    }

  private:
    static size_t HashCombine(size_t lhs, size_t rhs) {
        lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
        return lhs;
    }

    size_t GetHash() const {
        return CalculateHash();
    }

    size_t CalculateHash() const {
        size_t pathHash =
            HashCombine(std::hash<size_t>{}(mPath.index()),
                        IsPath() ? std::hash<std::string>{}(GetPath()) : std::hash<uint64_t>{}(GetPathHash()));

        size_t hash = HashCombine(pathHash, std::hash<std::uintptr_t>{}(mOwner));
        hash = HashCombine(hash, std::hash<std::shared_ptr<Archive>>{}(mParent));
        return hash;
    }

    std::variant<std::string, uint64_t> mPath = std::string();
    uintptr_t mOwner = 0;
    std::shared_ptr<Archive> mParent = nullptr;
};

/**
 * @brief std::hash specialization for ResourceIdentifier, used by unordered containers.
 */
struct ResourceIdentifierHash {
    size_t operator()(const ResourceIdentifier& rcd) const {
        return rcd.GetHash();
    }
};
} // namespace Ship
