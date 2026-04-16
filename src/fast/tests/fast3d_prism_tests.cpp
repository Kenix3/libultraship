// Fast3D Prism Shader Processor Tests
//
// Tests cover:
//  - Basic prism::Processor template variable substitution
//  - Conditional blocks (@if/@else/@end)
//  - For-loop iteration (@for)
//  - Boolean context variables
//  - Integer context variables
//  - Array context variables (MTDArray)
//  - Include loader binding
//  - Invocable functions (InvokeFunc)
//  - append_formula-style shader generation patterns
//  - Interaction with CCFeatures struct for shader config

#include <gtest/gtest.h>
#include <string>
#include <optional>

#include <prism/processor.h>
#include "fast/interpreter.h"

// ============================================================
// Basic Prism Processor Tests
// ============================================================

TEST(PrismProcessor, SimpleVariableSubstitution) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "name", std::string("Fast3D") },
    };
    processor.populate(ctx);
    processor.load("Hello @{name}!");
    std::string result = processor.process();
    EXPECT_NE(result.find("Hello Fast3D!"), std::string::npos);
}

TEST(PrismProcessor, IntegerVariableSubstitution) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "count", 42 },
    };
    processor.populate(ctx);
    processor.load("Count is @{count}.");
    std::string result = processor.process();
    EXPECT_NE(result.find("Count is 42."), std::string::npos);
}

TEST(PrismProcessor, BooleanVariable_True) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "enabled", true },
    };
    processor.populate(ctx);
    processor.load("@if(enabled)YES@end");
    std::string result = processor.process();
    EXPECT_NE(result.find("YES"), std::string::npos);
}

TEST(PrismProcessor, BooleanVariable_False) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "enabled", false },
    };
    processor.populate(ctx);
    processor.load("@if(enabled)YES@end");
    std::string result = processor.process();
    EXPECT_EQ(result.find("YES"), std::string::npos);
}

TEST(PrismProcessor, IfElseBlock) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "debug", false },
    };
    processor.populate(ctx);
    processor.load("@if(debug)DEBUG@else RELEASE@end");
    std::string result = processor.process();
    EXPECT_EQ(result.find("DEBUG"), std::string::npos);
    EXPECT_NE(result.find("RELEASE"), std::string::npos);
}

TEST(PrismProcessor, IfElseBlock_TrueBranch) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "debug", true },
    };
    processor.populate(ctx);
    processor.load("@if(debug)DEBUG@else RELEASE@end");
    std::string result = processor.process();
    EXPECT_NE(result.find("DEBUG"), std::string::npos);
}

TEST(PrismProcessor, ForLoop_Range) {
    prism::Processor processor;
    prism::ContextItems ctx = {};
    processor.populate(ctx);
    processor.load("@for(i in 0..3)[@{i}]@end");
    std::string result = processor.process();
    EXPECT_NE(result.find("[0]"), std::string::npos);
    EXPECT_NE(result.find("[1]"), std::string::npos);
    EXPECT_NE(result.find("[2]"), std::string::npos);
    // 0..3 generates 0, 1, 2 (exclusive end)
    EXPECT_EQ(result.find("[3]"), std::string::npos);
}

TEST(PrismProcessor, ForLoop_WithVariable) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "max", 2 },
    };
    processor.populate(ctx);
    processor.load("@for(i in 0..max)X@{i} @end");
    std::string result = processor.process();
    EXPECT_NE(result.find("X0"), std::string::npos);
    EXPECT_NE(result.find("X1"), std::string::npos);
    EXPECT_EQ(result.find("X2"), std::string::npos);
}

TEST(PrismProcessor, NestedIfInFor) {
    bool flags[3] = { true, false, true };
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "flags", M_ARRAY(flags, bool, 3) },
    };
    processor.populate(ctx);
    processor.load("@for(i in 0..3)@if(flags[i])Y@{i}@end @end");
    std::string result = processor.process();
    EXPECT_NE(result.find("Y0"), std::string::npos);
    EXPECT_EQ(result.find("Y1"), std::string::npos);
    EXPECT_NE(result.find("Y2"), std::string::npos);
}

