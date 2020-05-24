#version 330

// input from vertex shader
in vec3 norm;
in vec2 tc;

// the only output variable
out vec4 fragColor;

// texture sampler
uniform sampler2D TEX;
uniform sampler2D ballTEX;
uniform bool show_texcoord;
uniform bool isBall;

void main()
{
	//fragColor = show_texcoord ? vec4(tc,0,1) : texture2D( TEX, tc );
	//fragColor = vec4(tc.xy, 0, 1);
	fragColor = isBall ? texture2D( ballTEX, tc ) : texture2D( TEX, tc );

}
