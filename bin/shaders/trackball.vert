#version 330

// vertex attributes
in vec3 position;
in vec3 normal;
in vec2 texcoord;

// matrices
uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform mat4 rotate_matrix;

uniform vec3 center;
uniform float radius;
uniform float aspect_ratio;

out vec3 norm;
out vec2 tc;

void main()
{
	gl_Position = vec4(center, 0) + vec4(position*radius, 1);
	vec4 rpos = rotate_matrix * gl_Position;
	vec4 wpos = model_matrix * rpos;
	vec4 epos = view_matrix * wpos;
	gl_Position = projection_matrix * epos;

	// pass eye-coordinate normal to fragment shader
	norm = normalize(mat3(view_matrix*model_matrix)*normal);
	tc = texcoord;
}