TEST(PrismProcessor, IntegerComparison) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "val", 5 },
    };
    processor.populate(ctx);
    processor.load("@if(val == 5)FIVE@end");
    std::string result = processor.process();
    EXPECT_NE(result.find("FIVE"), std::string::npos);
}

TEST(PrismProcessor, IntegerComparison_NotEqual) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "val", 3 },
    };
    processor.populate(ctx);
    processor.load("@if(val == 5)FIVE@end");
    std::string result = processor.process();
    EXPECT_EQ(result.find("FIVE"), std::string::npos);
}

TEST(PrismProcessor, StringVariableInTemplate) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "version", std::string("#version 130") },
    };
    processor.populate(ctx);
    processor.load("@{version}\nvoid main() {}");
    std::string result = processor.process();
    EXPECT_NE(result.find("#version 130"), std::string::npos);
    EXPECT_NE(result.find("void main() {}"), std::string::npos);
}

TEST(PrismProcessor, EmptyTemplate) {
    prism::Processor processor;
    prism::ContextItems ctx = {};
    processor.populate(ctx);
    processor.load("");
    std::string result = processor.process();
    EXPECT_TRUE(result.empty() || result.find_first_not_of(" \n\r\t") == std::string::npos);
}

TEST(PrismProcessor, PlainTextPassthrough) {
    prism::Processor processor;
    prism::ContextItems ctx = {};
    processor.populate(ctx);
    processor.load("plain text with no directives");
    std::string result = processor.process();
    EXPECT_NE(result.find("plain text with no directives"), std::string::npos);
}

// ============================================================
// Include Loader Tests
// ============================================================

TEST(PrismProcessor, IncludeLoader_BasicInclude) {
    prism::Processor processor;
    prism::ContextItems ctx = {};
    processor.populate(ctx);
    processor.load("@include(\"test_include\")");
    processor.bind_include_loader([](const std::string& path) -> std::optional<std::string> {
        if (path == "test_include") {
            return "INCLUDED_CONTENT";
        }
        return std::nullopt;
    });
    std::string result = processor.process();
    EXPECT_NE(result.find("INCLUDED_CONTENT"), std::string::npos);
}

TEST(PrismProcessor, IncludeLoader_NotFound_ReturnsEmpty) {
    prism::Processor processor;
    prism::ContextItems ctx = {};
    processor.populate(ctx);
    processor.load("BEFORE@include(\"nonexistent\")AFTER");
    processor.bind_include_loader([](const std::string&) -> std::optional<std::string> {
        return std::nullopt;
    });
    std::string result = processor.process();
    EXPECT_NE(result.find("BEFORE"), std::string::npos);
    EXPECT_NE(result.find("AFTER"), std::string::npos);
}

// ============================================================
// InvokeFunc Tests (custom callable functions)
// ============================================================

static prism::ContextTypes* testAddFunc(prism::ContextTypes* _, prism::ContextTypes* a, prism::ContextTypes* b) {
    int va = std::get<int>(*a);
    int vb = std::get<int>(*b);
    return new prism::ContextTypes{ va + vb };
}

TEST(PrismProcessor, InvokeFunc_BasicCall) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "add", (InvokeFunc)testAddFunc },
    };
    processor.populate(ctx);
    processor.load("Result: @{add(3, 4)}");
    std::string result = processor.process();
    EXPECT_NE(result.find("Result: 7"), std::string::npos);
}

static prism::ContextTypes* testStringFunc(prism::ContextTypes* _, prism::ContextTypes* val) {
    int v = std::get<int>(*val);
    return new prism::ContextTypes{ std::string("item_") + std::to_string(v) };
}

TEST(PrismProcessor, InvokeFunc_ReturnsString) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "make_name", (InvokeFunc)testStringFunc },
    };
    processor.populate(ctx);
    processor.load("Name: @{make_name(5)}");
    std::string result = processor.process();
    EXPECT_NE(result.find("Name: item_5"), std::string::npos);
}

// ============================================================
// MTDArray Tests (multi-dimensional array access)
// ============================================================

