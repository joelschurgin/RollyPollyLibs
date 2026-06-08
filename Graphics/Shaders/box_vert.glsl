#version 330 core

layout(location = 0) in vec4 position;
uniform mat4 u_transform;

void main() {
    gl_Position = u_transform * position;
}
