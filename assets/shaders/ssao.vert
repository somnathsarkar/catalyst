#version 450

layout(push_constant) uniform constants{
    mat4 model_to_world_transform;
    mat4 world_to_view_transform;
    mat4 view_to_clip_transform;
}push_constants;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec2 screenPos;
layout(location = 1) out mat4 clipToViewTransform;

void main() {
    screenPos = (inPosition.xy+1.0f)/2.0f;
    vec4 clipPos = vec4(inPosition,1.0f);
    clipToViewTransform = inverse(push_constants.view_to_clip_transform);
    gl_Position = vec4(inPosition,1.0f);
}