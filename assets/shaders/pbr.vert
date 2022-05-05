#version 450

layout(push_constant) uniform constants{
    mat4 model_to_world_transform;
    mat4 world_to_view_transform;
    mat4 view_to_clip_transform;
    uint material_id;
}push_constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 worldPos;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec4 viewPos;
layout(location = 3) out vec3 viewNormal;
layout(location = 4) out vec3 worldCamera;
layout(location = 5) out vec2 texCoord;
layout(location = 6) out uint materialId;

void main() {
    mat3 model_to_world_vec_transform = mat3(transpose(inverse(push_constants.model_to_world_transform)));
    mat3 world_to_view_vec_transform = mat3(transpose(inverse(push_constants.world_to_view_transform)));
    worldPos = push_constants.model_to_world_transform*vec4(inPosition,1.0f);
    viewPos = push_constants.world_to_view_transform*worldPos;
    vec3 viewCamera = normalize(-viewPos.xyz/viewPos.w);
    worldCamera = normalize(inverse(world_to_view_vec_transform)*viewCamera);
    worldNormal = model_to_world_vec_transform*inNormal;
    viewNormal = world_to_view_vec_transform*worldNormal;
    texCoord = inUV;
    materialId = push_constants.material_id;
    gl_Position = push_constants.view_to_clip_transform*viewPos;
}