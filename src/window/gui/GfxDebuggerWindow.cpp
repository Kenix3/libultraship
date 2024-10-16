#include "GfxDebuggerWindow.h"
#include <imgui.h>
#include <spdlog/spdlog.h>
#include "Context.h"
#include "graphic/Fast3D/debug/GfxDebugger.h"
#include <stack>
#include <spdlog/fmt/fmt.h>
#include "libultraship/bridge.h"
#include <graphic/Fast3D/gfx_pc.h>
#include <optional>
#ifdef GFX_DEBUG_DISASSEMBLER
#include <gfxd.h>
#endif

namespace LUS {

GfxDebuggerWindow::~GfxDebuggerWindow() {
}

void GfxDebuggerWindow::InitElement() {
}

void GfxDebuggerWindow::UpdateElement() {
}

// LUSTODO handle switching ucodes
static const char* GetOpName(int8_t op) {
    return GfxGetOpcodeName(op);
}

static inline void* seg_addr(uintptr_t w1) {
    // Segmented?
    if (w1 & 1) {
        uint32_t segNum = (w1 >> 24);

        uint32_t offset = w1 & 0x00FFFFFE;
        // offset = 0; // Cursed Malon bug

        if (gSegmentPointers[segNum] != 0) {
            return (void*)(gSegmentPointers[segNum] + offset);
        } else {
            return (void*)w1;
        }
    } else {
        return (void*)w1;
    }
}

#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << width) - 1))

// static int s_dbgcnt = 0;
void GfxDebuggerWindow::DrawDisasNode(const F3DGfx* cmd, std::vector<const F3DGfx*>& gfxPath,
                                      float parentPosY = 0) const {
    const F3DGfx* dlStart = cmd;
    auto dbg = Ship::Context::GetInstance()->GetGfxDebugger();

    auto nodeWithText = [dbg, dlStart, parentPosY, this, &gfxPath](const F3DGfx* cmd, const std::string& text,
                                                                   const F3DGfx* sub = nullptr) mutable {
        gfxPath.push_back(cmd);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (dbg->HasBreakPoint(gfxPath)) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (sub == nullptr) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        bool scrollTo = false;
        float curPosY = ImGui::GetCursorPosY();
        bool open = ImGui::TreeNodeEx((const void*)cmd, flags, "%p:%4d: %s", cmd,
                                      (int)(((uintptr_t)cmd - (uintptr_t)dlStart) / sizeof(F3DGfx)), text.c_str());

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            dbg->SetBreakPoint(gfxPath);
        }

        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::Selectable("Copy text")) {
                SDL_SetClipboardText(text.c_str());
            }
            if (ImGui::Selectable("Copy address")) {
                std::string address = fmt::format("0x{:x}", (uintptr_t)cmd);
                SDL_SetClipboardText(address.c_str());
            }
            if (parentPosY > 0) {
                scrollTo = ImGui::Selectable("Scroll to parent");
            }
            ImGui::EndPopup();
        }

        if (scrollTo) {
            ImGui::SetScrollY(parentPosY);
        }

        if (open) {
            if (sub) {
                DrawDisasNode(sub, gfxPath, curPosY);
            }
            ImGui::TreePop();
        }
        gfxPath.pop_back();
    };

    auto simpleNode = [dbg, nodeWithText](const F3DGfx* cmd, int8_t opcode) mutable {
        const char* opname = GetOpName(opcode);

        if (opname) {
#ifdef GFX_DEBUG_DISASSEMBLER
            size_t size = 1;

            // Texture rectangle is larger due to RDPHALF
            if (opcode == RDP_G_TEXRECT || opcode == RDP_G_TEXRECTFLIP) {
                size = 3;
            }

            // Our Gfx uses uinptr_t for words, but libgfxd uses uint32_t,
            // Copy only the first 32bits of each word into a vector before passing the instructions
            std::vector<uint32_t> input;
            for (size_t i = 0; i < size; i++) {
                input.push_back(cmd[i].words.w0 & 0xFFFFFFFF);
                input.push_back(cmd[i].words.w1 & 0xFFFFFFFF);
            }

            gfxd_input_buffer(input.data(), sizeof(uint32_t) * size * 2);
            gfxd_endian(gfxd_endian_host, sizeof(uint32_t));
            char buff[512] = { 0 };
            gfxd_output_buffer(buff, sizeof(buff));
            gfxd_enable(gfxd_emit_dec_color);

            // Load the correct GBI
#if defined(F3DEX_GBI_2)
            gfxd_target(gfxd_f3dex2);
#elif defined(F3DEX_GBI)
            gfxd_target(gfxd_f3dex);
#else
            gfxd_target(gfxd_f3d);
#endif

            gfxd_execute();

            nodeWithText(cmd, fmt::format("{}", buff));
#else
            nodeWithText(cmd, fmt::format("{}", opname));
#endif // GFX_DEBUG_DISASSEMBLER
        } else {
            int8_t opcode = (int8_t)(cmd->words.w0 >> 24);
            nodeWithText(cmd, fmt::format("UNK: 0x{:X}", opcode));
        }
    };

    while (true) {
        int8_t opcode = (int8_t)(cmd->words.w0 >> 24);
        const F3DGfx* cmd0 = cmd;
        switch (opcode) {

#ifdef F3DEX_GBI_2
            case F3DEX2_G_NOOP: {
                const char* filename = (const char*)cmd->words.w1;
                uint32_t p = C0(16, 8);
                uint32_t l = C0(0, 16);

                if (p == 7) {
                    nodeWithText(cmd0, fmt::format("gDPNoOpOpenDisp(): {}:{}", filename, l));
                } else if (p == 8) {
                    nodeWithText(cmd0, fmt::format("gDPNoOpCloseDisp(): {}:{}", filename, l));
                } else {
                    simpleNode(cmd0, opcode);
                }

                cmd++;
                break;
            }
#endif

#ifdef F3DEX_GBI_2 // Different opcodes, same behavior. Handle subtrees for DL calls.
            case F3DEX2_G_DL: {
#else
            case F3DEX_G_DL: {
#endif
                F3DGfx* subGFX = (F3DGfx*)seg_addr(cmd->words.w1);
                if (C0(16, 1) == 0) {
                    nodeWithText(cmd0, fmt::format("G_DL: 0x{:x} -> {}", cmd->words.w1, (void*)subGFX), subGFX);
                    cmd++;
                } else {
                    nodeWithText(cmd0, fmt::format("G_DL (branch): 0x{:x} -> {}", cmd->words.w1, (void*)subGFX),
                                 subGFX);
                    return;
                }
                break;
            }

#ifdef F3DEX_GBI_2 // Different opcodes, same behavior. Return out of subtree.
            case F3DEX2_G_ENDDL: {
#else
            case F3DEX_G_ENDDL: {
#endif
                simpleNode(cmd, opcode);
                return;
            }

            // Increment 3 times because texture rectangle uses RDPHALF
            case RDP_G_TEXRECTFLIP:
            case RDP_G_TEXRECT: {
                simpleNode(cmd, opcode);
                cmd += 3;
                break;
            }

            case OTR_G_MARKER: {
                cmd++;
                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                const char* dlName = ResourceGetNameByCrc(hash);
                if (!dlName) {
                    dlName = "UNKNOWN";
                }

                nodeWithText(cmd0, fmt::format("G_MARKER: {}", dlName));
                cmd++;
                break;
            }

            case OTR_G_DL_OTR_HASH: {
                if (C0(16, 1) == 0) {
                    cmd++;
                    uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                    const char* dlName = ResourceGetNameByCrc(hash);
                    if (!dlName)
                        dlName = "UNKNOWN";

                    F3DGfx* subGfx = (F3DGfx*)ResourceGetDataByCrc(hash);
                    nodeWithText(cmd0, fmt::format("G_DL_OTR_HASH: {}", dlName), subGfx);
                    cmd++;
                } else {
                    assert(0 && "Invalid in gfx_pc????");
                }
                break;
            }

            case OTR_G_DL_OTR_FILEPATH: {
                char* fileName = (char*)cmd->words.w1;
                F3DGfx* subGfx = (F3DGfx*)ResourceGetDataByName((const char*)fileName);

                if (subGfx == nullptr) {
                    assert(0 && "in gfx_pc????");
                }

                if (C0(16, 1) == 0 && subGfx != nullptr) {
                    nodeWithText(cmd0, fmt::format("G_DL_OTR_HASH: {}", fileName), subGfx);
                    cmd++;
                    break;
                } else {
                    nodeWithText(cmd0, fmt::format("G_DL_OTR_HASH (branch): {}", fileName), subGfx);
                    return;
                }

                break;
            }

            case OTR_G_DL_INDEX: {
                uint8_t segNum = (uint8_t)(cmd->words.w1 >> 24);
                uint32_t index = (uint32_t)(cmd->words.w1 & 0x00FFFFFF);
                uintptr_t segAddr = (segNum << 24) | (index * sizeof(F3DGfx)) + 1;
                F3DGfx* subGFX = (F3DGfx*)seg_addr(segAddr);

                if (C0(16, 1) == 0) {
                    nodeWithText(cmd0, fmt::format("G_DL_INDEX: 0x{:x} -> {}", segAddr, (void*)subGFX), subGFX);
                    cmd++;
                } else {
                    nodeWithText(cmd0, fmt::format("G_DL_INDEX (branch): 0x{:x} -> {}", segAddr, (void*)subGFX),
                                 subGFX);
                    return;
                }

                break;
            }

            case OTR_G_BRANCH_Z_OTR: {
                uint8_t vbidx = (uint8_t)(cmd->words.w0 & 0x00000FFF);
                uint32_t zval = (uint32_t)(cmd->words.w1);

                cmd++;

                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                F3DGfx* subGfx = (F3DGfx*)ResourceGetDataByCrc(hash);
                const char* dlName = ResourceGetNameByCrc(hash);

                if (!dlName) {
                    dlName = "UNKNOWN";
                }

                // TODO: Figure out the vertex value at the time this command would have run, since debugger display is
                // not in sync with renderer execution.
                // if (subGfx && (g_rsp.loaded_vertices[vbidx].z <= zval ||
                //                (g_rsp.extra_geometry_mode & G_EX_ALWAYS_EXECUTE_BRANCH) != 0)) {
                if (subGfx) {
                    nodeWithText(cmd0, fmt::format("G_BRANCH_Z_OTR: zval {}, vIdx {}, DL {}", zval, vbidx, dlName),
                                 subGfx);
                } else {
                    nodeWithText(cmd0, fmt::format("G_BRANCH_Z_OTR: zval {}, vIdx {}, DL {}", zval, vbidx, dlName));
                }

                cmd++;
                break;
            }

            case OTR_G_SETTIMG_OTR_HASH: {
                cmd++;
                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                const char* name = ResourceGetNameByCrc(hash);
                if (!name) {
                    name = "UNKNOWN";
                }

                nodeWithText(cmd0, fmt::format("G_SETTIMG_OTR_HASH: {}", name));

                cmd++;
                break;
            }

            case OTR_G_SETTIMG_OTR_FILEPATH: {
                const char* fileName = (char*)cmd->words.w1;
                nodeWithText(cmd0, fmt::format("G_SETTIMG_OTR_FILEPATH: {}", fileName));

                cmd++;
                break;
            }

            case OTR_G_VTX_OTR_HASH: {
                cmd++;
                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                const char* name = ResourceGetNameByCrc(hash);
                if (!name) {
                    name = "UNKNOWN";
                }

                nodeWithText(cmd0, fmt::format("G_VTX_OTR_HASH: {}", name));

                cmd++;
                break;
            };

            case OTR_G_VTX_OTR_FILEPATH: {
                const char* fileName = (char*)cmd->words.w1;
                nodeWithText(cmd0, fmt::format("G_VTX_OTR_FILEPATH: {}", fileName));

                cmd += 2;
                break;
            }

            case OTR_G_MTX_OTR: {
                cmd++;
                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                const char* name = ResourceGetNameByCrc(hash);
                if (!name) {
                    name = "UNKNOWN";
                }

                nodeWithText(cmd0, fmt::format("G_MTX_OTR: {}", name));

                cmd++;
                break;
            }

            case OTR_G_MTX_OTR_FILEPATH: {
                const char* fileName = (char*)cmd->words.w1;
                nodeWithText(cmd0, fmt::format("G_MTX_OTR_FILEPATH: {}", fileName));

                cmd++;
                break;
            }

            case OTR_G_MOVEMEM_HASH: {
                const uint8_t index = C1(24, 8);
                const uint8_t offset = C1(16, 8);
                cmd++;
                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                const char* name = ResourceGetNameByCrc(hash);
                if (!name) {
                    name = "UNKNOWN";
                }

                nodeWithText(cmd0, fmt::format("G_MOVEMEM_HASH: idx {}, offset {}, {}", index, offset, name));

                cmd++;
                break;
            }

            case OTR_G_IMAGERECT: {
                nodeWithText(cmd0, fmt::format("G_IMAGERECT"));
                cmd += 3;
                break;
            }

            case OTR_G_TRI1_OTR: {
                nodeWithText(cmd0, fmt::format("G_TRI1_OTR: v00 {}, v01 {}, v02 {}", C0(0, 16), C1(16, 16), C1(0, 16)));
                cmd++;
                break;
            }

            case OTR_G_PUSHCD: {
                nodeWithText(cmd0, fmt::format("G_PUSHCD: filename {}", cmd->words.w1));
                cmd++;
                break;
            }

            case OTR_G_INVALTEXCACHE: {
                const char* texAddr = (const char*)cmd->words.w1;
                if (texAddr == 0) {
                    nodeWithText(cmd0, fmt::format("G_INVALTEXCACHE: clear all entries"));
                } else {
                    if (((uintptr_t)texAddr & 1) == 0 &&
                        Ship::Context::GetInstance()->GetResourceManager()->OtrSignatureCheck(texAddr)) {
                        nodeWithText(cmd0, fmt::format("G_INVALTEXCACHE: {}", texAddr));
                    } else {
                        nodeWithText(cmd0, fmt::format("G_INVALTEXCACHE: 0x{:x}", (uintptr_t)texAddr));
                    }
                }

                cmd++;
                break;
            }

            case OTR_G_SETTIMG_FB: {
                nodeWithText(cmd0, fmt::format("G_SETTIMG_FB: src FB {}", (int32_t)cmd->words.w1));
                cmd++;
                break;
            }

            case OTR_G_COPYFB: {
                nodeWithText(cmd0, fmt::format("G_COPYFB: src FB {}, dest FB {}, new frames only {}", C0(0, 11),
                                               C0(11, 11), C0(22, 1)));
                cmd++;
                break;
            }

            case OTR_G_SETFB: {
                nodeWithText(cmd0, fmt::format("G_SETFB: src FB {}", (int32_t)cmd->words.w1));
                cmd++;
                break;
            }

            case OTR_G_RESETFB: {
                nodeWithText(cmd0, fmt::format("G_RESETFB"));
                cmd++;
                break;
            }

            case OTR_G_READFB: {
                int fbId = C0(0, 8);
                bool bswap = C0(8, 1);
                cmd++;
                nodeWithText(cmd0, fmt::format("G_READFB: src FB {}, byteswap {}, ulx {}, uly {}, width {}, height {}",
                                               fbId, bswap, C0(0, 16), C0(16, 16), C1(0, 16), C1(16, 16)));
                cmd++;
                break;
            }

            case OTR_G_TEXRECT_WIDE: {
                int32_t lrx = static_cast<int32_t>((C0(0, 24) << 8)) >> 8;
                int32_t lry = static_cast<int32_t>((C1(0, 24) << 8)) >> 8;
                int32_t tile = C1(24, 3);
                cmd++;
                uint32_t ulx = static_cast<int32_t>((C0(0, 24) << 8)) >> 8;
                uint32_t uly = static_cast<int32_t>((C1(0, 24) << 8)) >> 8;
                cmd++;
                uint32_t uls = C0(16, 16);
                uint32_t ult = C0(0, 16);
                uint32_t dsdx = C1(16, 16);
                uint32_t dtdy = C1(0, 16);

                nodeWithText(
                    cmd0,
                    fmt::format("G_TEXRECT_WIDE: ulx {}, uly {}, lrx {}, lry {}, tile {}, s {}, t {}, dsdx {}, dtdy {}",
                                ulx, uly, lrx, lry, uls, tile, ult, dsdx, dtdy));

                cmd++;
                break;
            }

            case OTR_G_FILLWIDERECT: {
                int32_t lrx = (int32_t)(C0(0, 24) << 8) >> 8;
                int32_t lry = (int32_t)(C1(0, 24) << 8) >> 8;
                cmd++;
                int32_t ulx = (int32_t)(C0(0, 24) << 8) >> 8;
                int32_t uly = (int32_t)(C1(0, 24) << 8) >> 8;
                nodeWithText(cmd0, fmt::format("G_FILLWIDERECT: ulx {}, uly {}, lrx {}, lry {}", ulx, uly, lrx, lry));

                cmd++;
                break;
            }

            case OTR_G_SETGRAYSCALE: {
                nodeWithText(cmd0, fmt::format("G_SETGRAYSCALE: Enable {}", (uint32_t)cmd->words.w1));
                cmd++;
                break;
            }

            case OTR_G_SETINTENSITY: {
                nodeWithText(cmd0, fmt::format("G_SETINTENSITY: red {}, green {}, blue {}, alpha {}", C1(24, 8),
                                               C1(16, 8), C1(8, 8), C1(0, 8)));
                cmd++;
                break;
            }

            case OTR_G_EXTRAGEOMETRYMODE: {
                uint32_t setBits = (uint32_t)cmd->words.w1;
                uint32_t clearBits = ~C0(0, 24);
                nodeWithText(cmd0, fmt::format("G_EXTRAGEOMETRYMODE: Set {}, Clear {}", setBits, clearBits));
                cmd++;
                break;
            }

            case OTR_G_REGBLENDEDTEX: {
                const char* timg = (const char*)cmd->words.w1;
                cmd++;

                uint8_t* mask = (uint8_t*)cmd->words.w0;
                uint8_t* replacementTex = (uint8_t*)cmd->words.w1;

                if (Ship::Context::GetInstance()->GetResourceManager()->OtrSignatureCheck(timg)) {
                    timg += 7;
                    nodeWithText(cmd0, fmt::format("G_REGBLENDEDTEX: src {}, mask {}, blended {}", timg, (void*)mask,
                                                   (void*)replacementTex));
                } else {
                    nodeWithText(cmd0, fmt::format("G_REGBLENDEDTEX: src {}, mask {}, blended {}", (void*)timg,
                                                   (void*)mask, (void*)replacementTex));
                }

                cmd++;
                break;
            }

            default: {
                simpleNode(cmd, opcode);
                cmd++;
                break;
            }
        }
    }
}

static const char* getTexType(LUS::TextureType type) {
    switch (type) {

        case LUS::TextureType::RGBA32bpp:
            return "RGBA32";
        case LUS::TextureType::RGBA16bpp:
            return "RGBA16";
        case LUS::TextureType::Palette4bpp:
            return "CI4";
        case LUS::TextureType::Palette8bpp:
            return "CI8";
        case LUS::TextureType::Grayscale4bpp:
            return "I4";
        case LUS::TextureType::Grayscale8bpp:
            return "I8";
        case LUS::TextureType::GrayscaleAlpha4bpp:
            return "IA4";
        case LUS::TextureType::GrayscaleAlpha8bpp:
            return "IA8";
        case LUS::TextureType::GrayscaleAlpha16bpp:
            return "IA16";
        default:
            return "UNKNOWN";
    }
}

static bool bpEquals(const std::vector<const F3DGfx*>& x, const std::vector<const F3DGfx*>& y) {
    if (x.size() != y.size())
        return false;

    for (size_t i = 0; i < x.size(); i++) {
        if (x[i] != y[i]) {
            return false;
        }
    }

    return true;
}

// todo: __asan_on_error

void GfxDebuggerWindow::DrawDisas() {

    auto dbg = Ship::Context::GetInstance()->GetGfxDebugger();
    auto dlist = dbg->GetDisplayList();
    ImGui::Text("dlist: %p", dlist);
    std::string bp = "";
    for (auto& gfx : dbg->GetBreakPoint()) {
        bp += fmt::format("/{}", (const void*)gfx);
    }
    ImGui::Text("BreakPoint: %s", bp.c_str());

    bool isNew = !bpEquals(mLastBreakPoint, dbg->GetBreakPoint());
    if (isNew) {
        mLastBreakPoint = dbg->GetBreakPoint();
        // fprintf(stderr, "NEW BREAKPOINT %s\n", bp.c_str());
    }

    std::string TO_LOAD_TEX = "GfxDebuggerWindowTextureToLoad";

    const F3DGfx* cmd = dlist;
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();

    ImGui::BeginChild("###State", ImVec2(0.0f, 200.0f), true);
    {
        ImGui::BeginGroup();
        {
            ImGui::Text("Disp Stack");
            ImGui::BeginChild("### Disp Stack", ImVec2(400.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (auto& disp : g_exec_stack.disp_stack) {
                ImGui::Text("%s", fmt::format("{}:{}", disp.file, disp.line).c_str());
            }
            ImGui::EndChild();
        }
        ImGui::EndGroup();
        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Text("Tiles");
            ImGui::BeginChild("### Tile", ImVec2(400.0f, 0.0f), true);
            // for (size_t i = 0; i < 8; i++) {
            //     auto& tile = g_rdp.texture_tile[i];
            //     ImGui::Text(
            //         "%s", fmt::format("{}: fmt={}; siz={}; cms={}; cmt={};", i, tile.fmt, tile.siz, tile.cms,
            //         tile.cmt)
            //                   .c_str());
            // }

            auto draw_img = [isNew, &gui](std::optional<std::string> prefix, const std::string& name,
                                          const RawTexMetadata& metadata) {
                if (prefix) {
                    ImGui::Text("%s: %dx%d; type=%s", prefix->c_str(), metadata.width, metadata.height,
                                getTexType(metadata.type));
                } else {
                    ImGui::Text("%dx%d; type=%s", metadata.width, metadata.height, getTexType(metadata.type));
                }

                if (isNew && metadata.resource != nullptr) {
                    gui->UnloadTexture(name);
                    gui->LoadGuiTexture(name, *metadata.resource, ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
                }

                ImGui::Image(gui->GetTextureByName(name), ImVec2{ 100.0f, 100.0f });
            };

            ImGui::Text("Loaded Textures");
            for (size_t i = 0; i < 2; i++) {
                auto& tex = g_rdp.loaded_texture[i];
                // ImGui::Text("%s", fmt::format("{}: {}x{} type={}", i, tex.raw_tex_metadata.width,
                //                               tex.raw_tex_metadata.height, getTexType(tex.raw_tex_metadata.type))
                //                       .c_str());
                draw_img(std::to_string(i), fmt::format("GfxDebuggerWindowLoadedTexture{}", i), tex.raw_tex_metadata);
            }
            ImGui::Text("Texture To Load");
            {
                auto& tex = g_rdp.texture_to_load;
                // ImGui::Text("%s", fmt::format("{}x{} type={}", tex.raw_tex_metadata.width,
                // tex.raw_tex_metadata.height,
                //                               getTexType(tex.raw_tex_metadata.type))
                //                       .c_str());

                // if (isNew && g_rdp.texture_to_load.raw_tex_metadata.resource != nullptr) {
                //     gui->UnloadTexture(TO_LOAD_TEX);
                //     gui->LoadGuiTexture(TO_LOAD_TEX, *g_rdp.texture_to_load.raw_tex_metadata.resource,
                //                         ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f });
                // }

                // ImGui::Image(gui->GetTextureByName(TO_LOAD_TEX), ImVec2{ 100.0f, 100.0f });
                draw_img(std::nullopt, TO_LOAD_TEX, tex.raw_tex_metadata);
            }
            ImGui::EndChild();
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            auto showColor = [](const char* text, RGBA c) {
                float cf[] = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f };
                ImGui::ColorEdit3(text, cf, ImGuiColorEditFlags_NoInputs);
            };

            showColor("Env Color", g_rdp.env_color);
            showColor("Prim Color", g_rdp.prim_color);
            showColor("Fog Color", g_rdp.fog_color);
            showColor("Fill Color", g_rdp.fill_color);
            showColor("Grayscale Color", g_rdp.grayscale_color);
        }

        ImGui::EndGroup();
    }
    ImGui::EndChild();

    ImGui::BeginChild("##Disassembler", ImVec2(0.0f, 0.0f), true);
    {
        std::vector<const F3DGfx*> gfxPath;
        DrawDisasNode(dlist, gfxPath);
    }
    ImGui::EndChild();
}

void GfxDebuggerWindow::DrawElement() {
    auto dbg = Ship::Context::GetInstance()->GetGfxDebugger();
    // const ImVec2 pos = ImGui::GetWindowPos();
    // const ImVec2 size = ImGui::GetWindowSize();

    if (!dbg->IsDebugging()) {
        if (ImGui::Button("Debug")) {
            dbg->RequestDebugging();
        }
    } else {
        bool resumed = false;
        if (ImGui::Button("Resume Game")) {
            dbg->ResumeGame();
            resumed = true;
        }

        if (!resumed) {
            DrawDisas();
        }
    }
}

} // namespace LUS
