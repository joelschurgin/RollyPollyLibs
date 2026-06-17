#version 330 core

layout(location = 0) in vec4 position;

out vec2 v_texCoord;

//uniform mat4 u_transform;
uniform samplerBuffer u_fontLibrary;

void main() {
    //gl_Position = u_transform * position;

    int characterID = 105;
    int libraryIndex = characterID * 2;
    vec4 atlasBounds = texelFetch(u_fontLibrary, libraryIndex);
    vec4 planeBounds = texelFetch(u_fontLibrary, libraryIndex + 1);
 
    float left   = atlasBounds.x;
    float bottom = atlasBounds.y;
    float right  = atlasBounds.z;
    float top    = atlasBounds.w;

    v_texCoord.x = mix(left, right, position.x);
    v_texCoord.y = mix(bottom, top, position.y);

    float meshX = mix(planeBounds.x, planeBounds.z, position.x);
    float meshY = mix(planeBounds.y, planeBounds.w, position.y);

    gl_Position = vec4(vec2(meshX, meshY) * 0.5, 0.0, 1.0);
}
