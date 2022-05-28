#version 450

layout(push_constant) uniform constants{
    mat4 model_to_world_transform;
    mat4 world_to_view_transform;
    mat4 view_to_clip_transform;
    uint material_id;
}push_constants;

layout(location = 0) in vec3 clipPos;

layout(location = 0) out vec2 screenPos;

void main(){
    screenPos = vec2((clipPos.x+1.0f)/2.0f,(clipPos.y+1.0f)/2.0f);
    gl_Position = vec4(clipPos,1.0f);
}