#version 450

layout(location = 0) in vec3 clipPos;

layout(location = 0) out vec2 screenPos;

void main(){
    screenPos = vec2((clipPos.x+1.0f)/2.0f,(clipPos.y+1.0f)/2.0f);
    gl_Position = vec4(clipPos,1.0f);
}