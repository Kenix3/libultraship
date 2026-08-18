#include "fast/resource/factory/DisplayListFactory.h"
#include "fast/resource/type/DisplayList.h"
#include "spdlog/spdlog.h"
#include "libultraship/libultra/gbi.h"
#include "libultraship/libultra/gs2dex.h"
#include "fast/lus_gbi.h"
#include "ship/utils/StrHash64.h"
#include <tinyxml2.h>

namespace Fast {
std::unordered_map<std::string, uint32_t> renderModes = {
    { "G_RM_ZB_OPA_SURF", G_RM_ZB_OPA_SURF },
    { "G_RM_AA_ZB_OPA_SURF", G_RM_AA_ZB_OPA_SURF },
    { "G_RM_AA_ZB_OPA_DECAL", G_RM_AA_ZB_OPA_DECAL },
    { "G_RM_AA_ZB_OPA_INTER", G_RM_AA_ZB_OPA_INTER },
    { "G_RM_AA_ZB_TEX_EDGE", G_RM_AA_ZB_TEX_EDGE },
    { "G_RM_AA_ZB_XLU_SURF", G_RM_AA_ZB_XLU_SURF },
    { "G_RM_AA_ZB_XLU_DECAL", G_RM_AA_ZB_XLU_DECAL },
    { "G_RM_AA_ZB_XLU_INTER", G_RM_AA_ZB_XLU_INTER },
    { "G_RM_FOG_SHADE_A", G_RM_FOG_SHADE_A },
    { "G_RM_FOG_PRIM_A", G_RM_FOG_PRIM_A },
    { "G_RM_PASS", G_RM_PASS },
    { "G_RM_ADD", G_RM_ADD },
    { "G_RM_NOOP", G_RM_NOOP },
    { "G_RM_ZB_OPA_SURF", G_RM_ZB_OPA_SURF },
    { "G_RM_ZB_OPA_DECAL", G_RM_ZB_OPA_DECAL },
    { "G_RM_ZB_XLU_SURF", G_RM_ZB_XLU_SURF },
    { "G_RM_ZB_XLU_DECAL", G_RM_ZB_XLU_DECAL },
    { "G_RM_OPA_SURF", G_RM_OPA_SURF },
    { "G_RM_ZB_CLD_SURF", G_RM_ZB_CLD_SURF },
    { "G_RM_ZB_OPA_SURF2", G_RM_ZB_OPA_SURF2 },
    { "G_RM_AA_ZB_OPA_SURF2", G_RM_AA_ZB_OPA_SURF2 },
    { "G_RM_AA_ZB_OPA_DECAL2", G_RM_AA_ZB_OPA_DECAL2 },
    { "G_RM_AA_ZB_OPA_INTER2", G_RM_AA_ZB_OPA_INTER2 },
    { "G_RM_AA_ZB_TEX_EDGE2", G_RM_AA_ZB_TEX_EDGE2 },
    { "G_RM_AA_ZB_XLU_SURF2", G_RM_AA_ZB_XLU_SURF2 },
    { "G_RM_AA_ZB_XLU_DECAL2", G_RM_AA_ZB_XLU_DECAL2 },
    { "G_RM_AA_ZB_XLU_INTER2", G_RM_AA_ZB_XLU_INTER2 },
    { "G_RM_ADD2", G_RM_ADD2 },
    { "G_RM_ZB_OPA_SURF2", G_RM_ZB_OPA_SURF2 },
    { "G_RM_ZB_OPA_DECAL2", G_RM_ZB_OPA_DECAL2 },
    { "G_RM_ZB_XLU_SURF2", G_RM_ZB_XLU_SURF2 },
    { "G_RM_ZB_XLU_DECAL2", G_RM_ZB_XLU_DECAL2 },
    { "G_RM_ZB_CLD_SURF2", G_RM_ZB_CLD_SURF2 },
};

const std::unordered_map<std::string, uint32_t> imageFormats = {
    { "G_IM_FMT_RGBA", G_IM_FMT_RGBA }, { "G_IM_FMT_YUV", G_IM_FMT_YUV }, { "G_IM_FMT_CI", G_IM_FMT_CI },
    { "G_IM_FMT_IA", G_IM_FMT_IA },     { "G_IM_FMT_I", G_IM_FMT_I },
};

const std::unordered_map<std::string, uint32_t> imageSizes = {
    { "G_IM_SIZ_4b", G_IM_SIZ_4b },
    { "G_IM_SIZ_8b", G_IM_SIZ_8b },
    { "G_IM_SIZ_16b", G_IM_SIZ_16b },
    { "G_IM_SIZ_32b", G_IM_SIZ_32b },
};

const std::unordered_map<std::string, uint32_t> depthSourceModes = {
    { "G_ZS_PIXEL", G_ZS_PIXEL },
    { "G_ZS_PRIM", G_ZS_PRIM },
};

const std::unordered_map<std::string, uint32_t> alphaCompareModes = {
    { "G_AC_NONE", G_AC_NONE },
    { "G_AC_THRESHOLD", G_AC_THRESHOLD },
    { "G_AC_DITHER", G_AC_DITHER },
};

const std::unordered_map<std::string, uint32_t> alphaDitherModes = {
    { "G_AD_PATTERN", G_AD_PATTERN },
    { "G_AD_NOTPATTERN", G_AD_NOTPATTERN },
    { "G_AD_NOISE", G_AD_NOISE },
    { "G_AD_DISABLE", G_AD_DISABLE },
};

const std::unordered_map<std::string, uint32_t> colorDitherModes = {
    { "G_CD_MAGICSQ", G_CD_MAGICSQ }, { "G_CD_BAYER", G_CD_BAYER },     { "G_CD_NOISE", G_CD_NOISE },
    { "G_CD_ENABLE", G_CD_ENABLE },   { "G_CD_DISABLE", G_CD_DISABLE },
};

const std::unordered_map<std::string, uint32_t> combineKeyModes = {
    { "G_CK_NONE", G_CK_NONE },
    { "G_CK_KEY", G_CK_KEY },
};

const std::unordered_map<std::string, uint32_t> textureFilterModes = {
    { "G_TF_POINT", G_TF_POINT },
    { "G_TF_AVERAGE", G_TF_AVERAGE },
    { "G_TF_BILERP", G_TF_BILERP },
};

const std::unordered_map<std::string, uint32_t> textureLodModes = {
    { "G_TL_TILE", G_TL_TILE },
    { "G_TL_LOD", G_TL_LOD },
};

const std::unordered_map<std::string, uint32_t> textureDetailModes = {
    { "G_TD_CLAMP", G_TD_CLAMP },
    { "G_TD_SHARPEN", G_TD_SHARPEN },
    { "G_TD_DETAIL", G_TD_DETAIL },
};

const std::unordered_map<std::string, uint32_t> texturePerspModes = {
    { "G_TP_NONE", G_TP_NONE },
    { "G_TP_PERSP", G_TP_PERSP },
};

const std::unordered_map<std::string, uint32_t> scissorModes = {
    { "G_SC_NON_INTERLACE", G_SC_NON_INTERLACE },
    { "G_SC_ODD_INTERLACE", G_SC_ODD_INTERLACE },
    { "G_SC_EVEN_INTERLACE", G_SC_EVEN_INTERLACE },
};

const std::unordered_map<std::string, uint32_t> objectRenderModes = {
    { "G_OBJRM_NOTXCLAMP", G_OBJRM_NOTXCLAMP },
    { "G_OBJRM_XLU", G_OBJRM_XLU },
    { "G_OBJRM_ANTIALIAS", G_OBJRM_ANTIALIAS },
    { "G_OBJRM_BILERP", G_OBJRM_BILERP },
    { "G_OBJRM_SHRINKSIZE_1", G_OBJRM_SHRINKSIZE_1 },
    { "G_OBJRM_SHRINKSIZE_2", G_OBJRM_SHRINKSIZE_2 },
    { "G_OBJRM_WIDEN", G_OBJRM_WIDEN },
};

struct CombineMode {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t aa;
    uint32_t ab;
    uint32_t ac;
    uint32_t ad;
};

#define COMBINE_MODE_VALUES_IMPL(a, b, c, d, aa, ab, ac, ad) \
    { G_CCMUX_##a, G_CCMUX_##b, G_CCMUX_##c, G_CCMUX_##d, G_ACMUX_##aa, G_ACMUX_##ab, G_ACMUX_##ac, G_ACMUX_##ad }
#define CALL_COMBINE_MODE_VALUES(args) COMBINE_MODE_VALUES_IMPL args
#define COMBINE_MODE_VALUES(mode) CALL_COMBINE_MODE_VALUES((mode))

const std::unordered_map<std::string, CombineMode> combineModes = {
    { "G_CC_PRIMITIVE", COMBINE_MODE_VALUES(G_CC_PRIMITIVE) },
    { "G_CC_SHADE", COMBINE_MODE_VALUES(G_CC_SHADE) },
    { "G_CC_MODULATEI", COMBINE_MODE_VALUES(G_CC_MODULATEI) },
    { "G_CC_MODULATEIA", COMBINE_MODE_VALUES(G_CC_MODULATEIA) },
    { "G_CC_MODULATEIDECALA", COMBINE_MODE_VALUES(G_CC_MODULATEIDECALA) },
    { "G_CC_MODULATERGB", COMBINE_MODE_VALUES(G_CC_MODULATERGB) },
    { "G_CC_MODULATERGBA", COMBINE_MODE_VALUES(G_CC_MODULATERGBA) },
    { "G_CC_MODULATERGBDECALA", COMBINE_MODE_VALUES(G_CC_MODULATERGBDECALA) },
    { "G_CC_MODULATEI_PRIM", COMBINE_MODE_VALUES(G_CC_MODULATEI_PRIM) },
    { "G_CC_MODULATEIA_PRIM", COMBINE_MODE_VALUES(G_CC_MODULATEIA_PRIM) },
    { "G_CC_MODULATEIDECALA_PRIM", COMBINE_MODE_VALUES(G_CC_MODULATEIDECALA_PRIM) },
    { "G_CC_MODULATERGB_PRIM", COMBINE_MODE_VALUES(G_CC_MODULATERGB_PRIM) },
    { "G_CC_MODULATERGBA_PRIM", COMBINE_MODE_VALUES(G_CC_MODULATERGBA_PRIM) },
    { "G_CC_MODULATERGBDECALA_PRIM", COMBINE_MODE_VALUES(G_CC_MODULATERGBDECALA_PRIM) },
    { "G_CC_DECALRGB", COMBINE_MODE_VALUES(G_CC_DECALRGB) },
    { "G_CC_DECALRGBA", COMBINE_MODE_VALUES(G_CC_DECALRGBA) },
    { "G_CC_BLENDI", COMBINE_MODE_VALUES(G_CC_BLENDI) },
    { "G_CC_BLENDIA", COMBINE_MODE_VALUES(G_CC_BLENDIA) },
    { "G_CC_BLENDIDECALA", COMBINE_MODE_VALUES(G_CC_BLENDIDECALA) },
    { "G_CC_BLENDRGBA", COMBINE_MODE_VALUES(G_CC_BLENDRGBA) },
    { "G_CC_BLENDRGBDECALA", COMBINE_MODE_VALUES(G_CC_BLENDRGBDECALA) },
    { "G_CC_ADDRGB", COMBINE_MODE_VALUES(G_CC_ADDRGB) },
    { "G_CC_ADDRGBDECALA", COMBINE_MODE_VALUES(G_CC_ADDRGBDECALA) },
    { "G_CC_REFLECTRGB", COMBINE_MODE_VALUES(G_CC_REFLECTRGB) },
    { "G_CC_REFLECTRGBDECALA", COMBINE_MODE_VALUES(G_CC_REFLECTRGBDECALA) },
    { "G_CC_HILITERGB", COMBINE_MODE_VALUES(G_CC_HILITERGB) },
    { "G_CC_HILITERGBA", COMBINE_MODE_VALUES(G_CC_HILITERGBA) },
    { "G_CC_HILITERGBDECALA", COMBINE_MODE_VALUES(G_CC_HILITERGBDECALA) },
    { "G_CC_SHADEDECALA", COMBINE_MODE_VALUES(G_CC_SHADEDECALA) },
    { "G_CC_BLENDPE", COMBINE_MODE_VALUES(G_CC_BLENDPE) },
    { "G_CC_BLENDPEDECALA", COMBINE_MODE_VALUES(G_CC_BLENDPEDECALA) },
    { "_G_CC_BLENDPE", COMBINE_MODE_VALUES(_G_CC_BLENDPE) },
    { "_G_CC_BLENDPEDECALA", COMBINE_MODE_VALUES(_G_CC_BLENDPEDECALA) },
    { "_G_CC_TWOCOLORTEX", COMBINE_MODE_VALUES(_G_CC_TWOCOLORTEX) },
    { "_G_CC_SPARSEST", COMBINE_MODE_VALUES(_G_CC_SPARSEST) },
    { "G_CC_TEMPLERP", COMBINE_MODE_VALUES(G_CC_TEMPLERP) },
    { "G_CC_TRILERP", COMBINE_MODE_VALUES(G_CC_TRILERP) },
    { "G_CC_INTERFERENCE", COMBINE_MODE_VALUES(G_CC_INTERFERENCE) },
    { "G_CC_1CYUV2RGB", COMBINE_MODE_VALUES(G_CC_1CYUV2RGB) },
    { "G_CC_YUV2RGB", COMBINE_MODE_VALUES(G_CC_YUV2RGB) },
    { "G_CC_PASS2", COMBINE_MODE_VALUES(G_CC_PASS2) },
    { "G_CC_MODULATEI2", COMBINE_MODE_VALUES(G_CC_MODULATEI2) },
    { "G_CC_MODULATEIA2", COMBINE_MODE_VALUES(G_CC_MODULATEIA2) },
    { "G_CC_MODULATERGB2", COMBINE_MODE_VALUES(G_CC_MODULATERGB2) },
    { "G_CC_MODULATERGBA2", COMBINE_MODE_VALUES(G_CC_MODULATERGBA2) },
    { "G_CC_MODULATEI_PRIM2", COMBINE_MODE_VALUES(G_CC_MODULATEI_PRIM2) },
    { "G_CC_MODULATEIA_PRIM2", COMBINE_MODE_VALUES(G_CC_MODULATEIA_PRIM2) },
    { "G_CC_MODULATERGB_PRIM2", COMBINE_MODE_VALUES(G_CC_MODULATERGB_PRIM2) },
    { "G_CC_MODULATERGBA_PRIM2", COMBINE_MODE_VALUES(G_CC_MODULATERGBA_PRIM2) },
    { "G_CC_DECALRGB2", COMBINE_MODE_VALUES(G_CC_DECALRGB2) },
    { "G_CC_BLENDI2", COMBINE_MODE_VALUES(G_CC_BLENDI2) },
    { "G_CC_BLENDIA2", COMBINE_MODE_VALUES(G_CC_BLENDIA2) },
    { "G_CC_CHROMA_KEY2", COMBINE_MODE_VALUES(G_CC_CHROMA_KEY2) },
    { "G_CC_HILITERGB2", COMBINE_MODE_VALUES(G_CC_HILITERGB2) },
    { "G_CC_HILITERGBA2", COMBINE_MODE_VALUES(G_CC_HILITERGBA2) },
    { "G_CC_HILITERGBDECALA2", COMBINE_MODE_VALUES(G_CC_HILITERGBDECALA2) },
    { "G_CC_HILITERGBPASSA2", COMBINE_MODE_VALUES(G_CC_HILITERGBPASSA2) },
};

#undef COMBINE_MODE_VALUES
#undef CALL_COMBINE_MODE_VALUES
#undef COMBINE_MODE_VALUES_IMPL

static Gfx GsSpVertexOtR2P1(char* filePathPtr) {
    Gfx g;
    g.words.w0 = G_VTX_OTR_FILEPATH << 24;
    g.words.w1 = (uintptr_t)filePathPtr;

    return g;
}

static Gfx GsSpVertexOtR2P2(int vtxCnt, int vtxBufOffset, int vtxDataOffset) {
    Gfx g;
    g.words.w0 = (uintptr_t)vtxCnt;
    g.words.w1 = (uintptr_t)((vtxBufOffset << 16) | vtxDataOffset);

    return g;
}

static Gfx GsSPPushShader(const char* shader) {
    Gfx g;
    g.words.w0 = (uintptr_t)(_SHIFTL(G_PUSH_SHADER, 24, 8));
    g.words.w1 = (uintptr_t)(shader);
    return g;
}

static Gfx GsSPPopShader() {
    Gfx g;
    g.words.w0 = (uintptr_t)(_SHIFTL(G_POP_SHADER, 24, 8));
    g.words.w1 = (uintptr_t)(nullptr);
    return g;
}

uint32_t ResourceFactoryDisplayList::GetCombineLERPValue(const char* valStr) {
    static const char* strings[] = {
        "G_CCMUX_COMBINED",
        "G_CCMUX_TEXEL0",
        "G_CCMUX_TEXEL1",
        "G_CCMUX_PRIMITIVE",
        "G_CCMUX_SHADE",
        "G_CCMUX_ENVIRONMENT",
        "G_CCMUX_1",
        "G_CCMUX_NOISE",
        "G_CCMUX_0",
        "G_CCMUX_CENTER",
        "G_CCMUX_K4",
        "G_CCMUX_SCALE",
        "G_CCMUX_COMBINED_ALPHA",
        "G_CCMUX_TEXEL0_ALPHA",
        "G_CCMUX_TEXEL1_ALPHA",
        "G_CCMUX_PRIMITIVE_ALPHA",
        "G_CCMUX_SHADE_ALPHA",
        "G_CCMUX_ENV_ALPHA",
        "G_CCMUX_LOD_FRACTION",
        "G_CCMUX_PRIM_LOD_FRAC",
        "G_CCMUX_K5",
        "G_ACMUX_COMBINED",
        "G_ACMUX_TEXEL0",
        "G_ACMUX_TEXEL1",
        "G_ACMUX_PRIMITIVE",
        "G_ACMUX_SHADE",
        "G_ACMUX_ENVIRONMENT",
        "G_ACMUX_1",
        "G_ACMUX_0",
        "G_ACMUX_LOD_FRACTION",
        "G_ACMUX_PRIM_LOD_FRAC",
    };
    static uint32_t values[] = {
        G_CCMUX_COMBINED,
        G_CCMUX_TEXEL0,
        G_CCMUX_TEXEL1,
        G_CCMUX_PRIMITIVE,
        G_CCMUX_SHADE,
        G_CCMUX_ENVIRONMENT,
        G_CCMUX_1,
        G_CCMUX_NOISE,
        G_CCMUX_0,
        G_CCMUX_CENTER,
        G_CCMUX_K4,
        G_CCMUX_SCALE,
        G_CCMUX_COMBINED_ALPHA,
        G_CCMUX_TEXEL0_ALPHA,
        G_CCMUX_TEXEL1_ALPHA,
        G_CCMUX_PRIMITIVE_ALPHA,
        G_CCMUX_SHADE_ALPHA,
        G_CCMUX_ENV_ALPHA,
        G_CCMUX_LOD_FRACTION,
        G_CCMUX_PRIM_LOD_FRAC,
        G_CCMUX_K5,
        G_ACMUX_COMBINED,
        G_ACMUX_TEXEL0,
        G_ACMUX_TEXEL1,
        G_ACMUX_PRIMITIVE,
        G_ACMUX_SHADE,
        G_ACMUX_ENVIRONMENT,
        G_ACMUX_1,
        G_ACMUX_0,
        G_ACMUX_LOD_FRACTION,
        G_ACMUX_PRIM_LOD_FRAC,
    };

    for (size_t i = 0; i < std::size(values); i++) {
        if (strncmp(valStr, strings[i], strlen(strings[i])) == 0) {
            return values[i];
        }
    }

    return G_CCMUX_1;
}

int8_t GetEndOpcodeByUCode(UcodeHandlers ucode) {
    switch (ucode) {
        case ucode_f3d:
        case ucode_f3db:
        case ucode_f3dex:
        case ucode_f3dexb:
            return F3DEX_G_ENDDL;
        case ucode_f3dex2:
        case ucode_s2dex: {
            return F3DEX2_G_ENDDL;
        }
        case ucode_max:
            break;
    }
    return -1;
}

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryDisplayListV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                 std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto displayList = std::make_shared<DisplayList>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);
    auto ucode = (UcodeHandlers)reader->ReadInt8();

