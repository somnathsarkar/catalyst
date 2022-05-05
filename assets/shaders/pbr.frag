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

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec4 viewPos;
layout(location = 3) in vec3 viewNormal;
layout(location = 4) in vec3 worldCamera;
layout(location = 5) in vec2 texCoords;
layout(location = 6) flat in uint materialId;

layout(location = 0) out vec4 outColor;

void main() {
    Material material = material_uniform.materials[materialId];
    vec3 v = normalize(worldCamera);
    vec3 n = normalize(worldNormal);
    vec3 currentColor = vec3(0.0f);
    vec3 albedo = material.color.rgb;
    if(material.albedo_texture_id>-1)
        albedo = texture(textures[material.albedo_texture_id],texCoords).rgb;
    for(uint light_i = 0; light_i<directional_light_uniform.num_lights; light_i++){
        DirectionalLight light = directional_light_uniform.lights[light_i];
        vec3 world_light = -vec3(inverse(light.world_to_light_transform)[1]);
        vec3 l = normalize(world_light);
        vec3 h = normalize((l+v));
        // Height Correlated Smith G2 with GGX NDF
        // G_2/4|n.l||n.v|
        float mu_i = max(dot(n,l),0.0f);
        float mu_o = max(dot(n,v),0.0f);
        float g2_denom_o = mu_o*sqrt(material.roughness+mu_i*mu_i*(1-material.roughness));
        float g2_denom_i = mu_i*sqrt(material.roughness+mu_o*mu_o*(1-material.roughness));
        float G2 = 0.5f/(g2_denom_o+g2_denom_i);
        // GGX NDF
        // D(h), ignoring chi(n.m)
        float Dh = material.roughness/(M_PI*pow(1.0f+pow(dot(n,h),2.0f)*(material.roughness-1.0f),2.0f));
        // Fresnel reflectance General Shlick approximation
        // F(n,l)
        vec3 F0 = 0.16f * material.reflectance * material.reflectance * (1.0f-material.metallic) + (albedo*material.metallic);
        vec3 F90 = vec3(1.0f);
        vec3 F = F0+(F90-F0)*pow(1.0f-mu_i,5.0f);
        // Specular BRDF
        // f_spec(l,v)
        vec3 f_spec = F*G2*Dh;
        //f_spec = vec3(0.0f);
        // Diffuse BRDF
        // f_diff(l,v)
        vec3 f_diff = (1-F)*(1.0f-material.metallic)*albedo/M_PI;

        // Shadow mapping
        float in_shadow = 0.0f;
        float shadow_bias = 0.001f;
        vec4 light_pos = light.light_to_clip_transform*light.world_to_light_transform*worldPos;
        vec2 shadow_pos = (light_pos.st+1.0f)/2.0f;
        vec4 shadow_sample = texture(directional_shadow_map[light_i],shadow_pos);
        float shadow_depth = shadow_sample.r;
        if(shadow_depth<light_pos.z-shadow_bias){
            in_shadow=1.0f;
        }
        currentColor+=(1.0f-in_shadow)*((f_spec+f_diff)*vec3(light.color)*mu_i);
    }
    outColor = vec4(currentColor,1.0f);
}