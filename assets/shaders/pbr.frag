#version 450

#define M_PI 3.1415926535897932384626433832795
#define MAX_MIP_LEVEL 12.0f
#define EPS 1e-7f

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
    int ao_texture_id;
};

struct Skybox{
    int specular_cubemap_id;
    int diffuse_cubemap_id;
    float specular_intensity;
    float diffuse_intensity;
};

layout(binding = 0, set = 0, std140) uniform directional_light_uniform_block{
    DirectionalLight lights[16];
    int num_lights;
}directional_light_uniform;

layout(binding = 1) uniform sampler2DShadow directional_shadow_map[16];

layout(binding = 2, set = 0, std140) uniform material_uniform_block{
    Material materials[128];
    int num_materials;
}material_uniform;

layout(binding = 3) uniform sampler2D textures[16];

layout(binding = 4) uniform samplerCube cubemaps[2];

layout(binding = 5) uniform skybox_uniform_block{
    Skybox skybox;
}skybox_uniform;

layout(binding = 6) uniform sampler2D ssao_map;

layout(binding = 7) uniform sampler2D ssr_map;

layout(binding = 8, set = 0, std140) uniform SettingsUniformType{
    float shadowmap_bias;
    int shadowmap_kernel_size;
    float _pad[2];
}settings_uniform;

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 worldTangent;
layout(location = 3) in vec3 worldBiTangent;
layout(location = 4) in vec4 viewPos;
layout(location = 5) in vec4 clipPos;
layout(location = 6) in vec3 worldCamera;
layout(location = 7) in vec2 texCoords;
layout(location = 8) flat in uint materialId;

layout(location = 0) out vec4 outColor;

float gaussian_kernel[] = {0.0625f,0.125f,0.0625f,0.125f,0.25f,0.125f,0.0625f,0.125f,0.0625f};

vec4 texture_gaussian(sampler2D tex, vec2 tex_coord){
    vec2 tex_size = 1.0f/textureSize(tex,0);
    vec4 ans = vec4(0.0f);
    for(int i = -1; i<=1; i++){
        for(int j = -1; j<=1; j++){
            ans += texture(tex,tex_coord+ivec2(i,j)*tex_size)*gaussian_kernel[3*(i+1)+j+1];
        }
    }
    return ans;
}

float shadow_test(sampler2DShadow shadow_map, vec2 shadow_map_size, vec4 loc, vec2 offset){
    return textureProj(shadow_map,vec4(loc.xy+offset*loc.w*shadow_map_size,loc.z,loc.w));
}

