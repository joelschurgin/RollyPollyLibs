#version 330 core
precision highp float;

in  vec2 v_texCoord;
out vec4 fragColor;

uniform sampler2D u_texture;

float median(vec3 v) {
    return max(min(v.x, v.y), min(max(v.x, v.y), v.z));
}

void main() {
    vec3 msd = texture(u_texture, v_texCoord).rgb;
    float dist = median(msd);
    fragColor = vec4(1.0, 1.0, 1.0, smoothstep(0.45, 0.55, dist));
}
