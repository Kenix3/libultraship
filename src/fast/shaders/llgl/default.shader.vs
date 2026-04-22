@prism(type='vertex', name='Fast3D LLGL Vertex Shader', version='1.0.0', description='Vulkan GLSL vertex shader for the LLGL backend', author='libultraship')

#version 450 core

// vs_in_loc(fmt) increments the VS input location counter and records the
// attribute in the LLGL VertexFormat.  Format codes: 1=R32F, 2=RG32F, 3=RGB32F, 4=RGBA32F.
// vs_out_loc(0) increments the VS output (== FS input) location counter.

layout(location = @{vs_in_loc(4)}) in vec4 aVtxPos;

@for(i in 0..2)
    @if(o_textures[i])
        layout(location = @{vs_in_loc(2)}) in vec2 aTexCoord@{i};
        layout(location = @{vs_out_loc(0)}) out vec2 vTexCoord@{i};
        @for(j in 0..2)
            @if(o_clamp[i][j])
                @if(j == 0)
                    layout(location = @{vs_in_loc(1)}) in float aTexClampS@{i};
                    layout(location = @{vs_out_loc(0)}) out float vTexClampS@{i};
                @else
                    layout(location = @{vs_in_loc(1)}) in float aTexClampT@{i};
                    layout(location = @{vs_out_loc(0)}) out float vTexClampT@{i};
                @end
            @end
        @end
    @end
@end

@if(o_fog)
    layout(location = @{vs_in_loc(4)}) in vec4 aFog;
    layout(location = @{vs_out_loc(0)}) out vec4 vFog;
@end

@if(o_grayscale)
    layout(location = @{vs_in_loc(4)}) in vec4 aGrayscaleColor;
    layout(location = @{vs_out_loc(0)}) out vec4 vGrayscaleColor;
@end

@for(i in 0..o_inputs)
    @if(o_alpha)
        layout(location = @{vs_in_loc(4)}) in vec4 aInput@{i + 1};
        layout(location = @{vs_out_loc(0)}) out vec4 vInput@{i + 1};
    @else
        layout(location = @{vs_in_loc(3)}) in vec3 aInput@{i + 1};
        layout(location = @{vs_out_loc(0)}) out vec3 vInput@{i + 1};
    @end
@end

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    @for(i in 0..2)
        @if(o_textures[i])
            vTexCoord@{i} = aTexCoord@{i};
            @for(j in 0..2)
                @if(o_clamp[i][j])
                    @if(j == 0)
                        vTexClampS@{i} = aTexClampS@{i};
                    @else
                        vTexClampT@{i} = aTexClampT@{i};
                    @end
                @end
            @end
        @end
    @end
    @if(o_fog)
        vFog = aFog;
    @end
    @if(o_grayscale)
        vGrayscaleColor = aGrayscaleColor;
    @end
    @for(i in 0..o_inputs)
        vInput@{i + 1} = aInput@{i + 1};
    @end
    gl_Position = aVtxPos;
}