TEST(PrismProcessor, MTDArray_1D_BoolAccess) {
    bool arr[3] = { true, false, true };
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "arr", M_ARRAY(arr, bool, 3) },
    };
    processor.populate(ctx);
    processor.load("@if(arr[0])A@end @if(arr[1])B@end @if(arr[2])C@end");
    std::string result = processor.process();
    EXPECT_NE(result.find("A"), std::string::npos);
    EXPECT_EQ(result.find("B"), std::string::npos);
    EXPECT_NE(result.find("C"), std::string::npos);
}

TEST(PrismProcessor, MTDArray_2D_BoolAccess) {
    bool arr[2][2] = { { true, false }, { false, true } };
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "grid", M_ARRAY(arr, bool, 2, 2) },
    };
    processor.populate(ctx);
    processor.load("@if(grid[0][0])TL@end @if(grid[0][1])TR@end @if(grid[1][0])BL@end @if(grid[1][1])BR@end");
    std::string result = processor.process();
    EXPECT_NE(result.find("TL"), std::string::npos);
    EXPECT_EQ(result.find("TR"), std::string::npos);
    EXPECT_EQ(result.find("BL"), std::string::npos);
    EXPECT_NE(result.find("BR"), std::string::npos);
}

TEST(PrismProcessor, MTDArray_IntAccess) {
    int arr[4] = { 10, 20, 30, 40 };
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "vals", M_ARRAY(arr, int, 4) },
    };
    processor.populate(ctx);
    processor.load("@{vals[2]}");
    std::string result = processor.process();
    EXPECT_NE(result.find("30"), std::string::npos);
}

// ============================================================
// Shader-Pattern Tests (simulating shader generation patterns)
// ============================================================

TEST(PrismShaderPattern, VertexShader_TextureAttribs) {
    // Simulate the vertex shader pattern used by Fast3D OpenGL backend
    bool textures[2] = { true, false };
    bool clamp[2][2] = { { true, false }, { false, false } };

    prism::Processor processor;
    prism::ContextItems ctx = {
        { "VERTEX_SHADER", true },
        { "o_textures", M_ARRAY(textures, bool, 2) },
        { "o_clamp", M_ARRAY(clamp, bool, 2, 2) },
        { "o_fog", false },
        { "o_grayscale", false },
        { "o_alpha", false },
        { "o_inputs", 1 },
    };
    processor.populate(ctx);

    // Simplified version of the vertex shader template
    std::string tmpl =
        "@if(VERTEX_SHADER)\n"
        "@for(i in 0..2)\n"
        "@if(o_textures[i])\n"
        "attr vec2 aTexCoord@{i};\n"
        "@for(j in 0..2)\n"
        "@if(o_clamp[i][j])\n"
        "@if(j == 0)\n"
        "attr float aTexClampS@{i};\n"
        "@else\n"
        "attr float aTexClampT@{i};\n"
        "@end\n"
        "@end\n"
        "@end\n"
        "@end\n"
        "@end\n"
        "@if(o_fog)\n"
        "attr vec4 aFog;\n"
        "@end\n"
        "@for(i in 0..o_inputs)\n"
        "@if(o_alpha)\n"
        "attr vec4 aInput@{i + 1};\n"
        "@else\n"
        "attr vec3 aInput@{i + 1};\n"
        "@end\n"
        "@end\n"
        "@end\n";

    processor.load(tmpl);
    std::string result = processor.process();

    // Texture 0 is enabled → should have aTexCoord0
    EXPECT_NE(result.find("aTexCoord0"), std::string::npos);
    // Texture 1 is disabled → should NOT have aTexCoord1
    EXPECT_EQ(result.find("aTexCoord1"), std::string::npos);
    // Clamp S for texture 0 is enabled → should have aTexClampS0
    EXPECT_NE(result.find("aTexClampS0"), std::string::npos);
    // Clamp T for texture 0 is disabled → should NOT have aTexClampT0
    EXPECT_EQ(result.find("aTexClampT0"), std::string::npos);
    // Fog is disabled → no aFog
    EXPECT_EQ(result.find("aFog"), std::string::npos);
    // 1 input, alpha disabled → vec3 aInput1
    EXPECT_NE(result.find("vec3 aInput1"), std::string::npos);
    EXPECT_EQ(result.find("vec4 aInput1"), std::string::npos);
}

