#version 460

layout(location = 0) in vec2 texCoordFS;

layout(set = 0, binding = 0) uniform sampler2D myTexture;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(myTexture, texCoordFS);
}