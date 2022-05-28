#version 450

#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

layout(set = 0, binding = 1) uniform sampler2D illuminanceMap;

layout(location = 0) in vec2 screenPos;

layout(location = 0) out vec4 outColor;

void main(){
	vec4 gamma_correction = vec4(1.0f/2.2f);
	outColor = pow(texture(inputTexture,screenPos),gamma_correction);
}