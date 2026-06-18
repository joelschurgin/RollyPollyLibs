#version 330 core

layout(location = 0) in vec4 position;

out vec2 v_texCoord;

uniform mat4 u_transform;
uniform samplerBuffer u_fontLibrary;

uniform float u_fontSize;
uniform vec2 u_pos;

void main() {
    int characterID = 97;
    int libraryIndex = characterID * 2;
    vec4 atlasBounds = texelFetch(u_fontLibrary, libraryIndex);
    vec4 planeBounds = texelFetch(u_fontLibrary, libraryIndex + 1);
 
    float left   = atlasBounds.x;
    float bottom = atlasBounds.y;
    float right  = atlasBounds.z;
    float top    = atlasBounds.w;

    v_texCoord.x = mix(left, right, position.x);
    v_texCoord.y = mix(bottom, top, position.y);

    vec2 mesh = u_fontSize * mix(planeBounds.xy, planeBounds.zw, position.xy);
    gl_Position = u_transform * vec4(u_pos + mesh, 0.0, 1.0);
}
