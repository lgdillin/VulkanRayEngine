#version 450

layout (location = 0) in vec3 v_fragColor;

layout (location = 0) out vec4 fragColor;

//in vec3 v_fragColor;

void main() {
	fragColor = vec4(v_fragColor.xyz, 1.0);
}