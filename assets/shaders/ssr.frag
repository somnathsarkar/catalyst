#version 450

layout(set = 0, binding = 0) uniform sampler2D depthmap;

layout(set = 0, binding = 1) uniform sampler2D previous_frame;

layout(location = 0) in vec2 screenPos;
layout(location = 1) in mat4 clipToViewTransform;

layout(location = 0) out vec4 outColor;

vec2 depth_texel_size;
float depthmap_samples[5][5];
float depthmap_blur[3][3];
float gaussian_kernel[] = {0.0625f,0.125f,0.0625f,0.125f,0.25f,0.125f,0.0625f,0.125f,0.0625f};

void PopulateDepthmapSamples(){
	for(int i = -2; i<=2; i++){
		for(int j = -2; j<=2; j++){
			depthmap_samples[i+2][j+2] = texture(depthmap,screenPos+vec2(float(i),float(j))*depth_texel_size).r;
		}
	}
	for(int i = -1; i<=1; i++){
		for(int j = -1; j<=1; j++){
			depthmap_blur[i+1][j+1] = 0.0f;
			for(int gi = -1; gi<=1; gi++){
				for(int gj = -1; gj<=1; gj++){
					depthmap_blur[i+1][j+1] += gaussian_kernel[(gi+1)*3+gj+1]*depthmap_samples[i+gi+2][j+gj+2];
				}
			}
		}
	}
}

vec3 GetViewPos(ivec2 offset){
	float depth = depthmap_blur[offset.x+1][offset.y+1];
	vec2 tex_coord = screenPos+offset*depth_texel_size;
	vec4 clipPos = vec4(tex_coord.x*2.0f-1.0f,tex_coord.y*2.0f-1.0f,depth,1.0f);
	vec4 viewPos4 = clipToViewTransform*clipPos;
	return viewPos4.xyz/viewPos4.w;
}

vec3 GetViewNormal(vec3 view_pos){
	vec3 view_pos_x0 = GetViewPos(ivec2(-1,0));
	vec3 view_pos_x1 = GetViewPos(ivec2(1,0));
	vec3 view_pos_y0 = GetViewPos(ivec2(0,-1));
	vec3 view_pos_y1 = GetViewPos(ivec2(0,1));
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
	mat4 viewToClipTransform = inverse(clipToViewTransform);
	depth_texel_size = 1.0f/textureSize(depthmap,0);
	PopulateDepthmapSamples();
	vec3 view_pos = GetViewPos(ivec2(0,0));
	vec3 view_normal = GetViewNormal(view_pos);
	vec3 view_dir = normalize(view_pos);
	vec3 reflect_dir = reflect(view_dir,view_normal);
	float dist = 0.1f;
	vec3 reflect_color = vec3(0.0f);
	while(true){
		vec4 view_reflect_pos = vec4(view_pos+reflect_dir*dist,1.0f);
		vec4 clip_reflect_pos = viewToClipTransform*view_reflect_pos;
		vec2 screen_reflect_pos = clip_reflect_pos.xy/clip_reflect_pos.w;
		screen_reflect_pos = (screen_reflect_pos+1.0f)/2.0f;
		float reflect_depth = clip_reflect_pos.z/clip_reflect_pos.w;
		if(screen_reflect_pos.x<0.0f||screen_reflect_pos.x>1.0f||screen_reflect_pos.y<0.0f||screen_reflect_pos.y>1.0f)
			break;
		float depth_sample = texture(depthmap,screen_reflect_pos).r;
		if(depth_sample<reflect_depth){
			reflect_color = texture(previous_frame,screen_reflect_pos).rgb;
			break;
		}
		dist += 0.1f;
	}
	outColor = vec4(reflect_color,1.0f);
}