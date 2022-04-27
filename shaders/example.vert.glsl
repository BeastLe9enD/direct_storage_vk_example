#version 460

const vec2[] positions = vec2[6](
    vec2(-0.5, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5)
);

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec2 texCoordFS;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    texCoordFS = positions[gl_VertexIndex] + vec2(0.5);
}