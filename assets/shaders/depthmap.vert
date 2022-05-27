#version 450

layout(push_constant) uniform constants{
    mat4 model_to_world_transform;
    mat4 world_to_view_transform;
    mat4 view_to_clip_transform;
}push_constants;

invariant gl_Position;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

void main() {
    vec4 world_pos = push_constants.model_to_world_transform*vec4(inPosition,1.0f);    
    vec4 view_pos = push_constants.world_to_view_transform*world_pos;
    gl_Position = push_constants.view_to_clip_transform*view_pos;
}