    displayList->UCode = ucode;

    while (reader->GetBaseAddress() % 8 != 0) {
        reader->ReadInt8();
    }

    size_t idx = 0;
    while (true) {
        Gfx command;
        command.words.w0 = reader->ReadUInt32();
        command.words.w1 = reader->ReadUInt32();

        int8_t opcode = (int8_t)(command.words.w0 >> 24);
        bool isExpanded = opcode == G_SETTIMG_OTR_HASH || opcode == G_DL_OTR_HASH || opcode == G_VTX_OTR_HASH ||
                          opcode == G_BRANCH_Z_OTR || opcode == G_MARKER || opcode == G_MTX_OTR ||
                          opcode == G_MOVEMEM_OTR;

        // These are 128-bit commands, so read an extra 64 bits...
        if (isExpanded) {
#ifdef USE_GBI_TRACE
            command.words.trace.file = initData->Path.c_str();
            command.words.trace.idx = idx++;
            command.words.trace.valid = true;
#endif
            displayList->Instructions.push_back(command);
            command.words.w0 = reader->ReadUInt32();
            command.words.w1 = reader->ReadUInt32();
        }

#ifdef USE_GBI_TRACE
        command.words.trace.file = initData->Path.c_str();
        command.words.trace.idx = idx++;
        command.words.trace.valid = true;
#endif

        displayList->Instructions.push_back(command);

        if (opcode == GetEndOpcodeByUCode(ucode)) {
            break;
        }
    }

    return displayList;
}

