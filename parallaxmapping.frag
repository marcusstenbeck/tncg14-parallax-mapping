// NOTES ON CONVERTING shader #version 330 TO OPENGL 2.1
//
// In the fragment program, in becomes varying.
// In the fragment program, gl_FragColor is the output color value.
// In the fragment program, texture() becomes texture2D() or texture1D() as appropriate.

// in parameter from the vertex shader stage
varying vec4 theColor;
varying vec2 tc;
varying vec3 n;
varying vec4 vec2Camera;

// uniforms
uniform float time;
uniform float loopDuration;

uniform sampler2D diffuseMap, normalMap, displacementMap; 

// constant colors
const vec4 white = vec4(1.0, 1.0, 1.0, 1.0);

// Calculates the transformation matrix from camera space to tangent space
// normal:		Comes from the model
// vec2cam:	The vec2cam refers to the vector pointing at the camera
// texCoord:	Comes from the model... the fragments interpolated texture coordinate
// REQUIRES NORMALIZED INPUTS
// Based on: http://hacksoflife.blogspot.se/2009/11/per-pixel-tangent-space-normal-mapping.html
mat3 calcTangentMatrix(vec3 normal, vec3 vec2cam, vec2 texCoord)
{
	// Retrieve the change in texture coordinates and vector position between fragments
	vec3 dpx = dFdx(vec2cam);
	vec3 dpy = dFdy(vec2cam);
	vec2 dtx = dFdx(texCoord);
	vec2 dty = dFdy(texCoord);

	// Do some magical magic (which is: solving a system om equations)
	vec3 tangent 	= normalize( ( dpx * dty.t - dpy * dtx.t ) );
	vec3 cotangent 	= normalize( (-dpx * dty.s + dpy * dtx.s ) );

	return mat3(tangent, cotangent, normal);
}

vec2 calcNewTexCoords(sampler2D displacementMap, vec2 tc, vec3 tsVec2Camera)
{ 
	// Get height from height map
	float height = texture2D(displacementMap, tc).r; // .r because all color channels are the same

	// Calculate new height based on surface thickness and bias
	float surfaceThickness = 0.015; // Thickness relative to width and height
	float bias = surfaceThickness * -0.5;
	float height_bs = height * surfaceThickness + bias;

	// Calculate new texture coordinate based on viewing angle
	vec2 parallaxTextureOffset = height_bs * tsVec2Camera.xy;

	return parallaxTextureOffset;
}

void main()
{
	float currTime = mod(time, loopDuration);
	float currLerp = currTime / loopDuration;

	// Vary the light direction to show off effect
	vec3 lightDir = vec3(cos(5.0*currTime), 1.0, 1.0);

	
	// Calculate the TBN matrix with normalized vectors
	vec3 toCam 	= -vec2Camera.xyz;
	vec3 normal = normalize(n);
	mat3 TBN	= calcTangentMatrix(normal, toCam, tc);
	mat3 TBNi	= mat3( // Inverse of TBN = transpose of TBN
						TBN[0][0], TBN[1][0], TBN[2][0],
						TBN[0][1], TBN[1][1], TBN[2][1],
						TBN[0][2], TBN[1][2], TBN[2][2]
						);


	// START: Parallax code //

	// Transform the camera
	vec3 tsVec2Camera = TBNi * vec2Camera.xyz;

	// WHAT DOES THIS DO!?
	vec2 newCoords = tc + calcNewTexCoords(displacementMap, tc, tsVec2Camera); 
	newCoords+=calcNewTexCoords(displacementMap, newCoords, tsVec2Camera);
	newCoords+=calcNewTexCoords(displacementMap, newCoords, tsVec2Camera);
	newCoords+=calcNewTexCoords(displacementMap, newCoords, tsVec2Camera);


	

	// END: Parallax code //



	// Get the surface normal from the normal map and change range from [0,1] to [-1,1]
	vec3 bump = 2.0 * texture2D(normalMap, newCoords.xy).xyz - 1.0;

	// Transform surface normal from tangent space to camera space
	normal = normalize(TBNi * bump);

	// ... angle of light compared to normal ...
	float NdotL = max(dot(normal, lightDir), 0.0);

	// Allocate a variable to store final fragment color
	vec4 color;

	if(NdotL > 0.0)
	{
		color = NdotL * texture2D(diffuseMap, newCoords) * 0.8;
	}

	// Add ambient light
	color += vec4(0.2, 0.2, 0.2, 1.0) * texture2D(diffuseMap, newCoords);

	// Set the color of the currently processed fragment
	gl_FragColor = color;
	//gl_FragColor = vec4(normal, 1.0);
}