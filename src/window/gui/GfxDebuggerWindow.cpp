#include "GfxDebuggerWindow.h"
#include <ImGui/imgui.h>
#include <spdlog/spdlog.h>
#include "Context.h"
#include "debug/GfxDebugger.h"
#include <stack>
#include <spdlog/fmt/fmt.h>
#include "libultraship/bridge.h"
#include <graphic/Fast3D/gfx_pc.h>
#ifdef GFX_DEBUG_DISASSEMBLER
#include <gfxd.h>
#endif

extern uintptr_t gSegmentPointers[16];

namespace LUS {

GfxDebuggerWindow::~GfxDebuggerWindow() {
}

void GfxDebuggerWindow::InitElement() {
}

void GfxDebuggerWindow::UpdateElement() {
}

static const char* GetOpName(uint32_t op) {
    switch (op) {
#define CASE(x) \
    case x:     \
        return #x

        CASE(G_RDPHALF_1);
        CASE(G_RDPHALF_2);
        CASE(G_RDPPIPESYNC);
        CASE(G_RDPFULLSYNC);
        CASE(G_RDPLOADSYNC);
        CASE(G_LOAD_UCODE);
        CASE(G_MARKER);
        CASE(G_INVALTEXCACHE);
        CASE(G_NOOP);
        CASE(G_MTX);
        CASE(G_MTX_OTR);
        CASE(G_POPMTX);
        CASE(G_MOVEMEM);
        CASE(G_MOVEWORD);
        CASE(G_TEXTURE);
        CASE(G_VTX);
        CASE(G_VTX_OTR_HASH);
        CASE(G_VTX_OTR_FILEPATH);
        CASE(G_DL_OTR_FILEPATH);
        CASE(G_MODIFYVTX);
        CASE(G_DL);
        CASE(G_DL_OTR_HASH);
        CASE(G_PUSHCD);
        CASE(G_BRANCH_Z_OTR);
        CASE(G_ENDDL);
        CASE(G_GEOMETRYMODE);
        // CASE(G_SETGEOMETRYMODE);
        // CASE(G_CLEARGEOMETRYMODE);
        CASE(G_TRI1_OTR);
        CASE(G_TRI1);
        CASE(G_QUAD);
        CASE(G_TRI2);
        CASE(G_SETOTHERMODE_L);
        CASE(G_SETOTHERMODE_H);
        CASE(G_SETTIMG);
        CASE(G_SETTIMG_OTR_HASH);
        CASE(G_SETTIMG_OTR_FILEPATH);
        CASE(G_SETFB);
        CASE(G_RESETFB);
        CASE(G_SETTIMG_FB);
        CASE(G_SETGRAYSCALE);
        CASE(G_LOADBLOCK);
        CASE(G_LOADTILE);
        CASE(G_SETTILE);
        CASE(G_SETTILESIZE);
        CASE(G_LOADTLUT);
        CASE(G_SETENVCOLOR);
        CASE(G_SETPRIMCOLOR);
        CASE(G_SETFOGCOLOR);
        CASE(G_SETFILLCOLOR);
        CASE(G_SETINTENSITY);
        CASE(G_SETCOMBINE);
        CASE(G_TEXRECT);
        CASE(G_TEXRECTFLIP);
        CASE(G_TEXRECT_WIDE);
        CASE(G_FILLRECT);
        CASE(G_FILLWIDERECT);
        CASE(G_SETSCISSOR);
        CASE(G_SETZIMG);
        CASE(G_SETCIMG);
        CASE(G_RDPSETOTHERMODE);
        // CASE(G_BG_COPY);
        CASE(G_EXTRAGEOMETRYMODE);
        CASE(G_SETBLENDCOLOR);
        CASE(G_RDPTILESYNC);
        CASE(G_SPNOOP);
        CASE(G_CULLDL);
        CASE(G_IMAGERECT);
        CASE(G_COPYFB);
        default:
            return nullptr;
            // return "UNKNOWN";
#undef CASE
    }
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
void GfxDebuggerWindow::DrawDisasNode(const Gfx* cmd, std::vector<const Gfx*>& gfx_path) const {
    auto dbg = LUS::Context::GetInstance()->GetGfxDebugger();

    auto node_with_text = [dbg, this, &gfx_path](const Gfx* cmd, const std::string& text,
                                                 const Gfx* sub = nullptr) mutable {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

        gfx_path.push_back(cmd);
        if (dbg->HasBreakPoint(gfx_path))
            flags |= ImGuiTreeNodeFlags_Selected;
        if (sub == nullptr)
            flags |= ImGuiTreeNodeFlags_Leaf;

        bool open = ImGui::TreeNodeEx((const void*)cmd, flags, "%p: %s", cmd, text.c_str());
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            dbg->SetBreakPoint(gfx_path);
        }
        if (open) {
            if (sub) {
                DrawDisasNode(sub, gfx_path);
            }
            ImGui::TreePop();
        }
        gfx_path.pop_back();
    };

    auto simple_node = [dbg, node_with_text](const Gfx* cmd, uint32_t opcode) mutable {
        const char* opname = GetOpName(opcode);
        size_t size = 1;
        if (opcode == G_TEXRECT)
            size = 3;
        if (opname) {
#ifdef GFX_DEBUG_DISASSEMBLER
            // Our Gfx uses uinptr_t for words, but libgfxd uses uint32_t,
            // Copy only the first 32bits of each word into a vector before passing the instructions
            std::vector<uint32_t> input;
            for (size_t i = 0; i < size; i++) {
                input.push_back(cmd[i].words.w0 & 0xFFFFFFFF);
                input.push_back(cmd[i].words.w1 & 0xFFFFFFFF);
            }

            gfxd_input_buffer(input.data(), sizeof(uint32_t) * size * 2);
            gfxd_endian(gfxd_endian_host, sizeof(uint32_t));
            char buff[256] = { 0 };
            gfxd_output_buffer(buff, sizeof(buff));
            gfxd_enable(gfxd_emit_dec_color);
            gfxd_target(gfxd_f3dex2);
            gfxd_execute();

            node_with_text(cmd, fmt::format("{}", buff));
#else
            node_with_text(cmd, fmt::format("{}", opname));
#endif
        } else {
            uint32_t opcode = cmd->words.w0 >> 24;
            node_with_text(cmd, fmt::format("UNK: 0x{:X}", opcode));
        }
    };

    while (true) {
        uint32_t opcode = cmd->words.w0 >> 24;
        const Gfx* cmd0 = cmd;
        switch (opcode) {

            case G_NOOP: {
                const char* filename = (const char*)cmd->words.w1;
                uint32_t p = C0(16, 8);
                uint32_t l = C0(0, 16);

                if (p == 7) {
                    node_with_text(cmd0, fmt::format("gDPNoOpOpenDisp(): {}:{}", filename, l));
                } else if (p == 8) {
                    node_with_text(cmd0, fmt::format("gDPNoOpCloseDisp() {}:{}", filename, l));
                } else {
                    node_with_text(cmd0, fmt::format("G_NOOP: {}", p));
                }

                cmd++;
                break;
            }

            case G_MARKER: {
                cmd++;
                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                const char* dlName = ResourceGetNameByCrc(hash);
                if (!dlName)
                    dlName = "UNKNOWN";

                node_with_text(cmd0, fmt::format("G_MARKER: {}", dlName));
                cmd++;
                break;
            }

            case G_DL_OTR_HASH: {

                if (C0(16, 1) == 0) {
                    cmd++;
                    uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                    const char* dlName = ResourceGetNameByCrc(hash);
                    if (!dlName)
                        dlName = "UNKNOWN";

                    Gfx* subGfx = (Gfx*)ResourceGetDataByCrc(hash);
                    // node_with_text(cmd0, fmt::format("G_DL_OTR_HASH: {}", "UNKOWN"), subGfx);
                    node_with_text(cmd0, fmt::format("G_DL_OTR_HASH: {}", dlName), subGfx);
                    cmd++;
                } else {
                    assert(0 && "Invalid in gfx_pc????");
                }
                break;
            }

            case G_SETTIMG_OTR_HASH: {
                cmd++;
                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                const char* name = ResourceGetNameByCrc(hash);
                if (!name)
                    name = "UNKNOWN";

                node_with_text(cmd0, fmt::format("G_SETTIMG_OTR_HASH: {}", name));

                // std::shared_ptr<LUS::Texture> texture = std::static_pointer_cast<LUS::Texture>(
                //     LUS::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(ResourceGetNameByCrc(hash)));
                cmd++;
                break;
            }

            case G_VTX_OTR_HASH: {
                cmd++;
                uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
                const char* name = ResourceGetNameByCrc(hash);
                if (!name)
                    name = "UNKNOWN";

                node_with_text(cmd0, fmt::format("G_VTX_OTR_HASH: {}", name));

                // Vtx* vtx = (Vtx*)ResourceGetDataByCrc(hash);
                cmd++;
                break;
            };

            case G_DL: {
                Gfx* subGFX = (Gfx*)seg_addr(cmd->words.w1);
                if (C0(16, 1) == 0) {
                    node_with_text(cmd0, fmt::format("G_DL: 0x{:x} -> {}", cmd->words.w1, (void*)subGFX), subGFX);
                    cmd++;
                } else {
                    node_with_text(cmd0, fmt::format("G_DL (branch): 0x{:x} -> {}", cmd->words.w1, (void*)subGFX),
                                   subGFX);
                    return;
                }
                break;
            }

            case G_ENDDL: {
                simple_node(cmd, opcode);
                return;
            }

            case G_TEXRECT: {
                simple_node(cmd, opcode);
                cmd += 3;
                break;
            }

            case G_IMAGERECT: {
                node_with_text(cmd0, fmt::format("G_IMAGERECT"));
                cmd += 3;
                break;
            }

            case G_SETTIMG_FB: {
                node_with_text(cmd0, fmt::format("G_SETTIMG_FB: src {}", cmd->words.w1));
                cmd++;
                break;
            }

            case G_COPYFB: {
                node_with_text(cmd0, fmt::format("G_COPYFB: src {}, dest {}, new frames only {}", C0(0, 11), C0(11, 11),
                                                 C0(22, 1)));
                cmd++;
                break;
            }

            default: {
                simple_node(cmd, opcode);
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

static bool bpEquals(const std::vector<const Gfx*>& x, const std::vector<const Gfx*>& y) {
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

    auto dbg = LUS::Context::GetInstance()->GetGfxDebugger();
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

    const Gfx* cmd = dlist;
    auto gui = LUS::Context::GetInstance()->GetWindow()->GetGui();

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
        std::vector<const Gfx*> gfx_path;
        DrawDisasNode(dlist, gfx_path);
    }
    ImGui::EndChild();
}

void GfxDebuggerWindow::DrawElement() {
    auto dbg = LUS::Context::GetInstance()->GetGfxDebugger();

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("GFX Debugger", &mIsVisible, ImGuiWindowFlags_NoFocusOnAppearing);
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

    ImGui::End();
}

} // namespace LUS
