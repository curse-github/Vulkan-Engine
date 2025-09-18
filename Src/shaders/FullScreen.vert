#version 450

const vec2 Positions[6] = vec2[](
    vec2(- 1.0, - 1.0),// bottom left
    vec2(- 1.0, 1.0),// top left
    vec2( 1.0, 1.0),// top right
    vec2(- 1.0, - 1.0),// bottom left
    vec2( 1.0, 1.0),// top right
    vec2( 1.0, - 1.0) // bottom right
);

layout (location = 0) out vec2 vertUv;

void main() {
    vec2 pos = Positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);
    vertUv = vec2(0.5, 0.5) + pos*0.5;
}