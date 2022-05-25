#version 450

#define M_PI 3.1415926535897932384626433832795

struct DirectionalLight{
    mat4 world_to_light_transform;
    mat4 light_to_clip_transform;
    vec4 color;
};

struct Material{
    vec4 color;
    float reflectance;
    float metallic;
    float roughness;
    int albedo_texture_id;
    int metallic_texture_id;
    int roughness_texture_id;
    int normal_texture_id;
    int _pad;
};

struct Skybox{
    int specular_cubemap_id;
    int diffuse_cubemap_id;
    ivec2 _pad;
};

layout(binding = 0, set = 0, std140) uniform directional_light_uniform_block{
    DirectionalLight lights[16];
    int num_lights;
}directional_light_uniform;

layout(binding = 1) uniform sampler2D directional_shadow_map[16];

layout(binding = 2, set = 0, std140) uniform material_uniform_block{
    Material materials[128];
    int num_materials;
}material_uniform;

layout(binding = 3) uniform sampler2D textures[16];

layout(binding = 4) uniform samplerCube cubemaps[2];

layout(binding = 5) uniform skybox_uniform_block{
    Skybox skybox;
}skybox_uniform;


layout(location = 0) in vec3 eyeDirection;

layout(location = 0) out vec4 outColor;

void main(){
    int skybox_map = skybox_uniform.skybox.specular_cubemap_id;
    outColor = texture(cubemaps[skybox_map],eyeDirection);
}