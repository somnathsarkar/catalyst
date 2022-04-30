#version 450

layout(push_constant) uniform constants{
    mat4 model_to_world_transform;
    mat4 world_to_view_transform;
    mat4 view_to_clip_transform;
}push_constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;

void main() {
    vec4 view_pos = push_constants.world_to_view_transform*push_constants.model_to_world_transform*vec4(inPosition,1.0f);
    gl_Position = push_constants.view_to_clip_transform*view_pos;
    fragColor = vec3(0.827f,0.827f,0.827f);
}