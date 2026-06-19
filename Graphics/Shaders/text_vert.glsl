#version 330 core

layout(location = 0) in vec4 position;

out vec2 v_texCoord;

uniform mat4 u_transform;
uniform samplerBuffer u_fontLibrary;
uniform samplerBuffer u_stringBuffer;

uniform float u_fontSize;
uniform vec2 u_pos;

void main() {
    vec4 stringData = texelFetch(u_stringBuffer, gl_InstanceID);

    float font_size = stringData.z;
    vec2 char_pos = stringData.xy;
    int character = int(stringData.w);

    int libraryIndex = character * 2;

    vec4 atlasBounds = texelFetch(u_fontLibrary, libraryIndex);
    vec4 planeBounds = texelFetch(u_fontLibrary, libraryIndex + 1);

    v_texCoord = mix(atlasBounds.xy, atlasBounds.zw, position.xy);
    vec2 local_mesh = mix(planeBounds.xy, planeBounds.zw, position.xy);
    vec2 final_mesh = font_size * (local_mesh + char_pos) + u_pos;
    gl_Position = u_transform * vec4(final_mesh, position.zw);
}