std::shared_ptr<Ship::IResource>
ResourceFactoryXMLDisplayListV0::ReadResource(std::shared_ptr<Ship::File> file,
                                              std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto dl = std::make_shared<DisplayList>(initData);
    auto child =
        std::get<std::shared_ptr<tinyxml2::XMLDocument>>(file->Reader)->FirstChildElement()->FirstChildElement();

    while (child != nullptr) {
        std::string childName = child->Name();

        Gfx g = gsDPPipeSync();

        if (childName == "PipeSync") {
            g = gsDPPipeSync();
        } else if (childName == "FullSync") {
            g = gsDPFullSync();
        } else if (childName == "NoOp") {
            g = gsDPNoOpTag(child->UnsignedAttribute("Tag"));
        } else if (childName == "SPNoOp") {
            g = gsSPNoOp();
        } else if (childName == "Texture") {
            g = gsSPTexture(child->IntAttribute("S"), child->IntAttribute("T"), child->IntAttribute("Level"),
                            child->IntAttribute("Tile"), child->IntAttribute("On"));
        } else if (childName == "TextureL") {
            g = gsSPTextureL(child->IntAttribute("S"), child->IntAttribute("T"), child->IntAttribute("Level"),
                             child->IntAttribute("XParam"), child->IntAttribute("Tile"), child->IntAttribute("On"));
        } else if (childName == "SetPrimColor") {
            g = gsDPSetPrimColor(child->IntAttribute("M"), child->IntAttribute("L"), child->IntAttribute("R"),
                                 child->IntAttribute("G"), child->IntAttribute("B"), child->IntAttribute("A"));
        } else if (childName == "SetPrimDepth") {
            g = gsDPSetPrimDepth(child->IntAttribute("Z"), child->IntAttribute("DZ"));
        } else if (childName == "SetColorImage") {
            std::string fmt = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            std::string siz = child->Attribute("Size") != nullptr ? child->Attribute("Size") : "0";
            auto fmtEntry = imageFormats.find(fmt);
            auto sizEntry = imageSizes.find(siz);
            uint32_t fmtVal = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmt, nullptr, 0);
            uint32_t sizVal = sizEntry != imageSizes.end() ? sizEntry->second : std::stoul(siz, nullptr, 0);
            g = gsDPSetColorImage(fmtVal, sizVal, child->UnsignedAttribute("Width"),
                                  child->UnsignedAttribute("Address"));
        } else if (childName == "SetDepthImage" || childName == "SetMaskImage") {
            g = gsDPSetDepthImage(child->UnsignedAttribute("Address"));
        } else if (childName == "SetFillColor") {
            g = gsDPSetFillColor(child->IntAttribute("C"));
        } else if (childName == "SetFogColor") {
            g = gsDPSetFogColor(child->IntAttribute("R"), child->IntAttribute("G"), child->IntAttribute("B"),
                                child->IntAttribute("A"));
        } else if (childName == "SetBlendColor") {
            g = gsDPSetBlendColor(child->IntAttribute("R"), child->IntAttribute("G"), child->IntAttribute("B"),
                                  child->IntAttribute("A"));
        } else if (childName == "SetEnvColor") {
            g = gsDPSetEnvColor(child->IntAttribute("R"), child->IntAttribute("G"), child->IntAttribute("B"),
                                child->IntAttribute("A"));
        } else if (childName == "Grayscale") {
            g = gsSPGrayscale(child->BoolAttribute("Enabled"));
        } else if (childName == "SetGrayscaleColor") {
            g = gsDPSetGrayscaleColor(child->IntAttribute("R"), child->IntAttribute("G"), child->IntAttribute("B"),
                                      child->IntAttribute("A"));
        } else if (childName == "SetDepthSource") {
            std::string value = child->Attribute("Mode") != nullptr ? child->Attribute("Mode") : "0";
            auto entry = depthSourceModes.find(value);
            g = gsDPSetDepthSource(entry != depthSourceModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "SetAlphaCompare") {
            std::string value = child->Attribute("Mode") != nullptr ? child->Attribute("Mode") : "0";
            auto entry = alphaCompareModes.find(value);
            g = gsDPSetAlphaCompare(entry != alphaCompareModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "SetAlphaDither") {
            std::string value = child->Attribute("Type") != nullptr ? child->Attribute("Type") : "0";
            auto entry = alphaDitherModes.find(value);
            g = gsDPSetAlphaDither(entry != alphaDitherModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "SetColorDither") {
            std::string value = child->Attribute("Type") != nullptr ? child->Attribute("Type") : "0";
            auto entry = colorDitherModes.find(value);
            g = gsDPSetColorDither(entry != colorDitherModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "SetCombineKey") {
            std::string value = child->Attribute("Type") != nullptr ? child->Attribute("Type") : "0";
            auto entry = combineKeyModes.find(value);
            g = gsDPSetCombineKey(entry != combineKeyModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "SetTextureFilter") {
            std::string value = child->Attribute("Mode") != nullptr ? child->Attribute("Mode") : "0";
            auto entry = textureFilterModes.find(value);
            g = gsDPSetTextureFilter(entry != textureFilterModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "SetTextureLOD") {
            std::string value = child->Attribute("Mode") != nullptr ? child->Attribute("Mode") : "0";
            auto entry = textureLodModes.find(value);
            g = gsDPSetTextureLOD(entry != textureLodModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "SetTextureDetail") {
            std::string value = child->Attribute("Type") != nullptr ? child->Attribute("Type") : "0";
            auto entry = textureDetailModes.find(value);
            g = gsDPSetTextureDetail(entry != textureDetailModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "SetTexturePersp") {
            std::string value = child->Attribute("Enable") != nullptr ? child->Attribute("Enable") : "0";
            auto entry = texturePerspModes.find(value);
            g = gsDPSetTexturePersp(entry != texturePerspModes.end() ? entry->second : std::stoul(value, nullptr, 0));
        } else if (childName == "PerspNormalize") {
            g = gsSPPerspNormalize(child->IntAttribute("S"));
        } else if (childName == "FogPosition") {
            g = gsSPFogPosition(child->IntAttribute("Min"), child->IntAttribute("Max"));
        } else if (childName == "FogFactor") {
            g = gsSPFogFactor(child->IntAttribute("FM"), child->IntAttribute("FO"));
        } else if (childName == "NumLites") {
            g = gsSPNumLights(child->IntAttribute("Lites"));
        } else if (childName == "Segment") {
            g = gsSPSegment(child->IntAttribute("Seg"), child->IntAttribute("Base"));
        } else if (childName == "Line3D") {
            g = gsSPLineW3D(child->IntAttribute("V0"), child->IntAttribute("V1"), child->IntAttribute("Width"),
                            child->IntAttribute("Flag"));
            /*else if (childName == "Hilite2Tile")
            {
                    g = gsDPSetHilite2Tile(child->IntAttribute("Tile"), child->IntAttribute("Hilite"),
            child->IntAttribute("Width"), child->IntAttribute("Height"));
            }*/
        } else if (childName == "Viewport") {
            g = gsSPViewport(static_cast<uintptr_t>(child->Unsigned64Attribute("Address")));
        } else if (childName == "Light") {
            g = gsSPLight(static_cast<uintptr_t>(child->Unsigned64Attribute("Address")),
                          child->UnsignedAttribute("Number"));
        } else if (childName == "SetLights0" || childName == "SetLights1" || childName == "SetLights2" ||
                   childName == "SetLights3" || childName == "SetLights4" || childName == "SetLights5" ||
                   childName == "SetLights6" || childName == "SetLights7") {
            const uint32_t lightCount = static_cast<uint32_t>(childName.back() - '0');
            const uint32_t directionalCount = lightCount == 0 ? 1 : lightCount;
            const uintptr_t address = static_cast<uintptr_t>(child->Unsigned64Attribute("Address"));

            dl->Instructions.push_back(gsSPNumLights(directionalCount));
            for (uint32_t i = 0; i < directionalCount; i++) {
                dl->Instructions.push_back(gsSPLight(address + sizeof(Ambient) + i * sizeof(Light), i + 1));
            }
            g = gsSPLight(address, directionalCount + 1);
        } else if (childName == "LookAtX") {
            g = gsSPLookAtX(static_cast<uintptr_t>(child->Unsigned64Attribute("Address")));
        } else if (childName == "LookAtY") {
            g = gsSPLookAtY(static_cast<uintptr_t>(child->Unsigned64Attribute("Address")));
        } else if (childName == "LookAt") {
            uintptr_t address = child->Unsigned64Attribute("Address");
            Gfx g2[2] = { gsSPLookAt(address) };
            dl->Instructions.push_back(g2[0]);
            g = g2[1];
        } else if (childName == "SetHilite1Tile" || childName == "SetHilite2Tile") {
            const int32_t x = child->IntAttribute("X");
            const int32_t y = child->IntAttribute("Y");
            const int32_t width = child->IntAttribute("Width");
            const int32_t height = child->IntAttribute("Height");
            g = gsDPSetTileSize(child->UnsignedAttribute("Tile"), x & 0xFFF, y & 0xFFF, (((width - 1) * 4) + x) & 0xFFF,
                                (((height - 1) * 4) + y) & 0xFFF);
        } else if (childName == "ForceMatrix") {
            uintptr_t address = child->Unsigned64Attribute("Address");
#ifdef F3DEX_GBI_2
            Gfx g2[2] = { gsSPForceMatrix(address) };
            dl->Instructions.push_back(g2[0]);
            g = g2[1];
#else
            Gfx g2[4] = { gsSPForceMatrix(address) };
            dl->Instructions.push_back(g2[0]);
            dl->Instructions.push_back(g2[1]);
            dl->Instructions.push_back(g2[2]);
            g = g2[3];
#endif
        } else if (childName == "MoveWord") {
            g = gsMoveWd(child->UnsignedAttribute("Index"), child->UnsignedAttribute("Offset"),
                         child->UnsignedAttribute("Data"));
        } else if (childName == "MoveMem") {
            uintptr_t address = child->Unsigned64Attribute("Address");
#ifdef F3DEX_GBI_2
            g = gsDma2p(G_MOVEMEM, address, child->UnsignedAttribute("Length"), child->UnsignedAttribute("Index"),
                        child->UnsignedAttribute("Offset"));
#else
            g = gsDma1p(G_MOVEMEM, address, child->UnsignedAttribute("Length"), child->UnsignedAttribute("Index"));
#endif
        } else if (childName == "Matrix") {
            std::string fName = child->Attribute("Path");
            std::string param = child->Attribute("Param");

            uint8_t paramInt = 0;

            if (param == "G_MTX_PUSH") {
                paramInt = G_MTX_PUSH;
            } else if (param == "G_MTX_NOPUSH") {
                paramInt = G_MTX_NOPUSH;
            } else if (param == "G_MTX_LOAD") {
                paramInt = G_MTX_LOAD;
            } else if (param == "G_MTX_MUL") {
                paramInt = G_MTX_MUL;
            } else if (param == "G_MTX_MODELVIEW") {
                paramInt = G_MTX_MODELVIEW;
            } else if (param == "G_MTX_PROJECTION") {
                paramInt = G_MTX_PROJECTION;
            }

            if (fName[0] == '>' && fName[1] == '0' && fName[2] == 'x') {
                int offset = strtol(fName.c_str() + 1, NULL, 16);
                g = gsSPMatrix(offset | 1, paramInt);
            } else {
                g = { gsSPMatrix(0, paramInt) };

                g.words.w0 &= 0x00FFFFFF;
                g.words.w0 += (G_MTX_OTR_FILEPATH << 24);
                char* str = (char*)malloc(fName.size() + 1);
                g.words.w1 = (uintptr_t)str;
                dl->Strings.push_back(str);
                strcpy((char*)g.words.w1, fName.data());
            }
        } else if (childName == "PopMatrix") {
            std::string param = child->Attribute("Param");

            uint8_t paramInt = 0;

            if (param == "G_MTX_MODELVIEW") {
                paramInt = G_MTX_MODELVIEW;
            } else if (param == "G_MTX_PROJECTION") {
                paramInt = G_MTX_PROJECTION;
            }

            g = gsSPPopMatrix(paramInt);
        } else if (childName == "PopMatrixN") {
            std::string param = child->Attribute("Param");
            uint8_t paramInt = param == "G_MTX_PROJECTION" ? G_MTX_PROJECTION : G_MTX_MODELVIEW;
            uint32_t count = child->UnsignedAttribute("Count");
#ifdef F3DEX_GBI_2
            g = gsSPPopMatrixN(paramInt, count);
#else
            for (uint32_t i = 1; i < count; i++) {
                dl->Instructions.push_back(gsSPPopMatrix(paramInt));
            }
            g = gsSPPopMatrix(paramInt);
#endif
        } else if (childName == "SetCycleType") {
            uint32_t param = 0;

            if (child->Attribute("G_CYC_1CYCLE", 0)) {
                param |= G_CYC_1CYCLE;
            }

            if (child->Attribute("G_CYC_2CYCLE", 0)) {
                param |= G_CYC_2CYCLE;
            }

            if (child->Attribute("G_CYC_COPY", 0)) {
                param |= G_CYC_COPY;
            }

            if (child->Attribute("G_CYC_FILL", 0)) {
                param |= G_CYC_FILL;
            }

            g = gsDPSetCycleType(param);
        } else if (childName == "PipelineMode") {
            uint32_t param = 0;

            if (child->Attribute("G_PM_1PRIMITIVE", 0)) {
                param |= G_PM_1PRIMITIVE;
            }

            if (child->Attribute("G_PM_NPRIMITIVE", 0)) {
                param |= G_PM_NPRIMITIVE;
            }

            g = gsDPPipelineMode(param);
        } else if (childName == "TileSync") {
            g = gsDPTileSync();
        } else if (childName == "LoadTile") {
            uint32_t t = child->IntAttribute("T");
            uint32_t uls = child->IntAttribute("Uls");
            uint32_t ult = child->IntAttribute("Ult");
            uint32_t lrs = child->IntAttribute("Lrs");
            uint32_t lrt = child->IntAttribute("Lrt");

            g = gsDPLoadTile(t, uls, ult, lrs, lrt);
        } else if (childName == "FillRectangle") {
            g = gsDPFillRectangle(child->UnsignedAttribute("Ulx"), child->UnsignedAttribute("Uly"),
                                  child->UnsignedAttribute("Lrx"), child->UnsignedAttribute("Lry"));
        } else if (childName == "SetScissor") {
            std::string value = child->Attribute("Mode") != nullptr ? child->Attribute("Mode") : "0";
            auto entry = scissorModes.find(value);
            uint32_t mode = entry != scissorModes.end() ? entry->second : std::stoul(value, nullptr, 0);
            g = gsDPSetScissor(mode, child->UnsignedAttribute("Ulx"), child->UnsignedAttribute("Uly"),
                               child->UnsignedAttribute("Lrx"), child->UnsignedAttribute("Lry"));
        } else if (childName == "SetScissorFrac") {
            std::string value = child->Attribute("Mode") != nullptr ? child->Attribute("Mode") : "0";
            auto entry = scissorModes.find(value);
            uint32_t mode = entry != scissorModes.end() ? entry->second : std::stoul(value, nullptr, 0);
            g = gsDPSetScissorFrac(mode, child->UnsignedAttribute("Ulx"), child->UnsignedAttribute("Uly"),
                                   child->UnsignedAttribute("Lrx"), child->UnsignedAttribute("Lry"));
        } else if (childName == "SetConvert") {
            g = gsDPSetConvert(child->IntAttribute("K0"), child->IntAttribute("K1"), child->IntAttribute("K2"),
                               child->IntAttribute("K3"), child->IntAttribute("K4"), child->IntAttribute("K5"));
        } else if (childName == "SetKeyR") {
            g = gsDPSetKeyR(child->UnsignedAttribute("Center"), child->UnsignedAttribute("Scale"),
                            child->UnsignedAttribute("Width"));
        } else if (childName == "SetKeyGB") {
            g = gsDPSetKeyGB(child->UnsignedAttribute("CenterG"), child->UnsignedAttribute("ScaleG"),
                             child->UnsignedAttribute("WidthG"), child->UnsignedAttribute("CenterB"),
                             child->UnsignedAttribute("ScaleB"), child->UnsignedAttribute("WidthB"));
        } else if (childName == "SetTextureLUT") {
            std::string mode = child->Attribute("Mode");
            uint32_t modeVal = 0;

            if (mode == "G_TT_NONE") {
                modeVal = G_TT_NONE;
            } else if (mode == "G_TT_RGBA16") {
                modeVal = G_TT_RGBA16;
            } else if (mode == "G_TT_IA16") {
                modeVal = G_TT_IA16;
            }

            g = gsDPSetTextureLUT(modeVal);
        } else if (childName == "LoadTLUTCmd") {
            uint32_t tile = child->IntAttribute("Tile");
            uint32_t count = child->IntAttribute("Count");

            g = gsDPLoadTLUTCmd(tile, count);
        } else if (childName == "LoadTLUT" || childName == "LoadTLUTPal16" || childName == "LoadTLUTPal128" ||
                   childName == "LoadTLUTPal256") {
            Gfx g2[6];
            if (childName == "LoadTLUT") {
                Gfx g3[6] = { gsDPLoadTLUT(child->UnsignedAttribute("Count"), child->UnsignedAttribute("TMem"), 0) };
                memcpy(g2, g3, sizeof(g2));
            } else if (childName == "LoadTLUTPal16") {
                Gfx g3[6] = { gsDPLoadTLUT_pal16(child->UnsignedAttribute("Palette"), 0) };
                memcpy(g2, g3, sizeof(g2));
            } else if (childName == "LoadTLUTPal128") {
                Gfx g3[6] = { gsDPLoadTLUT(128, 256 + ((child->UnsignedAttribute("Palette") & 1) * 128), 0) };
                memcpy(g2, g3, sizeof(g2));
            } else {
                Gfx g3[6] = { gsDPLoadTLUT_pal256(0) };
                memcpy(g2, g3, sizeof(g2));
            }

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            dl->Strings.push_back(str);
            strcpy(str, path.c_str());
            g2[0].words.w0 = (g2[0].words.w0 & 0x00FFFFFF) | _SHIFTL(G_SETTIMG_OTR_FILEPATH, 24, 8);
            g2[0].words.w1 = reinterpret_cast<uintptr_t>(str);

            for (int i = 0; i < 5; i++) {
                dl->Instructions.push_back(g2[i]);
            }
            g = g2[5];
        } else if (childName == "SetCombineMode") {
            const CombineMode& mode1 = combineModes.at(child->Attribute("Mode1"));
            const CombineMode& mode2 = combineModes.at(child->Attribute("Mode2"));
            g = gsDPSetCombineLERP_NoMacros(mode1.a, mode1.b, mode1.c, mode1.d, mode1.aa, mode1.ab, mode1.ac, mode1.ad,
                                            mode2.a, mode2.b, mode2.c, mode2.d, mode2.aa, mode2.ab, mode2.ac, mode2.ad);
        } else if (childName == "SetCombineLERP") {
            const char* a0 = child->Attribute("A0", 0);
            const char* b0 = child->Attribute("B0", 0);
            const char* c0 = child->Attribute("C0", 0);
            const char* d0 = child->Attribute("D0", 0);

            const char* aa0 = child->Attribute("Aa0", 0);
            const char* ab0 = child->Attribute("Ab0", 0);
            const char* ac0 = child->Attribute("Ac0", 0);
            const char* ad0 = child->Attribute("Ad0", 0);

            const char* a1 = child->Attribute("A1", 0);
            const char* b1 = child->Attribute("B1", 0);
            const char* c1 = child->Attribute("C1", 0);
            const char* d1 = child->Attribute("D1", 0);

            const char* aa1 = child->Attribute("Aa1", 0);
            const char* ab1 = child->Attribute("Ab1", 0);
            const char* ac1 = child->Attribute("Ac1", 0);
            const char* ad1 = child->Attribute("Ad1", 0);

            g = gsDPSetCombineLERP_NoMacros(
                GetCombineLERPValue(a0), GetCombineLERPValue(b0), GetCombineLERPValue(c0), GetCombineLERPValue(d0),
                GetCombineLERPValue(aa0), GetCombineLERPValue(ab0), GetCombineLERPValue(ac0), GetCombineLERPValue(ad0),
                GetCombineLERPValue(a1), GetCombineLERPValue(b1), GetCombineLERPValue(c1), GetCombineLERPValue(d1),
                GetCombineLERPValue(aa1), GetCombineLERPValue(ab1), GetCombineLERPValue(ac1), GetCombineLERPValue(ad1));
        } else if (childName == "LoadSync") {
            g = gsDPLoadSync();
        } else if (childName == "TextureRectangle" || childName == "TextureRectangleFlip") {
            Gfx g2[3];
            if (childName == "TextureRectangle") {
                Gfx g3[3] = { gsSPTextureRectangle(
                    child->IntAttribute("Xl"), child->IntAttribute("Yl"), child->IntAttribute("Xh"),
                    child->IntAttribute("Yh"), child->IntAttribute("Tile"), child->IntAttribute("S"),
                    child->IntAttribute("T"), child->IntAttribute("Dsdx"), child->IntAttribute("Dtdy")) };
                memcpy(g2, g3, sizeof(g2));
            } else {
                Gfx g3[3] = { gsSPTextureRectangleFlip(
                    child->IntAttribute("Xl"), child->IntAttribute("Yl"), child->IntAttribute("Xh"),
                    child->IntAttribute("Yh"), child->IntAttribute("Tile"), child->IntAttribute("S"),
                    child->IntAttribute("T"), child->IntAttribute("Dsdx"), child->IntAttribute("Dtdy")) };
                memcpy(g2, g3, sizeof(g2));
            }
            dl->Instructions.push_back(g2[0]);
            dl->Instructions.push_back(g2[1]);
            g = g2[2];
        } else if (childName == "LoadBlock") {
            uint32_t tile = child->IntAttribute("Tile");
            uint32_t uls = child->IntAttribute("Uls");
            uint32_t ult = child->IntAttribute("Ult");
            uint32_t lrs = child->IntAttribute("Lrs");
            uint32_t dxt = child->IntAttribute("Dxt");

            g = gsDPLoadBlock(tile, uls, ult, lrs, dxt);
        } else if (childName == "LoadBlockWide") {
            uint32_t tile = child->IntAttribute("Tile");
            uint32_t uls = child->IntAttribute("Uls");
            uint32_t ult = child->IntAttribute("Ult");
            uint32_t lrs = child->IntAttribute("Lrs");
            uint32_t dxt = child->IntAttribute("Dxt");

            Gfx g2[2] = { gsDPLoadBlockWide(tile, uls, ult, lrs, dxt) };
            dl->Instructions.push_back(g2[0]);
            g = g2[1];
        } else if (childName == "Triangle1") {
            const uint32_t vertices[3] = { child->UnsignedAttribute("V00"), child->UnsignedAttribute("V01"),
                                           child->UnsignedAttribute("V02") };
            const uint32_t flag = child->UnsignedAttribute("Flag0");
            const uint32_t first = flag == 0 ? 0 : flag == 1 ? 1 : 2;
            g.words.w0 = _SHIFTL(G_TRI1_OTR, 24, 8) | vertices[first];
            g.words.w1 = _SHIFTL(vertices[(first + 1) % 3], 16, 16) | _SHIFTL(vertices[(first + 2) % 3], 0, 16);
        } else if (childName == "Triangles2") {
#ifdef F3DEX_GBI_2
            g = gsSP2Triangles(child->IntAttribute("V00"), child->IntAttribute("V01"), child->IntAttribute("V02"),
                               child->IntAttribute("Flag0"), child->IntAttribute("V10"), child->IntAttribute("V11"),
                               child->IntAttribute("V12"), child->IntAttribute("Flag1"));
#else
            const uint32_t vertices0[3] = { child->UnsignedAttribute("V00"), child->UnsignedAttribute("V01"),
                                            child->UnsignedAttribute("V02") };
            const uint32_t flag0 = child->UnsignedAttribute("Flag0");
            const uint32_t first0 = flag0 == 0 ? 0 : flag0 == 1 ? 1 : 2;
            g.words.w0 = _SHIFTL(G_TRI1_OTR, 24, 8) | vertices0[first0];
            g.words.w1 = _SHIFTL(vertices0[(first0 + 1) % 3], 16, 16) | _SHIFTL(vertices0[(first0 + 2) % 3], 0, 16);
            dl->Instructions.push_back(g);
            const uint32_t vertices1[3] = { child->UnsignedAttribute("V10"), child->UnsignedAttribute("V11"),
                                            child->UnsignedAttribute("V12") };
            const uint32_t flag1 = child->UnsignedAttribute("Flag1");
            const uint32_t first1 = flag1 == 0 ? 0 : flag1 == 1 ? 1 : 2;
            g.words.w0 = _SHIFTL(G_TRI1_OTR, 24, 8) | vertices1[first1];
            g.words.w1 = _SHIFTL(vertices1[(first1 + 1) % 3], 16, 16) | _SHIFTL(vertices1[(first1 + 2) % 3], 0, 16);
#endif
        } else if (childName == "Quadrangle") {
            const uint32_t vertices[4] = { child->UnsignedAttribute("V0"), child->UnsignedAttribute("V1"),
                                           child->UnsignedAttribute("V2"), child->UnsignedAttribute("V3") };
            const uint32_t flag = child->UnsignedAttribute("Flag");
            const uint32_t first = flag < 3 ? flag : 3;
            Gfx firstTriangle;
            firstTriangle.words.w0 = _SHIFTL(G_TRI1_OTR, 24, 8) | vertices[first];
            firstTriangle.words.w1 =
                _SHIFTL(vertices[(first + 1) & 3], 16, 16) | _SHIFTL(vertices[(first + 2) & 3], 0, 16);
            dl->Instructions.push_back(firstTriangle);
            g.words.w0 = _SHIFTL(G_TRI1_OTR, 24, 8) | vertices[first];
            g.words.w1 = _SHIFTL(vertices[(first + 2) & 3], 16, 16) | _SHIFTL(vertices[(first + 3) & 3], 0, 16);
        } else if (childName == "ModifyVertex") {
            std::string where = child->Attribute("Where");
            uint32_t whereValue = 0;

            if (where == "G_MWO_POINT_RGBA") {
                whereValue = G_MWO_POINT_RGBA;
            } else if (where == "G_MWO_POINT_ST") {
                whereValue = G_MWO_POINT_ST;
            } else if (where == "G_MWO_POINT_XYSCREEN") {
                whereValue = G_MWO_POINT_XYSCREEN;
            } else if (where == "G_MWO_POINT_ZSCREEN") {
                whereValue = G_MWO_POINT_ZSCREEN;
            } else {
                whereValue = std::stoul(where, nullptr, 0);
            }

            uint32_t value = std::stoul(child->Attribute("Value"), nullptr, 0);
            g = gsSPModifyVertex(child->IntAttribute("Vertex"), whereValue, value);
        } else if (childName == "LoadVertices") {
            std::string fName = child->Attribute("Path");
            // fName = ">" + fName;

            char* str = (char*)malloc(fName.size() + 1);
            dl->Strings.push_back(str);
            strcpy((char*)str, fName.data());

            g = GsSpVertexOtR2P1(str);

            dl->Instructions.push_back(g);

            g = GsSpVertexOtR2P2(child->IntAttribute("Count"), child->IntAttribute("VertexBufferIndex"),
                                 child->IntAttribute("VertexOffset"));
        } else if (childName == "SetTextureImage") {
            std::string fName = child->Attribute("Path");
            // fName = ">" + fName;
            std::string fmt = child->Attribute("Format");
            uint32_t fmtVal = G_IM_FMT_RGBA;

            if (fmt == "G_IM_FMT_I") {
                fmtVal = G_IM_FMT_I;
            } else if (fmt == "G_IM_FMT_IA") {
                fmtVal = G_IM_FMT_IA;
            } else if (fmt == "G_IM_FMT_CI") {
                fmtVal = G_IM_FMT_CI;
            } else if (fmt == "G_IM_FMT_YUV") {
                fmtVal = G_IM_FMT_YUV;
            } else if (fmt == "G_IM_FMT_RGBA") {
                fmtVal = G_IM_FMT_RGBA;
            }

            std::string siz = child->Attribute("Size");
            uint32_t sizVal = G_IM_SIZ_32b;

            if (siz == "G_IM_SIZ_8b_LOAD_BLOCK") {
                sizVal = G_IM_SIZ_8b_LOAD_BLOCK;
            } else if (siz == "G_IM_SIZ_4b") {
                sizVal = G_IM_SIZ_4b;
            } else if (siz == "G_IM_SIZ_8b") {
                sizVal = G_IM_SIZ_8b;
            } else if (siz == "G_IM_SIZ_16b" || siz == "G_IM_SIZ_16b_LOAD_BLOCK") {
                sizVal = G_IM_SIZ_16b;
            } else if (siz == "G_IM_SIZ_32b") {
                sizVal = G_IM_SIZ_32b;
            } else if (siz == "G_IM_SIZ_DD") {
                sizVal = G_IM_SIZ_DD;
            } else {
                int bp = 0;
            }

            uint32_t width = child->IntAttribute("Width");

            if (fName[0] == '>' && fName[1] == '0' && (fName[2] == 'x' || fName[2] == 'X')) {
                uint32_t seg = std::stoul(fName.substr(1), nullptr, 16);
                g = { gsDPSetTextureImage(fmtVal, sizVal, width, seg | 1) };
            } else {
                g = { gsDPSetTextureImage(fmtVal, sizVal, width, 0) };
                g.words.w0 &= 0x00FFFFFF;
                g.words.w0 += (G_SETTIMG_OTR_FILEPATH << 24);
                char* str = (char*)malloc(fName.size() + 1);
                dl->Strings.push_back(str);
                g.words.w1 = (uintptr_t)str;
                strcpy((char*)g.words.w1, fName.data());
            }

        } else if (childName == "SetTile") {
            uint32_t line = child->IntAttribute("Line");
            uint32_t tmem = child->IntAttribute("TMem");
            uint32_t tile = child->IntAttribute("Tile");
            uint32_t palette = child->IntAttribute("Palette");
            std::string cms0 = child->Attribute("Cms0");
            std::string cms1 = child->Attribute("Cms1");
            std::string cmt0 = child->Attribute("Cmt0");
            std::string cmt1 = child->Attribute("Cmt1");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");

            std::string fmt = child->Attribute("Format");
            uint32_t fmtVal = G_IM_FMT_RGBA;

            if (fmt == "G_IM_FMT_I") {
                fmtVal = G_IM_FMT_I;
            } else if (fmt == "G_IM_FMT_IA") {
                fmtVal = G_IM_FMT_IA;
            } else if (fmt == "G_IM_FMT_CI") {
                fmtVal = G_IM_FMT_CI;
            } else if (fmt == "G_IM_FMT_YUV") {
                fmtVal = G_IM_FMT_YUV;
            } else if (fmt == "G_IM_FMT_RGBA") {
                fmtVal = G_IM_FMT_RGBA;
            }

            std::string siz = child->Attribute("Size");
            uint32_t sizVal = G_IM_SIZ_32b;

            if (siz == "G_IM_SIZ_8b_LOAD_BLOCK") {
                sizVal = G_IM_SIZ_8b_LOAD_BLOCK;
            } else if (siz == "G_IM_SIZ_4b") {
                sizVal = G_IM_SIZ_4b;
            } else if (siz == "G_IM_SIZ_8b") {
                sizVal = G_IM_SIZ_8b;
            } else if (siz == "G_IM_SIZ_16b" || siz == "G_IM_SIZ_16b_LOAD_BLOCK") {
                sizVal = G_IM_SIZ_16b;
            } else if (siz == "G_IM_SIZ_32b") {
                sizVal = G_IM_SIZ_32b;
            } else if (siz == "G_IM_SIZ_DD") {
                sizVal = G_IM_SIZ_DD;
            } else {
                int bp = 0;
            }

            uint32_t cms0Val = 0;
            uint32_t cms1Val = 0;
            uint32_t cmt0Val = 0;
            uint32_t cmt1Val = 0;

            if (cms0 == "G_TX_MIRROR") {
                cms0Val = G_TX_MIRROR;
            }

            if (cms0 == "G_TX_CLAMP") {
                cms0Val = G_TX_CLAMP;
            }

            if (cms1 == "G_TX_MIRROR") {
                cms1Val = G_TX_MIRROR;
            }

            if (cms1 == "G_TX_CLAMP") {
                cms1Val = G_TX_CLAMP;
            }

            if (cmt0 == "G_TX_MIRROR") {
                cmt0Val = G_TX_MIRROR;
            }

            if (cmt0 == "G_TX_CLAMP") {
                cmt0Val = G_TX_CLAMP;
            }

            if (cmt1 == "G_TX_MIRROR") {
                cmt1Val = G_TX_MIRROR;
            }

            if (cmt1 == "G_TX_CLAMP") {
                cmt1Val = G_TX_CLAMP;
            }

            g = gsDPSetTile(fmtVal, sizVal, line, tmem, tile, palette, cmt0Val | cmt1Val, maskT, shiftT,
                            cms0Val | cms1Val, maskS, shiftS);
        } else if (childName == "SetTileSize") {
            uint32_t t = child->IntAttribute("T");
            uint32_t uls = child->IntAttribute("Uls");
            uint32_t ult = child->IntAttribute("Ult");
            uint32_t lrs = child->IntAttribute("Lrs");
            uint32_t lrt = child->IntAttribute("Lrt");

            g = gsDPSetTileSize(t, uls, ult, lrs, lrt);
        } else if (childName == "SetOtherMode") {
            std::string cmdStr = child->Attribute("Cmd");
            int sft = child->IntAttribute("Sft");
            int length = child->IntAttribute("Length");
            uint32_t data = 0;
            uint32_t cmdVal = 0;

            if (cmdStr == "G_SETOTHERMODE_H") {
                cmdVal = G_SETOTHERMODE_H;
            } else if (cmdStr == "G_SETOTHERMODE_L") {
                cmdVal = G_SETOTHERMODE_L;
            }

            // OTRTODO: There are so many more of these we haven't added in yet...
            if (child->Attribute("G_AD_PATTERN", 0)) {
                data |= G_AD_PATTERN;
            }

            if (child->Attribute("G_AD_NOTPATTERN", 0)) {
                data |= G_AD_NOTPATTERN;
            }

            if (child->Attribute("G_AD_DISABLE", 0)) {
                data |= G_AD_DISABLE;
            }

            if (child->Attribute("G_AD_NOISE", 0)) {
                data |= G_AD_NOISE;
            }

            if (child->Attribute("G_CD_MAGICSQ", 0)) {
                data |= G_CD_MAGICSQ;
            }

            if (child->Attribute("G_CD_BAYER", 0)) {
                data |= G_CD_BAYER;
            }

            if (child->Attribute("G_CD_NOISE", 0)) {
                data |= G_CD_NOISE;
            }

            if (child->Attribute("G_CK_NONE", 0)) {
                data |= G_CK_NONE;
            }

            if (child->Attribute("G_CK_KEY", 0)) {
                data |= G_CK_KEY;
            }

            if (child->Attribute("G_TC_CONV", 0)) {
                data |= G_TC_CONV;
            }

            if (child->Attribute("G_TC_FILTCONV", 0)) {
                data |= G_TC_FILTCONV;
            }

            if (child->Attribute("G_TC_FILT", 0)) {
                data |= G_TC_FILT;
            }

            if (child->Attribute("G_TF_POINT", 0)) {
                data |= G_TF_POINT;
            }

            if (child->Attribute("G_TF_AVERAGE", 0)) {
                data |= G_TF_AVERAGE;
            }

            if (child->Attribute("G_TF_BILERP", 0)) {
                data |= G_TF_BILERP;
            }

            if (child->Attribute("G_TL_TILE", 0)) {
                data |= G_TL_TILE;
            }

            if (child->Attribute("G_TL_LOD", 0)) {
                data |= G_TL_LOD;
            }

            if (child->Attribute("G_TD_CLAMP", 0)) {
                data |= G_TD_CLAMP;
            }

            if (child->Attribute("G_TD_SHARPEN", 0)) {
                data |= G_TD_SHARPEN;
            }

            if (child->Attribute("G_TD_DETAIL", 0)) {
                data |= G_TD_DETAIL;
            }

            if (child->Attribute("G_TP_NONE", 0)) {
                data |= G_TP_NONE;
            }

            if (child->Attribute("G_TP_PERSP", 0)) {
                data |= G_TP_PERSP;
            }

            if (child->Attribute("G_CYC_1CYCLE", 0)) {
                data |= G_CYC_1CYCLE;
            }

            if (child->Attribute("G_CYC_COPY", 0)) {
                data |= G_CYC_COPY;
            }

            if (child->Attribute("G_CYC_FILL", 0)) {
                data |= G_CYC_FILL;
            }

            if (child->Attribute("G_CYC_2CYCLE", 0)) {
                data |= G_CYC_2CYCLE;
            }

            if (child->Attribute("G_PM_1PRIMITIVE", 0)) {
                data |= G_PM_1PRIMITIVE;
            }

            if (child->Attribute("G_PM_NPRIMITIVE", 0)) {
                data |= G_PM_NPRIMITIVE;
            }

            if (child->Attribute("G_RM_FOG_SHADE_A", 0)) {
                data |= G_RM_FOG_SHADE_A;
            }

            if (child->Attribute("G_RM_FOG_PRIM_A", 0)) {
                data |= G_RM_FOG_PRIM_A;
            }

            if (child->Attribute("G_RM_PASS", 0)) {
                data |= G_RM_PASS;
            }

            if (child->Attribute("G_ZS_PIXEL", 0)) {
                data |= G_ZS_PIXEL;
            }

            if (child->Attribute("G_ZS_PRIM", 0)) {
                data |= G_ZS_PRIM;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_SURF", 0)) {
                data |= G_RM_AA_ZB_OPA_SURF;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_SURF2", 0)) {
                data |= G_RM_AA_ZB_OPA_SURF2;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_SURF2", 0)) {
                data |= G_RM_AA_ZB_OPA_SURF2;
            }

            if (child->Attribute("G_RM_AA_ZB_XLU_SURF", 0)) {
                data |= G_RM_AA_ZB_XLU_SURF;
            }

            if (child->Attribute("G_RM_AA_ZB_XLU_SURF2", 0)) {
                data |= G_RM_AA_ZB_XLU_SURF2;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_DECAL", 0)) {
                data |= G_RM_AA_ZB_OPA_DECAL;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_DECAL2", 0)) {
                data |= G_RM_AA_ZB_OPA_DECAL2;
            }

            if (child->Attribute("G_RM_AA_ZB_XLU_DECAL", 0)) {
                data |= G_RM_AA_ZB_XLU_DECAL;
            }

            if (child->Attribute("G_RM_AA_ZB_XLU_DECAL2", 0)) {
                data |= G_RM_AA_ZB_XLU_DECAL2;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_INTER", 0)) {
                data |= G_RM_AA_ZB_OPA_INTER;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_INTER2", 0)) {
                data |= G_RM_AA_ZB_OPA_INTER2;
            }

            if (child->Attribute("G_RM_AA_ZB_XLU_INTER", 0)) {
                data |= G_RM_AA_ZB_XLU_INTER;
            }

            if (child->Attribute("G_RM_AA_ZB_XLU_INTER2", 0)) {
                data |= G_RM_AA_ZB_XLU_INTER2;
            }

            if (child->Attribute("G_RM_AA_ZB_XLU_LINE", 0)) {
                data |= G_RM_AA_ZB_XLU_LINE;
            }

            if (child->Attribute("G_RM_AA_ZB_XLU_LINE2", 0)) {
                data |= G_RM_AA_ZB_XLU_LINE2;
            }

            if (child->Attribute("G_RM_AA_ZB_DEC_LINE", 0)) {
                data |= G_RM_AA_ZB_DEC_LINE;
            }

            if (child->Attribute("G_RM_AA_ZB_DEC_LINE2", 0)) {
                data |= G_RM_AA_ZB_DEC_LINE2;
            }

            if (child->Attribute("G_RM_AA_ZB_TEX_EDGE", 0)) {
                data |= G_RM_AA_ZB_TEX_EDGE;
            }

            if (child->Attribute("G_RM_AA_ZB_TEX_EDGE2", 0)) {
                data |= G_RM_AA_ZB_TEX_EDGE2;
            }

            if (child->Attribute("G_RM_AA_ZB_TEX_INTER", 0)) {
                data |= G_RM_AA_ZB_TEX_INTER;
            }

            if (child->Attribute("G_RM_AA_ZB_TEX_INTER2", 0)) {
                data |= G_RM_AA_ZB_TEX_INTER2;
            }

            if (child->Attribute("G_RM_AA_ZB_SUB_SURF", 0)) {
                data |= G_RM_AA_ZB_SUB_SURF;
            }

            if (child->Attribute("G_RM_AA_ZB_SUB_SURF2", 0)) {
                data |= G_RM_AA_ZB_SUB_SURF2;
            }

            if (child->Attribute("G_RM_AA_ZB_PCL_SURF", 0)) {
                data |= G_RM_AA_ZB_PCL_SURF;
            }

            if (child->Attribute("G_RM_AA_ZB_PCL_SURF2", 0)) {
                data |= G_RM_AA_ZB_PCL_SURF2;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_TERR", 0)) {
                data |= G_RM_AA_ZB_OPA_TERR;
            }

            if (child->Attribute("G_RM_AA_ZB_OPA_TERR2", 0)) {
                data |= G_RM_AA_ZB_OPA_TERR2;
            }

            if (child->Attribute("G_RM_AA_ZB_TEX_TERR", 0)) {
                data |= G_RM_AA_ZB_TEX_TERR;
            }

            if (child->Attribute("G_RM_AA_ZB_TEX_TERR2", 0)) {
                data |= G_RM_AA_ZB_TEX_TERR2;
            }

            if (child->Attribute("G_RM_AA_ZB_SUB_TERR", 0)) {
                data |= G_RM_AA_ZB_SUB_TERR;
            }

            if (child->Attribute("G_RM_AA_ZB_SUB_TERR2", 0)) {
                data |= G_RM_AA_ZB_SUB_TERR2;
            }

            g = gsSPSetOtherMode(cmdVal, sft, length, data);
        } else if (childName == "SetRDPOtherMode") {
            g = gsDPSetOtherMode(child->UnsignedAttribute("Mode0"), child->UnsignedAttribute("Mode1"));
        } else if (childName == "LoadTextureBlock") {
            const char* fmtAttribute = child->Attribute("Format");
            std::string fmt = fmtAttribute != nullptr ? fmtAttribute : "0";
            uint32_t fmtVal = G_IM_FMT_RGBA;

            if (fmt == "G_IM_FMT_I") {
                fmtVal = G_IM_FMT_I;
            } else if (fmt == "G_IM_FMT_IA") {
                fmtVal = G_IM_FMT_IA;
            } else if (fmt == "G_IM_FMT_CI") {
                fmtVal = G_IM_FMT_CI;
            } else if (fmt == "G_IM_FMT_YUV") {
                fmtVal = G_IM_FMT_YUV;
            } else if (fmt == "G_IM_FMT_RGBA") {
                fmtVal = G_IM_FMT_RGBA;
            } else {
                fmtVal = std::stoul(fmt, nullptr, 0);
            }

            const char* sizAttribute = child->Attribute("Size");
            std::string siz = sizAttribute != nullptr ? sizAttribute : "0";
            uint32_t sizVal = G_IM_SIZ_32b;

            if (siz == "G_IM_SIZ_4b") {
                sizVal = G_IM_SIZ_4b;
            } else if (siz == "G_IM_SIZ_8b") {
                sizVal = G_IM_SIZ_8b;
            } else if (siz == "G_IM_SIZ_16b") {
                sizVal = G_IM_SIZ_16b;
            } else if (siz == "G_IM_SIZ_32b") {
                sizVal = G_IM_SIZ_32b;
            } else {
                sizVal = std::stoul(siz, nullptr, 0);
            }

            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = 0;
            uint32_t cmt = 0;

            if (child->Attribute("CMS_TXMirror", 0)) {
                cms |= G_TX_MIRROR;
            }

            if (child->Attribute("CMS_TXNoMirror", 0)) {
                cms |= G_TX_NOMIRROR;
            }

            if (child->Attribute("CMS_TXWrap", 0)) {
                cms |= G_TX_WRAP;
            }

            if (child->Attribute("CMS_TXClamp", 0)) {
                cms |= G_TX_CLAMP;
            }

            if (child->Attribute("CMT_TXMirror", 0)) {
                cmt |= G_TX_MIRROR;
            }

            if (child->Attribute("CMT_TXNoMirror", 0)) {
                cmt |= G_TX_NOMIRROR;
            }

            if (child->Attribute("CMT_TXWrap", 0)) {
                cmt |= G_TX_WRAP;
            }

            if (child->Attribute("CMT_TXClamp", 0)) {
                cmt |= G_TX_CLAMP;
            }

            Gfx g2[7];

            if (sizVal == G_IM_SIZ_4b) {
                Gfx g3[7] = { gsDPLoadTextureBlock_4b(0, fmtVal, width, height, 0, cms, cmt, maskS, maskT, shiftS,
                                                      shiftT) };
                memcpy(g2, g3, 7 * sizeof(Gfx));
            } else if (sizVal == G_IM_SIZ_8b) {
                Gfx g3[7] = { gsDPLoadTextureBlock(0, fmtVal, G_IM_SIZ_8b, width, height, 0, cms, cmt, maskS, maskT,
                                                   shiftS, shiftT) };
                memcpy(g2, g3, 7 * sizeof(Gfx));
            } else if (sizVal == G_IM_SIZ_16b) {
                Gfx g3[7] = { gsDPLoadTextureBlock(0, fmtVal, G_IM_SIZ_16b, width, height, 0, cms, cmt, maskS, maskT,
                                                   shiftS, shiftT) };
                memcpy(g2, g3, 7 * sizeof(Gfx));
            } else if (sizVal == G_IM_SIZ_32b) {
                Gfx g3[7] = { gsDPLoadTextureBlock(0, fmtVal, G_IM_SIZ_32b, width, height, 0, cms, cmt, maskS, maskT,
                                                   shiftS, shiftT) };
                memcpy(g2, g3, 7 * sizeof(Gfx));
            }

            std::string fName = child->Attribute("Path");
            char* str = (char*)malloc(fName.size() + 1);
            dl->Strings.push_back(str);
            strcpy(str, fName.data());

            g2[0].words.w0 = (g2[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            g2[0].words.w1 = (uintptr_t)str;

            for (int j = 0; j < 6; j++) {
                dl->Instructions.push_back(g2[j]);
            }

            g = g2[6];
        } else if (childName == "LoadTextureBlockS") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            std::string sizName = child->Attribute("Size") != nullptr ? child->Attribute("Size") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            auto sizEntry = imageSizes.find(sizName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t siz = sizEntry != imageSizes.end() ? sizEntry->second : std::stoul(sizName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = {};

            if (siz == G_IM_SIZ_8b) {
                Gfx generatedCommands[7] = { gsDPLoadTextureBlockS(0, fmt, G_IM_SIZ_8b, width, height, palette, cms,
                                                                   cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generatedCommands, sizeof(commands));
            } else if (siz == G_IM_SIZ_16b) {
                Gfx generatedCommands[7] = { gsDPLoadTextureBlockS(0, fmt, G_IM_SIZ_16b, width, height, palette, cms,
                                                                   cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generatedCommands, sizeof(commands));
            } else if (siz == G_IM_SIZ_32b) {
                Gfx generatedCommands[7] = { gsDPLoadTextureBlockS(0, fmt, G_IM_SIZ_32b, width, height, palette, cms,
                                                                   cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generatedCommands, sizeof(commands));
            }

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadTextureBlock4bS") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = { gsDPLoadTextureBlock_4bS(0, fmt, width, height, palette, cms, cmt, maskS, maskT, shiftS,
                                                         shiftT) };

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadMultiBlock") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            std::string sizName = child->Attribute("Size") != nullptr ? child->Attribute("Size") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            auto sizEntry = imageSizes.find(sizName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t siz = sizEntry != imageSizes.end() ? sizEntry->second : std::stoul(sizName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t tmem = child->UnsignedAttribute("TMem");
            uint32_t renderTile = child->UnsignedAttribute("RenderTile");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = {};

            if (siz == G_IM_SIZ_8b) {
                Gfx generated[7] = { gsDPLoadMultiBlock(0, tmem, renderTile, fmt, G_IM_SIZ_8b, width, height, palette,
                                                        cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            } else if (siz == G_IM_SIZ_16b) {
                Gfx generated[7] = { gsDPLoadMultiBlock(0, tmem, renderTile, fmt, G_IM_SIZ_16b, width, height, palette,
                                                        cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            } else if (siz == G_IM_SIZ_32b) {
                Gfx generated[7] = { gsDPLoadMultiBlock(0, tmem, renderTile, fmt, G_IM_SIZ_32b, width, height, palette,
                                                        cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            }

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadMultiBlockS") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            std::string sizName = child->Attribute("Size") != nullptr ? child->Attribute("Size") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            auto sizEntry = imageSizes.find(sizName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t siz = sizEntry != imageSizes.end() ? sizEntry->second : std::stoul(sizName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t tmem = child->UnsignedAttribute("TMem");
            uint32_t renderTile = child->UnsignedAttribute("RenderTile");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = {};

            if (siz == G_IM_SIZ_8b) {
                Gfx generated[7] = { gsDPLoadMultiBlockS(0, tmem, renderTile, fmt, G_IM_SIZ_8b, width, height, palette,
                                                         cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            } else if (siz == G_IM_SIZ_16b) {
                Gfx generated[7] = { gsDPLoadMultiBlockS(0, tmem, renderTile, fmt, G_IM_SIZ_16b, width, height, palette,
                                                         cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            } else if (siz == G_IM_SIZ_32b) {
                Gfx generated[7] = { gsDPLoadMultiBlockS(0, tmem, renderTile, fmt, G_IM_SIZ_32b, width, height, palette,
                                                         cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            }

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadMultiBlock4b") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t tmem = child->UnsignedAttribute("TMem");
            uint32_t renderTile = child->UnsignedAttribute("RenderTile");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = { gsDPLoadMultiBlock_4b(0, tmem, renderTile, fmt, width, height, palette, cms, cmt, maskS,
                                                      maskT, shiftS, shiftT) };

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadMultiBlock4bS") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t tmem = child->UnsignedAttribute("TMem");
            uint32_t renderTile = child->UnsignedAttribute("RenderTile");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = { gsDPLoadMultiBlock_4bS(0, tmem, renderTile, fmt, width, height, palette, cms, cmt,
                                                       maskS, maskT, shiftS, shiftT) };

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadTextureTile") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            std::string sizName = child->Attribute("Size") != nullptr ? child->Attribute("Size") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            auto sizEntry = imageSizes.find(sizName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t siz = sizEntry != imageSizes.end() ? sizEntry->second : std::stoul(sizName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t uls = child->UnsignedAttribute("Uls");
            uint32_t ult = child->UnsignedAttribute("Ult");
            uint32_t lrs = child->UnsignedAttribute("Lrs");
            uint32_t lrt = child->UnsignedAttribute("Lrt");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = {};

            if (siz == G_IM_SIZ_8b) {
                Gfx generated[7] = { gsDPLoadTextureTile(0, fmt, G_IM_SIZ_8b, width, height, uls, ult, lrs, lrt,
                                                         palette, cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            } else if (siz == G_IM_SIZ_16b) {
                Gfx generated[7] = { gsDPLoadTextureTile(0, fmt, G_IM_SIZ_16b, width, height, uls, ult, lrs, lrt,
                                                         palette, cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            } else if (siz == G_IM_SIZ_32b) {
                Gfx generated[7] = { gsDPLoadTextureTile(0, fmt, G_IM_SIZ_32b, width, height, uls, ult, lrs, lrt,
                                                         palette, cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            }

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadTextureTile4b") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t uls = child->UnsignedAttribute("Uls");
            uint32_t ult = child->UnsignedAttribute("Ult");
            uint32_t lrs = child->UnsignedAttribute("Lrs");
            uint32_t lrt = child->UnsignedAttribute("Lrt");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = { gsDPLoadTextureTile_4b(0, fmt, width, height, uls, ult, lrs, lrt, palette, cms, cmt,
                                                       maskS, maskT, shiftS, shiftT) };

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadMultiTile") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            std::string sizName = child->Attribute("Size") != nullptr ? child->Attribute("Size") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            auto sizEntry = imageSizes.find(sizName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t siz = sizEntry != imageSizes.end() ? sizEntry->second : std::stoul(sizName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t uls = child->UnsignedAttribute("Uls");
            uint32_t ult = child->UnsignedAttribute("Ult");
            uint32_t lrs = child->UnsignedAttribute("Lrs");
            uint32_t lrt = child->UnsignedAttribute("Lrt");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t tmem = child->UnsignedAttribute("TMem");
            uint32_t renderTile = child->UnsignedAttribute("RenderTile");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = {};

            if (siz == G_IM_SIZ_8b) {
                Gfx generated[7] = { gsDPLoadMultiTile(0, tmem, renderTile, fmt, G_IM_SIZ_8b, width, height, uls, ult,
                                                       lrs, lrt, palette, cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            } else if (siz == G_IM_SIZ_16b) {
                Gfx generated[7] = { gsDPLoadMultiTile(0, tmem, renderTile, fmt, G_IM_SIZ_16b, width, height, uls, ult,
                                                       lrs, lrt, palette, cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            } else if (siz == G_IM_SIZ_32b) {
                Gfx generated[7] = { gsDPLoadMultiTile(0, tmem, renderTile, fmt, G_IM_SIZ_32b, width, height, uls, ult,
                                                       lrs, lrt, palette, cms, cmt, maskS, maskT, shiftS, shiftT) };
                memcpy(commands, generated, sizeof(commands));
            }

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadMultiTile4b") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t uls = child->UnsignedAttribute("Uls");
            uint32_t ult = child->UnsignedAttribute("Ult");
            uint32_t lrs = child->UnsignedAttribute("Lrs");
            uint32_t lrt = child->UnsignedAttribute("Lrt");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t tmem = child->UnsignedAttribute("TMem");
            uint32_t renderTile = child->UnsignedAttribute("RenderTile");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = { gsDPLoadMultiTile_4b(0, tmem, renderTile, fmt, width, height, uls, ult, lrs, lrt,
                                                     palette, cms, cmt, maskS, maskT, shiftS, shiftT) };

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadTextureBlockYuv") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            std::string sizName = child->Attribute("Size") != nullptr ? child->Attribute("Size") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            auto sizEntry = imageSizes.find(sizName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t siz = sizEntry != imageSizes.end() ? sizEntry->second : std::stoul(sizName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = {};
            Gfx* command = commands;

            if (siz == G_IM_SIZ_8b) {
                gDPLoadTextureBlockYuv(command++, 0, fmt, G_IM_SIZ_8b, width, height, palette, cms, cmt, maskS, maskT,
                                       shiftS, shiftT);
            } else if (siz == G_IM_SIZ_16b) {
                gDPLoadTextureBlockYuv(command++, 0, fmt, G_IM_SIZ_16b, width, height, palette, cms, cmt, maskS, maskT,
                                       shiftS, shiftT);
            } else if (siz == G_IM_SIZ_32b) {
                gDPLoadTextureBlockYuv(command++, 0, fmt, G_IM_SIZ_32b, width, height, palette, cms, cmt, maskS, maskT,
                                       shiftS, shiftT);
            }

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "LoadTextureBlockYuvS") {
            std::string fmtName = child->Attribute("Format") != nullptr ? child->Attribute("Format") : "0";
            std::string sizName = child->Attribute("Size") != nullptr ? child->Attribute("Size") : "0";
            auto fmtEntry = imageFormats.find(fmtName);
            auto sizEntry = imageSizes.find(sizName);
            uint32_t fmt = fmtEntry != imageFormats.end() ? fmtEntry->second : std::stoul(fmtName, nullptr, 0);
            uint32_t siz = sizEntry != imageSizes.end() ? sizEntry->second : std::stoul(sizName, nullptr, 0);
            uint32_t width = child->IntAttribute("Width");
            uint32_t height = child->IntAttribute("Height");
            uint32_t palette = child->UnsignedAttribute("Palette");
            uint32_t maskS = child->IntAttribute("MaskS");
            uint32_t maskT = child->IntAttribute("MaskT");
            uint32_t shiftS = child->IntAttribute("ShiftS");
            uint32_t shiftT = child->IntAttribute("ShiftT");
            uint32_t cms = (child->Attribute("CMS_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMS_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMS_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMS_TXClamp", 0) ? G_TX_CLAMP : 0);
            uint32_t cmt = (child->Attribute("CMT_TXMirror", 0) ? G_TX_MIRROR : 0) |
                           (child->Attribute("CMT_TXNoMirror", 0) ? G_TX_NOMIRROR : 0) |
                           (child->Attribute("CMT_TXWrap", 0) ? G_TX_WRAP : 0) |
                           (child->Attribute("CMT_TXClamp", 0) ? G_TX_CLAMP : 0);
            Gfx commands[7] = {};
            Gfx* command = commands;

            if (siz == G_IM_SIZ_8b) {
                gDPLoadTextureBlockYuvS(command++, 0, fmt, G_IM_SIZ_8b, width, height, palette, cms, cmt, maskS, maskT,
                                        shiftS, shiftT);
            } else if (siz == G_IM_SIZ_16b) {
                gDPLoadTextureBlockYuvS(command++, 0, fmt, G_IM_SIZ_16b, width, height, palette, cms, cmt, maskS, maskT,
                                        shiftS, shiftT);
            } else if (siz == G_IM_SIZ_32b) {
                gDPLoadTextureBlockYuvS(command++, 0, fmt, G_IM_SIZ_32b, width, height, palette, cms, cmt, maskS, maskT,
                                        shiftS, shiftT);
            }

            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            strcpy(str, path.c_str());
            dl->Strings.push_back(str);
            commands[0].words.w0 = (commands[0].words.w0 & 0x00FFFFFF) | (G_SETTIMG_OTR_FILEPATH << 24);
            commands[0].words.w1 = reinterpret_cast<uintptr_t>(str);
            for (int i = 0; i < 6; i++) {
                dl->Instructions.push_back(commands[i]);
            }
            g = commands[6];
        } else if (childName == "EndDisplayList") {
            g = gsSPEndDisplayList();
        } else if (childName == "CullDisplayList") {
            uint32_t start = child->IntAttribute("Start");
            uint32_t end = child->IntAttribute("End");

            g = gsSPCullDisplayList(start, end);
        } else if (childName == "ClipRatio") {
            uint32_t ratio = child->IntAttribute("Start");
            Gfx g2[4];

            switch (ratio) {
                case 1: {
                    Gfx g3[4] = { gsSPClipRatio(FRUSTRATIO_1) };
                    memcpy(g2, g3, sizeof(Gfx) * 4);
                } break;
                case 2: {
                    Gfx g3[4] = { gsSPClipRatio(FRUSTRATIO_2) };
                    memcpy(g2, g3, sizeof(Gfx) * 4);
                } break;
                case 3: {
                    Gfx g3[4] = { gsSPClipRatio(FRUSTRATIO_3) };
                    memcpy(g2, g3, sizeof(Gfx) * 4);
                } break;
                case 4: {
                    Gfx g3[4] = { gsSPClipRatio(FRUSTRATIO_4) };
                    memcpy(g2, g3, sizeof(Gfx) * 4);
                } break;
                case 5: {
                    Gfx g3[4] = { gsSPClipRatio(FRUSTRATIO_5) };
                    memcpy(g2, g3, sizeof(Gfx) * 4);
                } break;
                case 6: {
                    Gfx g3[4] = { gsSPClipRatio(FRUSTRATIO_6) };
                    memcpy(g2, g3, sizeof(Gfx) * 4);
                } break;
            }

            for (int j = 0; j < 3; j++) {
                dl->Instructions.push_back(g2[j]);
            }
            g = g2[3];
        } else if (childName == "JumpToDisplayList") {
            std::string dlPath = (char*)child->Attribute("Path");
            if (dlPath[0] == '>' && dlPath[1] == '0' && (dlPath[2] == 'x' || dlPath[2] == 'X')) {
                uint32_t seg = std::stoul(dlPath.substr(1), nullptr, 16);
                g = { gsSPBranchListOTRHash(seg | 1) };
            } else {
                char* dlPath2 = (char*)malloc(strlen(dlPath.c_str()) + 1);
                dl->Strings.push_back(dlPath2);
                strcpy(dlPath2, dlPath.c_str());

                g = gsSPBranchListOTRFilePath(dlPath2);
            }
        } else if (childName == "CallDisplayList") {
            std::string dlPath = (char*)child->Attribute("Path");
            if (dlPath[0] == '>' && dlPath[1] == '0' && (dlPath[2] == 'x' || dlPath[2] == 'X')) {
                uint32_t seg = std::stoul(dlPath.substr(1), nullptr, 16);
                g = { gsSPDisplayList(seg | 1) };
            } else {
                char* dlPath2 = (char*)malloc(strlen(dlPath.c_str()) + 1);
                dl->Strings.push_back(dlPath2);
                strcpy(dlPath2, dlPath.c_str());

                g = gsSPDisplayListOTRFilePath(dlPath2);
            }
        } else if (childName == "GeometryMode") {
            const uint32_t clear = child->UnsignedAttribute("Clear");
            const uint32_t set = child->UnsignedAttribute("Set");
#ifdef F3DEX_GBI_2
            g = gsSPGeometryMode(clear, set);
#else
            dl->Instructions.push_back(gsSPClearGeometryMode(clear));
            g = gsSPSetGeometryMode(set);
#endif
        } else if (childName == "LoadGeometryMode") {
            const uint32_t mode = child->UnsignedAttribute("Mode");
#ifdef F3DEX_GBI_2
            g = gsSPLoadGeometryMode(mode);
#else
            dl->Instructions.push_back(gsSPClearGeometryMode(UINT32_MAX));
            g = gsSPSetGeometryMode(mode);
#endif
        } else if (childName == "ClearGeometryMode" || childName == "SetGeometryMode") {
            uint64_t clearData = 0;

            if (child->Attribute("G_SHADE", 0)) {
                clearData |= G_SHADE;
            }

            if (child->Attribute("G_LIGHTING", 0)) {
                clearData |= G_LIGHTING;
            }

            if (child->Attribute("G_SHADING_SMOOTH", 0)) {
                clearData |= G_SHADING_SMOOTH;
            }

            if (child->Attribute("G_ZBUFFER", 0)) {
                clearData |= G_ZBUFFER;
            }

            if (child->Attribute("G_TEXTURE_GEN", 0)) {
                clearData |= G_TEXTURE_GEN;
            }

            if (child->Attribute("G_TEXTURE_GEN_LINEAR", 0)) {
                clearData |= G_TEXTURE_GEN_LINEAR;
            }

            if (child->Attribute("G_CULL_BACK", 0)) {
                clearData |= G_CULL_BACK;
            }

            if (child->Attribute("G_CULL_FRONT", 0)) {
                clearData |= G_CULL_FRONT;
            }

            if (child->Attribute("G_CULL_BOTH", 0)) {
                clearData |= G_CULL_BOTH;
            }

            if (child->Attribute("G_FOG", 0)) {
                clearData |= G_FOG;
            }

            if (child->Attribute("G_CLIPPING", 0)) {
                clearData |= G_CLIPPING;
            }

            if (childName == "ClearGeometryMode") {
                g = gsSPClearGeometryMode(clearData);
            } else {
                g = gsSPSetGeometryMode(clearData);
            }
        } else if (childName == "LightColor") {
            int n = child->IntAttribute("N");
            uint32_t col = child->IntAttribute("Col");

            Gfx g2[2];

            switch (n) {
                case 1: {
                    Gfx g3[2] = { gsSPLightColor(LIGHT_1, col) };
                    memcpy(g2, g3, sizeof(Gfx) * 2);
                } break;
                case 2: {
                    Gfx g3[2] = { gsSPLightColor(LIGHT_2, col) };
                    memcpy(g2, g3, sizeof(Gfx) * 2);
                } break;
                case 3: {
                    Gfx g3[2] = { gsSPLightColor(LIGHT_3, col) };
                    memcpy(g2, g3, sizeof(Gfx) * 2);
                } break;
                case 4: {
                    Gfx g3[2] = { gsSPLightColor(LIGHT_4, col) };
                    memcpy(g2, g3, sizeof(Gfx) * 2);
                } break;
                case 5: {
                    Gfx g3[2] = { gsSPLightColor(LIGHT_5, col) };
                    memcpy(g2, g3, sizeof(Gfx) * 2);
                } break;
                case 6: {
                    Gfx g3[2] = { gsSPLightColor(LIGHT_6, col) };
                    memcpy(g2, g3, sizeof(Gfx) * 2);
                } break;
                case 7: {
                    Gfx g3[2] = { gsSPLightColor(LIGHT_7, col) };
                    memcpy(g2, g3, sizeof(Gfx) * 2);
                } break;
                case 8: {
                    Gfx g3[2] = { gsSPLightColor(LIGHT_8, col) };
                    memcpy(g2, g3, sizeof(Gfx) * 2);
                } break;
            }

            dl->Instructions.push_back(g2[0]);
            g = g2[1];
        } else if (childName == "SetRenderMode") {
            std::string rawMode1 = child->Attribute("Mode1");
            std::string rawMode2 = child->Attribute("Mode2");
            g = gsDPSetRenderMode(renderModes[rawMode1], renderModes[rawMode2]);
        } else if (childName == "SetFramebuffer") {
            gsSPSetFB(&g, child->UnsignedAttribute("Id"));
        } else if (childName == "ResetFramebuffer") {
            gsSPResetFB(&g);
        } else if (childName == "SetTextureImageFramebuffer") {
            gDPSetTextureImageFB(&g, 0, 0, 1, child->UnsignedAttribute("Id"));
        } else if (childName == "InvalidateTextureCache") {
            __gSPInvalidateTexCache(&g, child->Unsigned64Attribute("Address"));
        } else if (childName == "ExtraGeometryMode") {
            gSPExtraGeometryMode(&g, child->UnsignedAttribute("Clear"), child->UnsignedAttribute("Set"));
        } else if (childName == "SetInterpolationTarget") {
            gDPSetInterpolation(&g, child->UnsignedAttribute("Index"));
        } else if (childName == "SetTileSizeInterpolated") {
            Gfx g2[3];
            __gDPSetTileSizeInterp(g2, child->UnsignedAttribute("Tile"), child->UnsignedAttribute("Uls"),
                                   child->UnsignedAttribute("Ult"), child->UnsignedAttribute("Lrs"),
                                   child->UnsignedAttribute("Lrt"));
            float coords[4] = { child->FloatAttribute("UlsFloat"), child->FloatAttribute("UltFloat"),
                                child->FloatAttribute("LrsFloat"), child->FloatAttribute("LrtFloat") };
            memcpy(&g2[1].words.w0, &coords[0], sizeof(float));
            memcpy(&g2[1].words.w1, &coords[1], sizeof(float));
            memcpy(&g2[2].words.w0, &coords[2], sizeof(float));
            memcpy(&g2[2].words.w1, &coords[3], sizeof(float));
            dl->Instructions.push_back(g2[0]);
            dl->Instructions.push_back(g2[1]);
            g = g2[2];
        } else if (childName == "WideTextureRectangle") {
            Gfx g2[3] = { gsSPWideTextureRectangle(
                child->IntAttribute("Xl"), child->IntAttribute("Yl"), child->IntAttribute("Xh"),
                child->IntAttribute("Yh"), child->IntAttribute("Tile"), child->IntAttribute("S"),
                child->IntAttribute("T"), child->IntAttribute("Dsdx"), child->IntAttribute("Dtdy")) };
            dl->Instructions.push_back(g2[0]);
            dl->Instructions.push_back(g2[1]);
            g = g2[2];
        } else if (childName == "FillWideRectangle") {
            Gfx g2[2];
            g2[0].words.w0 = _SHIFTL(G_FILLWIDERECT, 24, 8) | _SHIFTL(child->IntAttribute("Lrx"), 2, 22);
            g2[0].words.w1 = _SHIFTL(child->IntAttribute("Lry"), 2, 22);
            g2[1].words.w0 = _SHIFTL(child->IntAttribute("Ulx"), 2, 22);
            g2[1].words.w1 = _SHIFTL(child->IntAttribute("Uly"), 2, 22);
            dl->Instructions.push_back(g2[0]);
            g = g2[1];
        } else if (childName == "ImageRectangle") {
            Gfx g2[3];
            g2[0].words.w0 = _SHIFTL(G_IMAGERECT, 24, 8) | _SHIFTL(child->IntAttribute("Tile"), 0, 3);
            g2[0].words.w1 =
                _SHIFTL(child->IntAttribute("ImageWidth"), 16, 16) | _SHIFTL(child->IntAttribute("ImageHeight"), 0, 16);
            g2[1].words.w0 = _SHIFTL(child->IntAttribute("X0"), 16, 16) | _SHIFTL(child->IntAttribute("Y0"), 0, 16);
            g2[1].words.w1 = _SHIFTL(child->IntAttribute("S0"), 16, 16) | _SHIFTL(child->IntAttribute("T0"), 0, 16);
            g2[2].words.w0 = _SHIFTL(child->IntAttribute("X1"), 16, 16) | _SHIFTL(child->IntAttribute("Y1"), 0, 16);
            g2[2].words.w1 = _SHIFTL(child->IntAttribute("S1"), 16, 16) | _SHIFTL(child->IntAttribute("T1"), 0, 16);
            dl->Instructions.push_back(g2[0]);
            dl->Instructions.push_back(g2[1]);
            g = g2[2];
        } else if (childName == "BackgroundCopy" || childName == "Background1Cycle" || childName == "ObjectRectangle" ||
                   childName == "ObjectRectangleR") {
            uintptr_t address = child->Unsigned64Attribute("Address");
            if (childName == "BackgroundCopy") {
                g = gsSPBgRectCopy(address);
            } else if (childName == "Background1Cycle") {
                g = gsSPBgRect1Cyc(address);
            } else if (childName == "ObjectRectangle") {
                g = gsSPObjRectangle(address);
            } else {
                g = gsSPObjRectangleR(address);
            }
        } else if (childName == "ObjectRenderMode") {
            std::string modes = child->Attribute("Mode") != nullptr ? child->Attribute("Mode") : "0";
            uint32_t modeValue = 0;
            size_t position = 0;

            while (position <= modes.size()) {
                size_t separator = modes.find('|', position);
                std::string mode = modes.substr(position, separator - position);
                size_t first = mode.find_first_not_of(" \t");
                size_t last = mode.find_last_not_of(" \t");
                mode = first == std::string::npos ? "0" : mode.substr(first, last - first + 1);
                auto entry = objectRenderModes.find(mode);
                modeValue |= entry != objectRenderModes.end() ? entry->second : std::stoul(mode, nullptr, 0);

                if (separator == std::string::npos) {
                    break;
                }
                position = separator + 1;
            }

            g = gsSPObjRenderMode(modeValue);
        } else if (childName == "LoadVerticesWide") {
            const uint32_t count = child->UnsignedAttribute("Count");
            const uint32_t v0 = child->UnsignedAttribute("VertexBufferIndex");
            g.words.w0 = _SHIFTL(G_VTX_WIDE, 24, 8) | _SHIFTL(count, 12, 8) | _SHIFTL(v0 + count, 1, 7);
            g.words.w1 = child->Unsigned64Attribute("Address");
        } else if (childName == "DisplayListMarker") {
            Gfx header;
            header.words.w0 = _SHIFTL(G_MARKER, 24, 8);
            header.words.w1 = 0;
            dl->Instructions.push_back(header);
            const uint64_t hash = CRC64(child->Attribute("Path"));
            g.words.w0 = static_cast<uint32_t>(hash >> 32);
            g.words.w1 = static_cast<uint32_t>(hash);
        } else if (childName == "BranchLessZHash") {
            Gfx header;
            header.words.w0 = _SHIFTL(G_BRANCH_Z_OTR, 24, 8) | _SHIFTL(child->UnsignedAttribute("Vertex"), 0, 12);
            header.words.w1 = child->UnsignedAttribute("ZValue");
            dl->Instructions.push_back(header);
            const uint64_t hash = CRC64(child->Attribute("Path"));
            g.words.w0 = static_cast<uint32_t>(hash >> 32);
            g.words.w1 = static_cast<uint32_t>(hash);
        } else if (childName == "MoveMemHash") {
            Gfx header;
            header.words.w0 = _SHIFTL(G_MOVEMEM_OTR, 24, 8);
            header.words.w1 = _SHIFTL(child->UnsignedAttribute("Index"), 24, 8) |
                              _SHIFTL(child->UnsignedAttribute("Offset"), 16, 8) |
                              _SHIFTL(child->BoolAttribute("HasOffset"), 8, 8);
            dl->Instructions.push_back(header);
            const uint64_t hash = CRC64(child->Attribute("Path"));
            g.words.w0 = static_cast<uint32_t>(hash >> 32);
            g.words.w1 = static_cast<uint32_t>(hash);
        } else if (childName == "PushCurrentDirectory") {
            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            dl->Strings.push_back(str);
            strcpy(str, path.c_str());
            gsSPPushCD(&g, str);
        } else if (childName == "CopyFramebuffer") {
            gDPCopyFB(&g, child->UnsignedAttribute("Destination"), child->UnsignedAttribute("Source"),
                      child->BoolAttribute("Once"), child->Unsigned64Attribute("CopiedAddress"));
        } else if (childName == "ReadFramebuffer") {
            Gfx header;
            header.words.w0 = _SHIFTL(G_READFB, 24, 8) | _SHIFTL(child->BoolAttribute("ByteSwap"), 8, 1) |
                              _SHIFTL(child->UnsignedAttribute("Source"), 0, 8);
            header.words.w1 = child->Unsigned64Attribute("DestinationAddress");
            dl->Instructions.push_back(header);
            g.words.w0 =
                _SHIFTL(child->UnsignedAttribute("Uly"), 16, 16) | _SHIFTL(child->UnsignedAttribute("Ulx"), 0, 16);
            g.words.w1 =
                _SHIFTL(child->UnsignedAttribute("Height"), 16, 16) | _SHIFTL(child->UnsignedAttribute("Width"), 0, 16);
        } else if (childName == "RegisterBlendedTexture") {
            Gfx header;
            header.words.w0 = _SHIFTL(G_REGBLENDEDTEX, 24, 8);
            std::string path = child->Attribute("Path");
            char* str = static_cast<char*>(malloc(path.size() + 1));
            dl->Strings.push_back(str);
            strcpy(str, path.c_str());
            header.words.w1 = reinterpret_cast<uintptr_t>(str);
            dl->Instructions.push_back(header);
            g.words.w0 = child->Unsigned64Attribute("MaskAddress");
            g.words.w1 = child->Unsigned64Attribute("ReplacementAddress");
        } else if (childName == "PushShader") {
            const char* shader = child->Attribute("Shader", nullptr);
            if (shader == nullptr) {
                printf("DisplayListXML: PushShader is missing its Shader attribute\n");
            } else {
                char* shaderCopy = (char*)malloc(strlen(shader) + 1);
                dl->Strings.push_back(shaderCopy);
                strcpy(shaderCopy, shader);
                g = GsSPPushShader(shaderCopy);
            }
        } else if (childName == "PopShader") {
            g = GsSPPopShader();
        } else {
            printf("DisplayListXML: Unknown node %s\n", childName.c_str());
            g = gsDPPipeSync();
        }

        dl->Instructions.push_back(g);

        child = child->NextSiblingElement();
    }

#ifdef F3DEX_GBI_2
    dl->UCode = ucode_f3dex2;
#elif defined(F3DEX_GBI)
    dl->UCode = ucode_f3dex;
#elif defined(F3D_OLD)
    dl->UCode = ucode_f3d;
#endif

    return dl;
}
} // namespace Fast
