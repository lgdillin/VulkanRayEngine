#version 450

layout (location = 0) out vec3 v_fragColor;

void main() {
	const vec3 pos[3] = vec3[3](
		vec3(-1.0, 1.0, 0.0),
		vec3(1.0, 1.0, 0.0),
		vec3(0.0, -1.0, 0.0) 
	);

	const vec3 color[3] = vec3[3](
		vec3(1.0, 0.0, 0.0),
		vec3(0.0, 1.0, 0.0),
		vec3(0.0, 0.0, 1.0)
	);
	
	//int x = gl_VertexIndex;
	v_fragColor = color[gl_VertexIndex];
	gl_Position = vec4(pos[gl_VertexIndex], 1.0);
}