void main() {
    Material material = material_uniform.materials[materialId];
    int specular_environment_map = skybox_uniform.skybox.specular_cubemap_id;
    int diffuse_environemnt_map = skybox_uniform.skybox.diffuse_cubemap_id;

    vec3 v = normalize(worldCamera);
    vec3 N = normalize(worldNormal);
    vec3 T = normalize(worldTangent);
    vec3 B = normalize(worldTangent);

    vec3 currentColor = vec3(0.0f);

    vec3 n = N;
    if(material.normal_texture_id>-1)
        n = mat3(T,B,N)*texture(textures[material.normal_texture_id],texCoords).rgb;
    vec3 albedo = material.color.rgb;
    if(material.albedo_texture_id>-1)
        albedo = texture(textures[material.albedo_texture_id],texCoords).rgb;
    float metallic = material.metallic;
    if(material.metallic_texture_id>-1)
        metallic = texture(textures[material.metallic_texture_id],texCoords).r;
    float roughness = material.roughness;
    if(material.roughness_texture_id>-1)
        roughness = texture(textures[material.roughness_texture_id],texCoords).r;
    vec3 R = normalize(reflect(-v,n));
    
    vec2 screen_pos = (clipPos.xy/clipPos.w+1.0f)/2.0f;
    vec3 ssao_sample = texture_gaussian(ssao_map,screen_pos).rgb;
    if(material.ao_texture_id>-1)
        ssao_sample = texture(textures[material.ao_texture_id],texCoords).rgb;
    float roughness_mip = (roughness*MAX_MIP_LEVEL);

    // GI Specular component
    vec3 ssr_sample = ssao_sample*texture_gaussian(ssr_map,screen_pos).rgb;
    currentColor += (1.0f-roughness)*ssr_sample;
    
    // Environmental IBL, Specular component
    currentColor += ssao_sample*vec3(skybox_uniform.skybox.specular_intensity*textureLod(cubemaps[specular_environment_map],R,roughness_mip));
    // Environmental IBL, Diffuse component
    currentColor += ssao_sample*albedo*vec3(skybox_uniform.skybox.diffuse_intensity*textureLod(cubemaps[diffuse_environemnt_map],n,roughness_mip));

    for(uint light_i = 0; light_i<directional_light_uniform.num_lights; light_i++){
        DirectionalLight light = directional_light_uniform.lights[light_i];
        mat4 light_to_world_transform = inverse(light.world_to_light_transform);
        vec3 world_light_pos = light_to_world_transform[3].xyz/light_to_world_transform[3].w;
        vec3 world_light_rel_pos = world_light_pos-worldPos.xyz/worldPos.w;
        vec3 world_light = -vec3(light_to_world_transform[1]);
        // Check if point is behind directional light
        if(dot(world_light_rel_pos,world_light)<=0.0f){
            continue;
        }
        vec3 l = normalize(world_light);
        vec3 h = normalize((l+v));
        // Check if face is away from light
        if(dot(l,n)<=0){
            continue;
        }
        // Height Correlated Smith G2 with GGX NDF
        // G_2/4|n.l||n.v|
        float mu_i = max(dot(n,l),0.0f);
        float mu_o = max(dot(n,v),0.0f);
        float g2_denom_o = mu_o*sqrt(roughness+mu_i*mu_i*(1-roughness));
        float g2_denom_i = mu_i*sqrt(roughness+mu_o*mu_o*(1-roughness));
        float G2 = 0.5f/(g2_denom_o+g2_denom_i);
        // GGX NDF
        // D(h), ignoring chi(n.m)
        float Dh = roughness/(M_PI*pow(1.0f+pow(dot(n,h),2.0f)*(roughness-1.0f),2.0f));
        // Fresnel reflectance General Shlick approximation
        // F(n,l)
        vec3 F0 = 0.16f * material.reflectance * material.reflectance * (1.0f-metallic) + (albedo*metallic);
        vec3 F90 = vec3(1.0f);
        vec3 F = F0+(F90-F0)*pow(1.0f-mu_i,5.0f);
        // Specular BRDF
        // f_spec(l,v)
        vec3 f_spec = F*G2*Dh;
        //f_spec = vec3(0.0f);
        // Diffuse BRDF
        // f_diff(l,v)
        vec3 f_diff = (1-F)*(1.0f-metallic)*albedo/M_PI;

        // Shadow mapping
        float in_shadow = 0.0f;
        float shadow_bias = settings_uniform.shadowmap_bias;
        int shadow_kernel_dim = settings_uniform.shadowmap_kernel_size;
        vec4 light_pos = light.light_to_clip_transform*light.world_to_light_transform*worldPos;
        vec4 light_pos_proj = light_pos/light_pos.w;
        light_pos_proj.xy = (light_pos_proj.xy+1.0f)/2.0f;
        light_pos_proj.z -= shadow_bias;
        vec2 shadowmap_size = 1.0f/textureSize(directional_shadow_map[light_i],0);
        for(int xi = 0; xi<shadow_kernel_dim; xi++){
            for(int yi = 0; yi<shadow_kernel_dim; yi++){
                vec2 shadow_offset = vec2(xi,yi)-(shadow_kernel_dim-1.0f)/2.0f;
                in_shadow += shadow_test(directional_shadow_map[light_i],shadowmap_size,light_pos_proj,shadow_offset);
            }
        }
        in_shadow/=(shadow_kernel_dim*shadow_kernel_dim);
        currentColor+=in_shadow*((f_spec+f_diff)*vec3(light.color)*mu_i);
    }
    // Pass non-gamma corrected values to HDR framebuffer
    outColor = vec4(currentColor,1.0f);
}