// NOTES ON CONVERTING #version 330 TO OPENGL 2.1
//
// In the fragment program, in becomes varying.
// In the fragment program, gl_FragColor is the output color value.
// In the fragment program, texture() becomes texture2D() or texture1D() as appropriate.

// in parameter from the vertex shader stage
varying vec4 theColor;
varying vec2 tc;
varying vec3 n;
varying vec3 cP;

// uniforms
uniform float time;
uniform float loopDuration;

uniform sampler2D colorMap, normalMap; 

// constant colors
const vec4 white = vec4(1.0, 1.0, 1.0, 1.0);

// Calculates the transformation matrix from camera space to tangent space
// normal:		Comes from the model
// position:	The position refers to the vector pointing at the camera position
// texCoord:	Comes from the model... the fragments interpolated texture coordinate
// REQUIRES NORMALIZED INPUTS
// Based on: http://hacksoflife.blogspot.se/2009/11/per-pixel-tangent-space-normal-mapping.html
mat3 calcTangentMatrix(vec3 normal, vec3 position, vec2 texCoord)
{
	// Retrieve the change in texture coordinates and vector position between fragments
	vec3 dpx = dFdx(position);
	vec3 dpy = dFdy(position);
	vec2 dtx = dFdx(texCoord);
	vec2 dty = dFdy(texCoord);

	// Do some magical magic
	vec3 tangent 	= normalize( dpx * dty.t - dpy * dtx.t );
	vec3 cotangent 	= normalize(-dpx * dty.s + dpy * dtx.s );

	return mat3(tangent, cotangent, normal);
}

void main()
{
	float currTime = mod(time, loopDuration);
	float currLerp = currTime / loopDuration;

	vec3 lightDir = vec3(-1.0, 1.0, 1.0);

	
	// Calculate the TBN matrix with normalized vectors
	vec3 camPos = normalize(cP);
	vec3 normal = normalize(n);
	mat3 TBN	= calcTangentMatrix(normal, camPos, tc);

	// Get the surface normal from the normal map and change range from [0,1] to [-1,1]
	vec3 bump = 2.0 * texture2D(normalMap, tc.xy).xyz - 1.0;

	// Transform surface normal from tangent space to camera space
	normal = normalize(TBN * bump);

	// ... angle of light compared to normal ...
	float NdotL = max(dot(normal, lightDir), 0.0);

	// Allocate a variable to store final fragment color
	vec4 color;

	if(NdotL > 0.0)
	{
		color = NdotL * texture2D(colorMap, tc.xy) * 0.8;
	}

	// Add ambient light
	color += vec4(0.2, 0.2, 0.2, 1.0) * texture2D(colorMap, tc.xy);

	// Set the color of the currently processed fragment
	gl_FragColor = color;
	//gl_FragColor = vec4(normal, 1.0);
}