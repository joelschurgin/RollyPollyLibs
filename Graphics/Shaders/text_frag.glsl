#version 330 core
precision highp float;

in  vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_texture;

uniform float u_pxRange;

float median(vec3 v) {
    return max(min(v.x, v.y), min(max(v.x, v.y), v.z));
}

void main() {
    float dist = median(texture(u_texture, v_texCoord).rgb);

    vec2 unitRange = vec2(u_pxRange) / vec2(textureSize(u_texture, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(v_texCoord);

    float screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);
    float screenPxDistance = screenPxRange * (dist - 0.5);
 
    float alpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    fragColor = vec4(1.0, 1.0, 1.0, alpha);
}