TEST(PrismShaderPattern, VertexShader_WithFogAndAlpha) {
    bool textures[2] = { false, false };
    bool clamp[2][2] = { { false, false }, { false, false } };

    prism::Processor processor;
    prism::ContextItems ctx = {
        { "VERTEX_SHADER", true },
        { "o_textures", M_ARRAY(textures, bool, 2) },
        { "o_clamp", M_ARRAY(clamp, bool, 2, 2) },
        { "o_fog", true },
        { "o_grayscale", false },
        { "o_alpha", true },
        { "o_inputs", 2 },
    };
    processor.populate(ctx);

    std::string tmpl =
        "@if(VERTEX_SHADER)\n"
        "@if(o_fog)\n"
        "attr vec4 aFog;\n"
        "@end\n"
        "@for(i in 0..o_inputs)\n"
        "@if(o_alpha)\n"
        "attr vec4 aInput@{i + 1};\n"
        "@else\n"
        "attr vec3 aInput@{i + 1};\n"
        "@end\n"
        "@end\n"
        "@end\n";

    processor.load(tmpl);
    std::string result = processor.process();

    // Fog enabled
    EXPECT_NE(result.find("aFog"), std::string::npos);
    // 2 inputs with alpha → vec4
    EXPECT_NE(result.find("vec4 aInput1"), std::string::npos);
    EXPECT_NE(result.find("vec4 aInput2"), std::string::npos);
    // No vec3 inputs
    EXPECT_EQ(result.find("vec3 aInput"), std::string::npos);
}

TEST(PrismShaderPattern, FragmentShader_NotVertexShader) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "VERTEX_SHADER", false },
    };
    processor.populate(ctx);

    std::string tmpl =
        "@if(VERTEX_SHADER)\n"
        "VERTEX\n"
        "@else\n"
        "FRAGMENT\n"
        "@end\n";

    processor.load(tmpl);
    std::string result = processor.process();

    EXPECT_EQ(result.find("VERTEX"), std::string::npos);
    EXPECT_NE(result.find("FRAGMENT"), std::string::npos);
}

// ============================================================
// CCFeatures-Driven Shader Pattern Tests
// ============================================================

TEST(PrismShaderPattern, CCFeatures_AllDisabled) {
    // A CCFeatures with everything off should produce minimal shader code
    CCFeatures cc = {};
    memset(&cc, 0, sizeof(cc));

    bool textures[2] = { false, false };
    bool clamp[2][2] = { { false, false }, { false, false } };
    bool masks[2] = { false, false };
    bool blend[2] = { false, false };
    bool do_single[2][2] = { { false, false }, { false, false } };
    bool do_multiply[2][2] = { { false, false }, { false, false } };
    bool do_mix[2][2] = { { false, false }, { false, false } };
    bool color_alpha_same[2] = { false, false };

    prism::Processor processor;
    prism::ContextItems ctx = {
        { "VERTEX_SHADER", true },
        { "o_textures", M_ARRAY(textures, bool, 2) },
        { "o_clamp", M_ARRAY(clamp, bool, 2, 2) },
        { "o_fog", false },
        { "o_grayscale", false },
        { "o_alpha", false },
        { "o_inputs", 0 },
    };
    processor.populate(ctx);

    std::string tmpl =
        "@if(VERTEX_SHADER)\n"
        "@for(i in 0..2)\n"
        "@if(o_textures[i])\n"
        "HAS_TEX@{i}\n"
        "@end\n"
        "@end\n"
        "@if(o_fog)\n"
        "HAS_FOG\n"
        "@end\n"
        "@if(o_grayscale)\n"
        "HAS_GRAYSCALE\n"
        "@end\n"
        "@end\n";

    processor.load(tmpl);
    std::string result = processor.process();

    EXPECT_EQ(result.find("HAS_TEX"), std::string::npos);
    EXPECT_EQ(result.find("HAS_FOG"), std::string::npos);
    EXPECT_EQ(result.find("HAS_GRAYSCALE"), std::string::npos);
}

