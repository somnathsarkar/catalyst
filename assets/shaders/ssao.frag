#version 450

#define NUM_SAMPLES 64

layout(binding = 0) uniform sampler2D depthmap;

layout(binding = 1) uniform sampler2D noisemap;

layout(binding = 2, set = 0, std140) uniform ssao_sample_uniform_block{
	vec4 samples[NUM_SAMPLES];
}ssao_sample_uniform;

layout(location = 0) in vec2 screenPos;
layout(location = 1) in mat4 clipToViewTransform;

layout(location = 0) out vec4 outColor;

vec3 GetViewPos(vec2 tex_coord){
	float depth = texture(depthmap,tex_coord).r;
	vec4 clipPos = vec4(tex_coord.x*2.0f-1.0f,tex_coord.y*2.0f-1.0f,depth,1.0f);
	vec4 viewPos4 = clipToViewTransform*clipPos;
	return viewPos4.xyz/viewPos4.w;
}

vec3 GetViewNormal(vec3 view_pos){
	vec2 texel_size = 1.0f/textureSize(depthmap,0);
	vec2 screenPosX0 = screenPos+vec2(-1.0f,0.0f)*texel_size;
	vec3 view_pos_x0 = GetViewPos(screenPosX0);
	vec2 screenPosX1 = screenPos+vec2(1.0f,0.0f)*texel_size;
	vec3 view_pos_x1 = GetViewPos(screenPosX1);
	vec2 screenPosY0 = screenPos+vec2(0.0f,-1.0f)*texel_size;
	vec3 view_pos_y0 = GetViewPos(screenPosY0);
	vec2 screenPosY1 = screenPos+vec2(0.0f,1.0f)*texel_size;
	vec3 view_pos_y1 = GetViewPos(screenPosY1);
	vec3 view_diff_x = view_pos-view_pos_x0;
	vec3 view_diff_y = view_pos-view_pos_y1;
	if(distance(view_pos_x0,view_pos)>distance(view_pos_x1,view_pos)){
		view_diff_x = view_pos_x1-view_pos;
	}
	if(distance(view_pos_y0,view_pos)<distance(view_pos_y1,view_pos)){
		view_diff_y = view_pos_y0-view_pos;
	}
	vec3 view_normal = normalize(cross(view_diff_x,view_diff_y));
	return view_normal;
}

void main(){
	vec3 view_pos = GetViewPos(screenPos);
	vec3 view_normal = GetViewNormal(view_pos);
	vec4 noise_sample = normalize(texture(noisemap,screenPos));
	vec3 view_bitangent = normalize(cross(view_normal,vec3(1.0f,0.0f,0.0f)));
	vec3 view_tangent = normalize(cross(view_normal,view_bitangent));
	mat3 view_tbn = mat3(view_tangent,view_bitangent,view_normal);
	float ao = 0.0f;
	for(int i = 0; i<NUM_SAMPLES; i++){
		vec3 sample_direction = view_tbn*ssao_sample_uniform.samples[i].xyz;
		vec3 sample_view_pos = 0.1f*sample_direction+view_pos;
		vec4 sample_view_pos4 = vec4(sample_view_pos,1.0f);
		vec4 sample_clip_pos4 = inverse(clipToViewTransform)*sample_view_pos4;
		vec3 sample_clip_pos = sample_clip_pos4.xyz/sample_clip_pos4.w;
		vec2 sample_screen_pos = (sample_clip_pos.xy+1.0f)/2.0f;
		float sample_depth = texture(depthmap,sample_screen_pos).r;
		if(sample_depth<sample_clip_pos.z){
			ao+=1.0f/NUM_SAMPLES;
		}
	}
	outColor = vec4(vec3(ao),1.0f);
}