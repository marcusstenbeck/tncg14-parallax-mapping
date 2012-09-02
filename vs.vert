// NOTES ON CONVERTING #version 330 TO OPENGL 2.1
//
// In the vertex program, in becomes attribute.
// In the vertex program, out becomes varying.
// There aren't any of the smooth, flat, or perspective keywords.

// VBO inputs
attribute vec4 position;
attribute vec4 color;
attribute vec2 texCoord;
attribute vec3 normal;

// out parameter going into the fragment shader stage
varying vec4 theColor;
varying vec2 tc;
varying vec3 n;
varying vec4 toCamera;

// uniforms
uniform float time;
uniform float loopDuration;
uniform mat4 perspectiveMatrix;

void main()
{
	// Divide 2*PI by the loopDuration to control movement speed
	float timeScale = 3.14159 * 2.0 / loopDuration;

	// Calculate looptime from [0, loopDuration]
	float currTime = mod(time, loopDuration);

	// Calculate offset position of vertex (COLUMN-MAJOR)
	mat4 offset = mat4(
							1.0, 						0.0, 						0.0, 	0.0,
							0.0, 						1.0, 						0.0, 	0.0,
							0.0, 						0.0, 						1.0, 	0.0,
							0.0,						0.0,						-2.0,	1.0
							/*cos(currTime * timeScale), 	sin(currTime * timeScale), 	-5.0, 	1.0*/
						);

	// 
	mat4 rotY = mat4(
						cos(currTime * timeScale),		0.0,	-sin(currTime * timeScale), 0.0,
						0.0,							1.0,						0.0,	0.0,
						sin(currTime * timeScale),		0.0,	cos(currTime * timeScale),	0.0,
						0.0,							0.0,						0.0,	1.0
					);

	mat4 rotX = mat4(
						1.0,							0.0,						0.0, 	0.0,
						0.0,		cos(currTime * timeScale),	sin(currTime * timeScale),	0.0,
						0.0,		-sin(currTime * timeScale),	cos(currTime * timeScale),	0.0,
						0.0,							0.0,						0.0,	1.0
					);

	// 
	mat4 modelViewMatrix = offset * rotY * rotX;
	vec4 cameraPos = modelViewMatrix * position;

	// Save normalized device coordinates to GPU memory
	gl_Position = perspectiveMatrix * cameraPos;


	// BELOW: Prepare some variables for the fragment shader

	// Pass the color value to the fragment shader
	theColor = color;
	tc = texCoord;

	// 
	mat3 normalMatrix = mat3(
						modelViewMatrix[0][0], modelViewMatrix[0][1], modelViewMatrix[0][2], 
						modelViewMatrix[1][0], modelViewMatrix[1][1], modelViewMatrix[1][2], 
						modelViewMatrix[2][0], modelViewMatrix[2][1], modelViewMatrix[2][2]
					);
	n = normalize(normalMatrix * normal);

	toCamera = cameraPos;
}