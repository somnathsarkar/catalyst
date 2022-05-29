#version 450

#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

layout(set = 0, binding = 1) uniform ToneMappingUniformType{
	float log_illuminance_sum;
	uint num_pixels;
	float _pad[2];
}tonemapping_uniform;

layout(location = 0) in vec2 screenPos;

layout(location = 0) out vec4 outColor;

vec3 RgbToXyy(vec3 rgb){
	mat3 transform_mat = mat3(0.4124564,0.2126729,0.0193339,
								0.3575761,0.7151522,0.1191920,
								0.1804375,0.0721750,0.9503041);
	vec3 xyz = transform_mat*rgb;
	float y = xyz.y;
	float den = xyz.x+xyz.y+xyz.z;
	vec3 xyy = vec3(xyz.x/den,xyz.y/den,0.0f);
	xyy.z = y;
	return xyy;
}

vec3 XyyToRgb(vec3 xyy){
	mat3 transform_mat = mat3(3.2404542,-0.9692660,0.0556434,
							-1.5371385,1.8760108,-0.2040259,
							-0.4985314,0.0415560,1.0572252);
	float den = xyy.z/xyy.y;
	vec3 xyz = vec3(xyy.x*den,xyy.y*den,0.0f);
	xyz.z = den-xyz.x-xyz.y;
	vec3 rgb = transform_mat*xyz;
	return rgb;
}

void main(){
	float exposure = exp(-tonemapping_uniform.log_illuminance_sum/tonemapping_uniform.num_pixels);
	vec3 inRgb = texture(inputTexture,screenPos).rgb;
	vec3 inXyy = RgbToXyy(inRgb);
	inXyy.z*=exposure;
	vec3 exposedRgb = XyyToRgb(inXyy);
	vec3 reinhardColor = exposedRgb/(1.0f+exposedRgb);
	vec3 gamma_correction = vec3(1.0f/2.2f);
	vec3 correctedColor = pow(reinhardColor,gamma_correction);
	outColor = vec4(correctedColor,1.0f);
}