TEST(PrismShaderPattern, CCFeatures_WithTexturesAndGrayscale) {
    bool textures[2] = { true, true };
    bool clamp[2][2] = { { false, false }, { false, false } };

    prism::Processor processor;
    prism::ContextItems ctx = {
        { "VERTEX_SHADER", true },
        { "o_textures", M_ARRAY(textures, bool, 2) },
        { "o_clamp", M_ARRAY(clamp, bool, 2, 2) },
        { "o_fog", false },
        { "o_grayscale", true },
        { "o_alpha", false },
        { "o_inputs", 0 },
    };
    processor.populate(ctx);

    std::string tmpl =
        "@if(VERTEX_SHADER)\n"
        "@for(i in 0..2)\n"
        "@if(o_textures[i])\n"
        "HAS_TEX@{i}\n"
        "@end\n"
        "@end\n"
        "@if(o_grayscale)\n"
        "HAS_GRAYSCALE\n"
        "@end\n"
        "@end\n";

    processor.load(tmpl);
    std::string result = processor.process();

    EXPECT_NE(result.find("HAS_TEX0"), std::string::npos);
    EXPECT_NE(result.find("HAS_TEX1"), std::string::npos);
    EXPECT_NE(result.find("HAS_GRAYSCALE"), std::string::npos);
}

// ============================================================
// Arithmetic Expression Tests
// ============================================================

TEST(PrismProcessor, ArithmeticInExpression) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "base", 10 },
    };
    processor.populate(ctx);
    processor.load("@{base + 5}");
    std::string result = processor.process();
    EXPECT_NE(result.find("15"), std::string::npos);
}

TEST(PrismProcessor, MultiplicationInExpression) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "x", 3 },
    };
    processor.populate(ctx);
    processor.load("@{x * 4}");
    std::string result = processor.process();
    EXPECT_NE(result.find("12"), std::string::npos);
}

// ============================================================
// ElseIf Tests
// ============================================================

TEST(PrismProcessor, ElseIf_FirstBranch) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "val", 1 },
    };
    processor.populate(ctx);
    processor.load("@if(val == 1)ONE@elseif(val == 2)TWO@else OTHER@end");
    std::string result = processor.process();
    EXPECT_NE(result.find("ONE"), std::string::npos);
    EXPECT_EQ(result.find("TWO"), std::string::npos);
    EXPECT_EQ(result.find("OTHER"), std::string::npos);
}

TEST(PrismProcessor, ElseIf_SecondBranch) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "val", 2 },
    };
    processor.populate(ctx);
    processor.load("@if(val == 1)ONE@elseif(val == 2)TWO@else OTHER@end");
    std::string result = processor.process();
    EXPECT_EQ(result.find("ONE"), std::string::npos);
    EXPECT_NE(result.find("TWO"), std::string::npos);
    EXPECT_EQ(result.find("OTHER"), std::string::npos);
}

TEST(PrismProcessor, ElseIf_ElseBranch) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "val", 99 },
    };
    processor.populate(ctx);
    processor.load("@if(val == 1)ONE@elseif(val == 2)TWO@else OTHER@end");
    std::string result = processor.process();
    EXPECT_EQ(result.find("ONE"), std::string::npos);
    EXPECT_EQ(result.find("TWO"), std::string::npos);
    EXPECT_NE(result.find("OTHER"), std::string::npos);
}

// ============================================================
// Negation / Boolean Logic Tests
// ============================================================

TEST(PrismProcessor, NegatedCondition) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "flag", false },
    };
    processor.populate(ctx);
    processor.load("@if(!flag)NEGATED@end");
    std::string result = processor.process();
    EXPECT_NE(result.find("NEGATED"), std::string::npos);
}

TEST(PrismProcessor, NegatedCondition_True) {
    prism::Processor processor;
    prism::ContextItems ctx = {
        { "flag", true },
    };
    processor.populate(ctx);
    processor.load("@if(!flag)NEGATED@end");
    std::string result = processor.process();
    EXPECT_EQ(result.find("NEGATED"), std::string::npos);
}
