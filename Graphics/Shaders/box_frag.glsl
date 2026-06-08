#version 330 core
precision highp float;

out vec4 fragColor;

uniform vec4 u_fill_color;
uniform vec4 u_border_color;
uniform vec4 u_rect;
uniform float u_radius;
uniform float u_border_thickness;

float sdRoundedBox(in vec2 p, in vec2 b, in float r) {
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

float sdBorder(float d, float thickness) {
    return abs(d + thickness * 0.5) - thickness * 0.5;
}

void main() {
    vec2 half_res = u_rect.zw * 0.5;
    float rounded_box = sdRoundedBox(gl_FragCoord.xy - u_rect.xy - half_res, half_res, u_radius);
    float border_dist = sdBorder(rounded_box, u_border_thickness);

    float border_edge_softness = fwidth(border_dist);
    float border = smoothstep(-border_edge_softness, border_edge_softness, border_dist);

    vec4 mixed_color = mix(u_border_color, u_fill_color, border);
    fragColor = vec4(mixed_color.xyz, mixed_color.w * (1.0 - smoothstep(0.0, 1.0, rounded_box)));
}
