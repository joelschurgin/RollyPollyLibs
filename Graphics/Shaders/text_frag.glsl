#version 330 core
precision highp float;

in  vec2 texCoords;
out vec4 fragColor;

uniform sampler2D tex;

void main() {
    fragColor = texture(tex, texCoords);
}
