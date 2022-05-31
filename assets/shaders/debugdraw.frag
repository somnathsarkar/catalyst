#version 450

#define EPS 1e-7

layout(set = 0, binding = 0) uniform sampler2D billboards[2];

layout(location = 0) in vec2 texCoords;
layout(location = 1) flat in uint debugdrawId;

layout(location = 0) out vec4 outColor;

void main() {
    if(debugdrawId==0){
        // Default gray
        outColor = vec4(vec3(0.84f),1.0f);
    }else{
        // Billboard texture, alpha testing
        vec4 texSample = texture(billboards[debugdrawId-1],texCoords);
        if(texSample.a < EPS)
            discard;
        outColor = vec4(vec3(0.84f),1.0f);;
    }
}