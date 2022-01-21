//type vertex
#version 450

layout(location = 0) in vec2 a_Pos;

void main() {
    gl_Position = vec4(a_Pos.x, a_Pos.y, 0.0, 1.0);
}

//type fragment
#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0f, 0.5f, 0.0f, 1.0f);
}