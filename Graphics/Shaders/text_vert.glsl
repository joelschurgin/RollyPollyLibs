#version 330 core

layout(location = 0) in vec4 position;

out vec2 texCoords;

uniform mat4 u_transform;

void main() {
    gl_Position = u_transform * position;
    texCoords = position.xy;
}
