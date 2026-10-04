// generate_keys_header.cpp
//
// Reads one or more Ed25519 public-key PEM files and emits a C++ header
// embedding the raw (32-byte, hex-encoded) key data.
//
// Only SubjectPublicKeyInfo ("BEGIN PUBLIC KEY") PEM files are accepted.
// If a private-key PEM is passed the tool exits with an error and a hint.
//
// Usage: generate_keys_header <output.h> <key1.pem> [key2.pem ...]

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

// ─── Base-64 ─────────────────────────────────────────────────────────────────

static int B64Val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == '=') return -2; // padding
    return -1;               // whitespace / ignore
}

static std::vector<uint8_t> Base64Decode(const std::string& s) {
    std::vector<uint8_t> out;
    out.reserve(s.size() * 3 / 4);
    uint32_t acc = 0;
    int bits = 0;
    for (char c : s) {
        int v = B64Val(c);
        if (v == -1) continue;
        if (v == -2) break;
        acc = (acc << 6) | static_cast<uint32_t>(v);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<uint8_t>((acc >> bits) & 0xFF));
        }
    }
    return out;
}

// ─── PEM ─────────────────────────────────────────────────────────────────────

static std::vector<uint8_t> ReadPemDer(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        throw std::runtime_error("Cannot open: " + path);
    }

    std::string line, b64;
    bool inBody = false;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.compare(0, 11, "-----BEGIN ") == 0) {
            if (line.find("PRIVATE") != std::string::npos) {
                throw std::runtime_error(
                    path + ": private key PEM is not supported.\n"
                    "  Extract the public key first, e.g.:\n"
                    "  openssl pkey -in " + path + " -pubout -out public.pem");
            }
            inBody = true;
            continue;
        }
        if (line.compare(0, 9, "-----END ") == 0) break;
        if (inBody) b64 += line;
    }
    if (b64.empty()) throw std::runtime_error("No PEM body found in: " + path);
    return Base64Decode(b64);
}

// ─── DER ─────────────────────────────────────────────────────────────────────

// Scans the DER blob for the Ed25519 SubjectPublicKeyInfo signature:
//
//   06 03 2b 65 70   OID id-Ed25519 (1.3.101.112)
//   03 21 00         BIT STRING, 33 bytes, 0 unused bits
//   <32 bytes>       raw public key
//
// This pattern is unique and deterministic for Ed25519 SPKI blobs.
static std::array<uint8_t, 32> ExtractEd25519PublicKey(const std::vector<uint8_t>& der,
                                                        const std::string& path) {
    static const uint8_t kPattern[] = {
        0x06, 0x03, 0x2b, 0x65, 0x70, // OID id-Ed25519
        0x03, 0x21, 0x00               // BIT STRING header
    };
    constexpr size_t kKeyLen = 32;

    for (size_t i = 0; i + sizeof(kPattern) + kKeyLen <= der.size(); ++i) {
        if (std::memcmp(der.data() + i, kPattern, sizeof(kPattern)) == 0) {
            std::array<uint8_t, 32> key;
            std::memcpy(key.data(), der.data() + i + sizeof(kPattern), kKeyLen);
            return key;
        }
    }
    throw std::runtime_error(path + ": Ed25519 public key pattern not found in DER");
}

// ─── Hex ─────────────────────────────────────────────────────────────────────

static std::string ToHex(const std::array<uint8_t, 32>& b) {
    static const char kDigits[] = "0123456789abcdef";
    std::string s(64, '\0');
    for (int i = 0; i < 32; ++i) {
        s[i * 2]     = kDigits[b[i] >> 4];
        s[i * 2 + 1] = kDigits[b[i] & 0xf];
    }
    return s;
}

// ─── Stem ────────────────────────────────────────────────────────────────────

static std::string PathStem(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = name.rfind('.');
    return (dot == std::string::npos) ? name : name.substr(0, dot);
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: generate_keys_header <output.h> <key1.pem> [key2.pem ...]\n";
        return 1;
    }

    const std::string outputPath = argv[1];
    std::vector<std::pair<std::string, std::string>> keys; // (stem, hexData)

    for (int i = 2; i < argc; ++i) {
        const std::string pemPath = argv[i];
        try {
            auto der = ReadPemDer(pemPath);
            auto key = ExtractEd25519PublicKey(der, pemPath);
            keys.emplace_back(PathStem(pemPath), ToHex(key));
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "Error: cannot write to " << outputPath << "\n";
        return 1;
    }

    out << "#pragma once\n\n"
           "#include <string_view>\n"
           "#include <array>\n"
           "#include <cstddef>\n\n"
           "// Auto-generated — do not edit\n\n"
           "struct DefaultKey {\n"
           "    std::string_view name;\n"
           "    std::string_view data;\n"
           "};\n\n";

    for (auto& [name, hex] : keys)
        out << "inline constexpr std::string_view " << name << "_data = \"" << hex << "\";\n";

    out << "\ninline constexpr std::array<DefaultKey, " << keys.size() << "> AllDefaultKeys = {{\n";
    for (auto& [name, hex] : keys)
        out << "    DefaultKey{ \"" << name << "\", " << name << "_data },\n";
    out << "}};\n\n"
        << "inline constexpr std::size_t AllDefaultKeysSize = " << keys.size() << ";\n";

    return 0;
}
