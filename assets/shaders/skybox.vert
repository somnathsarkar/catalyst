#version 450

layout(push_constant) uniform constants{
    mat4 model_to_world_transform;
    mat4 world_to_view_transform;
    mat4 view_to_clip_transform;
    uint material_id;
}push_constants;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 eyeDirection;

void main(){
    vec4 clipPos = vec4(inPosition,1.0f);
    mat4 inverseProjection = inverse(push_constants.view_to_clip_transform);
    mat3 inverseModelView = mat3(transpose(push_constants.world_to_view_transform));
    vec3 unprojected = (inverseProjection*clipPos).xyz;
    eyeDirection = inverseModelView*unprojected;
    gl_Position = clipPos;
}