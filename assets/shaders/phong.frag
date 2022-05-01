#version 450

struct DirectionalLight{
    mat4 world_to_light_transform;
    mat4 light_to_clip_transform;
    vec4 color;
};

layout(binding = 0, set = 0, std140) uniform directional_light_uniform_block{
    DirectionalLight lights[16];
    int num_lights;
}directional_light_uniform;

layout(binding = 1) uniform sampler2D directional_shadow_map[16];

layout(location = 0) in vec4 worldPos;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec4 viewPos;
layout(location = 3) in vec3 viewNormal;
layout(location = 4) in vec3 worldCamera;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 inColor = vec3(1.0f,1.0f,1.0f);
    float shininess = 20.0f;
    float k_a = 0.1f;
    float k_d = 1.0f;
    float k_s = 1.0f;
    vec3 ambient = k_a*inColor;
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);
    for(int light_i = 0; light_i<directional_light_uniform.num_lights; light_i++){
        float shadow = 0.0f;
        vec2 texel_size = 1.0f/textureSize(directional_shadow_map[light_i],0);
        DirectionalLight light = directional_light_uniform.lights[light_i];
        vec3 world_light_dir = -vec3(inverse(light.world_to_light_transform)[1]);
        vec4 light_pos = light.light_to_clip_transform*light.world_to_light_transform*worldPos;
        vec3 L = normalize(world_light_dir);
        vec3 N = normalize(worldNormal);
        vec3 V = normalize(worldCamera);
        vec3 R = reflect(L,N);
        //float shadow_bias = max(0.05f*(1.0f-dot(N,L)),0.005f);
        float shadow_bias = 0.001f;
        float light_shadow = 0.0f;
        for(int x_off = -1; x_off<=1; x_off++){
            for(int y_off = -1; y_off<=1; y_off++){
                vec2 shadow_tex_coord = (light_pos.st+1.0f)/2.0f;
                vec4 shadow_sample = texture(directional_shadow_map[light_i],shadow_tex_coord+texel_size*vec2(x_off,y_off));
                float shadow_depth = shadow_sample.r;
                if(shadow_depth<light_pos.z-shadow_bias){
                    light_shadow+=1.0f;
                }
            }
        }
        light_shadow /= 9.0f;
        shadow+=light_shadow;
        float lambertian = max(dot(N,L),0.0f);
        diffuse+=(1-shadow)*lambertian*vec3(light.color);
        specular+=(1-shadow)*pow(max(dot(R,V),0.0f),shininess)*vec3(light.color);
    }
    diffuse*=k_d;
    specular*=k_s;
    vec3 phong = ambient+(diffuse+specular);
    outColor = vec4(phong,1.0f);
}