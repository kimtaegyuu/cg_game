#version 330

in vec2 TexCoords;
out vec4 color;
uniform sampler2D text;
uniform vec4 textColor;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor) * sampled;
	//vec4 sampled = vec4(textColor.xyz, texture(text, TexCoords).r*textColor.a);
	//color = mix(vec4(1,0,0,0), sampled, sampled.a);
} 