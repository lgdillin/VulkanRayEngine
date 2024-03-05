#version 450

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec3 in_color;

//layout (location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
	mat2 transform;
	vec2 offset;
	vec3 color;
} push;

void main() {
  //gl_Position = vec4(in_position + push.offset, 0.0, 1.0);
  
  // if we do this, we don't need to specify the VK_SHADER_FRAGMENT_BIT
  //fragColor = push.color;
}