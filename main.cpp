/*
	Everything written by Marcus Stenbeck

	# Literature used
	
	Learning Modern 3D Graphics Programming
	http://www.arcsynthesis.org/gltut/index.html


	# TODO
		- Point light
		- Mouse movement
		- Load object from file

	# DONE
		- Textures
		- Transformations 

*/


// Standard C++ headers
#include <iostream>		// 
#include <vector>		// 
#include <math.h>

// Include header for OpenGL, GLUT and GLEW
#include <GL/glew.h>	// Removes the need to include OpenGL headers
#include <GLUT/glut.h>	// Window handling system

// Custom headers
#include "lib/readfile.h"	// Reads from file to char*
#include "lib/png_reader.h"	// Reads PNG files


#define ARRAY_COUNT( array ) (sizeof( array ) / (sizeof( array[0] ) * (sizeof( array ) != sizeof(void*) || sizeof( array[0] ) <= sizeof(void*))))


// 
GLuint LoadShader(GLenum eShaderType, const char* fileName)
{
	// Create a shader object
	GLuint shader = glCreateShader(eShaderType);

	// Create a C-style character array string from C++ std::string object
	const char *strFileData = readFile(fileName);

	// Load shader string into shader object
	glShaderSource(
					shader,			// The shader object to load the string into
					1,				// Number of string to put into shader: 1
					&strFileData,	// An array of const char* strings
					NULL			// Array of lengths of the strings, or NULL som null-terminated strings
				);

	// Compile the shader
	glCompileShader(shader);

	// After compiling we need to see if there were any errors
	GLint status;
	glGetShaderiv(
					shader,				// The shader object to retrieve information from
					GL_COMPILE_STATUS,	// What to retrieve from the shader object
					&status				// Where to put the data we retrieve
				);

	// If the compilation has errors
	if(status == GL_FALSE)
	{
		// 
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		// 
		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(
							shader,			// 
							infoLogLength,	// 
							NULL,			// 
							strInfoLog		// 
							);

		// 
		const char *strShaderType = NULL;
		switch(eShaderType)
		{
			case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
			//case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
			case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
		}

		// 
		fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete[] strInfoLog;
	}

	// 
	return shader;
}

// 
GLuint CreateProgram(const std::vector<GLuint> &shaderList)
{
	// 
	GLuint program = glCreateProgram();

	// 
	for(size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glAttachShader(
						program,			// Which program to attach the shader object to
						shaderList[iLoop]	// Reference to the shader object
					);

	// 
	glBindAttribLocation(program, 0, "vertexPosition");
	glBindAttribLocation(program, 1, "vertexColor");
	glBindAttribLocation(program, 2, "vertexTexCoords");
	glBindAttribLocation(program, 3, "vertexNormal");

	// Link shader objects to shader program
	glLinkProgram(program);

	// 
	GLint status;
	glGetProgramiv(
					program,		// 
					GL_LINK_STATUS,	// 
					&status			// 
				);

	// 
	if(status == GL_FALSE)
	{
		// 
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		// 
		GLchar *strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		fprintf(stderr, "Linker failure: %s\n", strInfoLog);
	}

	// 
	for(size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
		glDetachShader(program, shaderList[iLoop]);

	// 
	return program;
}

// 
GLuint theProgram;
GLuint perspectiveMatrixUni;

const char* fnVertexShader = "vs.vert";
const char* fnFragmentShader = "parallaxmapping.frag";

// Uniform location
GLuint timeUniform;

// Allocate memory for perspective matrix
float perspectiveMatrix[16];

float CalcFrustumScale(float fFovDeg)
{
    const float degToRad = 3.14159f * 2.0f / 360.0f;
    float fFovRad = fFovDeg * degToRad;
    return 1.0f / tan(fFovRad / 2.0f);
}

float frustumScale = CalcFrustumScale(39.6);

void createPerspectiveMatrix(float frustumScale, float zNear, float zFar, float* mat)
{
	/*
		[  0  1  2  3 ]
		[  4  5  6  7 ]
		[  8  9 10 11 ]
		[ 12 13 14 15 ]
	*/

	mat[0] = frustumScale;
	mat[5] = frustumScale;
	mat[10] = (zNear + zFar) / (zNear - zFar);
	mat[11]	= 2.0 * zNear * zFar / (zNear - zFar);
	mat[14] = -1.0;
}

void InitializeProgram()
{
	// Create a vector to store all shader objects
	std::vector<GLuint> shaderList;

	// Load the shader objects
	shaderList.push_back(LoadShader(GL_VERTEX_SHADER, fnVertexShader));
	shaderList.push_back(LoadShader(GL_FRAGMENT_SHADER, fnFragmentShader));

	// Create a shader program containing all shaders
	theProgram = CreateProgram(shaderList);
	
	// Delete the shader objects - they are still in the compiled shader program
	std:for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

	// Get the location for the shader uniform "offset"
	timeUniform = glGetUniformLocation(theProgram, "time");
	GLuint loopDurationUniform = glGetUniformLocation(theProgram, "loopDuration");
	perspectiveMatrixUni = glGetUniformLocation(theProgram, "perspectiveMatrix");
	
	// void * memset ( void * ptr, int value, size_t num );
	// Fill block of memory
	// Sets the first num bytes of the block of memory pointed by ptr to the specified value (interpreted as an unsigned char).
	// 
	// So... basically set all values to zero
	memset(perspectiveMatrix, 0, sizeof(float) * 16);

	createPerspectiveMatrix(
							frustumScale,		// Frustum scale
							1.0,				// Near clipping plane
							10000.0,			// Far clipping plane
							perspectiveMatrix	// The array to write to
						);

	// We need to bind the program to set uniforms
	glUseProgram(theProgram);

		glUniform1f(loopDurationUniform, 25.0);
		glUniformMatrix4fv(
							perspectiveMatrixUni,	// Uniform location
							1,						// Number of matrixes (can be an array of matrices)
							GL_TRUE,				// Is the array row-major? GL_TRUE / GL_FALSE
							perspectiveMatrix				// The actual data
						);

	glUseProgram(0);
}

// MODELS
struct VertexData
{
	float position[4];
	float color[4];
	float textureCoordinate[2];
	float normal[3];
};

struct VertexData UnitCube[] = {
	//   x     y     z    w  	  r     g     b     a 		 tx    ty		 nx	   ny    nz

	// FRONT
	{ -0.5,  0.5,  0.5, 1.0,  	1.0,  0.0,  0.0,  1.0,  	0.0,  1.0,		0.0,  0.0,  1.0 },
	{ -0.5, -0.5,  0.5, 1.0,  	1.0,  0.0,  0.0,  1.0,  	0.0,  0.0,		0.0,  0.0,  1.0 },
	{  0.5, -0.5,  0.5, 1.0,  	1.0,  0.0,  0.0,  1.0,  	1.0,  0.0,		0.0,  0.0,  1.0 },

	{ -0.5,  0.5,  0.5, 1.0,  	1.0,  0.0,  0.0,  1.0,  	0.0,  1.0,		0.0,  0.0,  1.0 },
	{  0.5, -0.5,  0.5, 1.0,  	1.0,  0.0,  0.0,  1.0,  	1.0,  0.0,		0.0,  0.0,  1.0 },
	{  0.5,  0.5,  0.5, 1.0,  	1.0,  0.0,  0.0,  1.0,  	1.0,  1.0,		0.0,  0.0,  1.0 },

	// BACK
	{ -0.5,  0.5, -0.5, 1.0,  	0.0,  1.0,  0.0,  1.0,  	1.0,  1.0,		0.0,  0.0, -1.0 },
	{  0.5, -0.5, -0.5, 1.0,  	0.0,  1.0,  0.0,  1.0,  	0.0,  0.0,		0.0,  0.0, -1.0 },
	{ -0.5, -0.5, -0.5, 1.0,  	0.0,  1.0,  0.0,  1.0,  	1.0,  0.0,		0.0,  0.0, -1.0 },

	{ -0.5,  0.5, -0.5, 1.0,  	0.0,  1.0,  0.0,  1.0,  	1.0,  1.0,		0.0,  0.0, -1.0 },
	{  0.5,  0.5, -0.5, 1.0,  	0.0,  1.0,  0.0,  1.0,  	0.0,  1.0,		0.0,  0.0, -1.0 },
	{  0.5, -0.5, -0.5, 1.0,  	0.0,  1.0,  0.0,  1.0,  	0.0,  0.0,		0.0,  0.0, -1.0 },

	// LEFT
	{  0.5,  0.5, -0.5, 1.0,  	0.0,  0.0,  1.0,  1.0,  	1.0,  1.0,		1.0,  0.0,  0.0 },
	{  0.5, -0.5,  0.5, 1.0,  	0.0,  0.0,  1.0,  1.0,  	0.0,  0.0,		1.0,  0.0,  0.0 },
	{  0.5, -0.5, -0.5, 1.0,  	0.0,  0.0,  1.0,  1.0,  	1.0,  0.0,		1.0,  0.0,  0.0 },

	{  0.5,  0.5, -0.5, 1.0,  	0.0,  0.0,  1.0,  1.0,  	1.0,  1.0,		1.0,  0.0,  0.0 },
	{  0.5,  0.5,  0.5, 1.0,  	0.0,  0.0,  1.0,  1.0,  	0.0,  1.0,		1.0,  0.0,  0.0 },
	{  0.5, -0.5,  0.5, 1.0,  	0.0,  0.0,  1.0,  1.0,  	0.0,  0.0,		1.0,  0.0,  0.0 },

	// RIGHT
	{ -0.5,  0.5, -0.5, 1.0,  	1.0,  1.0,  0.0,  1.0,  	0.0,  1.0,	   -1.0,  0.0,  0.0 },
	{ -0.5, -0.5, -0.5, 1.0,  	1.0,  1.0,  0.0,  1.0,  	0.0,  0.0,	   -1.0,  0.0,  0.0 },
	{ -0.5, -0.5,  0.5, 1.0,  	1.0,  1.0,  0.0,  1.0,  	1.0,  0.0,	   -1.0,  0.0,  0.0 },

	{ -0.5,  0.5, -0.5, 1.0,  	1.0,  1.0,  0.0,  1.0,  	0.0,  1.0,	   -1.0,  0.0,  0.0 },
	{ -0.5, -0.5,  0.5, 1.0,  	1.0,  1.0,  0.0,  1.0,  	1.0,  0.0,	   -1.0,  0.0,  0.0 },
	{ -0.5,  0.5,  0.5, 1.0,  	1.0,  1.0,  0.0,  1.0,  	1.0,  1.0,	   -1.0,  0.0,  0.0 },

	// TOP
	{ -0.5,  0.5, -0.5, 1.0,  	0.0,  1.0,  1.0,  1.0,  	0.0,  1.0,		0.0,  1.0,  0.0 },
	{ -0.5,  0.5,  0.5, 1.0,  	0.0,  1.0,  1.0,  1.0,  	0.0,  0.0,		0.0,  1.0,  0.0 },
	{  0.5,  0.5, -0.5, 1.0,  	0.0,  1.0,  1.0,  1.0,  	1.0,  1.0,		0.0,  1.0,  0.0 },

	{ -0.5,  0.5,  0.5, 1.0,  	0.0,  1.0,  1.0,  1.0,		0.0,  0.0,		0.0,  1.0,  0.0 },
	{  0.5,  0.5,  0.5, 1.0,  	0.0,  1.0,  1.0,  1.0,  	1.0,  0.0,		0.0,  1.0,  0.0 },
	{  0.5,  0.5, -0.5, 1.0,  	0.0,  1.0,  1.0,  1.0,  	1.0,  1.0,		0.0,  1.0,  0.0 },

	// BOTTOM
	{ -0.5, -0.5, -0.5, 1.0,  	1.0,  0.0,  1.0,  1.0,  	0.0,  0.0,		0.0, -1.0,  0.0 },
	{  0.5, -0.5, -0.5, 1.0,  	1.0,  0.0,  1.0,  1.0,  	1.0,  0.0,		0.0, -1.0,  0.0 },
	{ -0.5, -0.5,  0.5, 1.0,  	1.0,  0.0,  1.0,  1.0,  	0.0,  1.0,		0.0, -1.0,  0.0 },

	{ -0.5, -0.5,  0.5, 1.0,  	1.0,  0.0,  1.0,  1.0,  	0.0,  1.0,		0.0, -1.0,  0.0 },
	{  0.5, -0.5, -0.5, 1.0,  	1.0,  0.0,  1.0,  1.0,  	1.0,  0.0,		0.0, -1.0,  0.0 },
	{  0.5, -0.5,  0.5, 1.0,  	1.0,  0.0,  1.0,  1.0,  	1.0,  1.0,		0.0, -1.0,  0.0 }
};



// Reference variable for the buffer object
GLuint bufferObject;

void InitializeVertexBuffer()
{
	GLuint dataSize = sizeof(UnitCube);
	VertexData* data = UnitCube;

	// Create a buffer object
	glGenBuffers(
				1,						// Number of buffer objects to create: 1
				&bufferObject	// Where to store the reference to the buffer object
				);

	// Bind the buffer object
	glBindBuffer(
				GL_ARRAY_BUFFER,		// Bind the buffer object to the GL_ARRAY_BUFFER binding target
				bufferObject	// The buffer object to bind
				);

	// Allocate memory for a bound buffer and copy data into OpenGL memory
	glBufferData(
				GL_ARRAY_BUFFER,		// What context is the buffer bound to?
				dataSize,		// How much memory to allocate
				data,				// The actual data
				GL_STREAM_DRAW			// We write to the buffer each frame for animation. GL_STATIC_DRAW expects that we never change the buffer object, or at least very seldom.
				);

	// Unbind the buffer object
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//
void ComputePositionOffsets(float &fXOffset, float &fYOffset)
{
	const float fLoopDuration = 10.0f;
	const float fScale = 3.14159f * 2.0f / fLoopDuration;

	float fElapsedTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

	float fCurrTimeThroughLoop = fmodf(fElapsedTime, fLoopDuration);

	fXOffset = cosf(fCurrTimeThroughLoop * fScale) * 0.5f;
	fYOffset = sinf(fCurrTimeThroughLoop * fScale) * 0.5f;
}

/*
void AdjustVertexData(float fXOffset, float fYOffset)
{
	// Allocate a new array to hold adjusted vertex data
    std::vector<VertexData> fNewData(ARRAY_COUNT(vertexData));

    // Copy block of memory
    // void * memcpy ( void * destination, const void * source, size_t num );
	memcpy(&fNewData[0], vertexData, sizeof(vertexData));
    
    for(int iVertex = 0; iVertex < ARRAY_COUNT(vertexData); iVertex++)
    { 
        fNewData[iVertex].position[0] += fXOffset;
        fNewData[iVertex].position[1] += fYOffset;
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
    // glBufferSubData: Write the new data into an existing buffer object without reinitializing it
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertexData), &fNewData[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
*/
// Vertex Array Object
GLuint vao;

// 
void init()
{	
	// 
	InitializeProgram();
	// 
	InitializeVertexBuffer();

    // 
	glGenVertexArrays(1, &vao);
	// 
	glBindVertexArray(vao);


	// Enable backface culling with counter-clockwise triangles
	glEnable(GL_CULL_FACE);		// Enable culling
	glCullFace(GL_BACK);		// Cull faces facing away from camera (GL_FRONT / GL_BACK / GL_FRONT_AND_BACK)
	// NOTE: Figure out why the triangles face into the cube (probably something with the perspective transform).
	glFrontFace(GL_CCW);		// Triangles are defined counter-clockwise (GL_CW / GL_CCW)
	
	// Set the texture maps
	glUseProgram(theProgram);
		// TEXTURE MAP
		GLuint diffuseMapUniform = glGetUniformLocation(theProgram, "diffuseMap");
		glUniform1i(diffuseMapUniform, 0);

		GLuint diffuseMapID;
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &diffuseMapID); // Generate a unique texture ID
		glBindTexture(GL_TEXTURE_2D, diffuseMapID); // Activate the texture

		png_data_t *image = read_png((char*)"assets/photosculpt-graystonewall-diffuse.png");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 
		0, 
		image->has_alpha ? GL_RGBA : GL_RGB,
		image->width,
		image->height, 
		0, 
		image->has_alpha ? GL_RGBA : GL_RGB,
		GL_UNSIGNED_BYTE,
		image->pixelData);



		// NORMAL MAP
		GLuint normalMapUniform = glGetUniformLocation(theProgram, "normalMap");
		glUniform1i(normalMapUniform, 1);

		GLuint normalMapID;
		glActiveTexture(GL_TEXTURE1);
		glGenTextures(1, &normalMapID); // Generate a unique texture ID
		glBindTexture(GL_TEXTURE_2D, normalMapID); // Activate the texture

		image = read_png((char*)"assets/photosculpt-graystonewall-normal.png");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 
		0, 
		image->has_alpha ? GL_RGBA : GL_RGB,
		image->width,
		image->height, 
		0, 
		image->has_alpha ? GL_RGBA : GL_RGB,
		GL_UNSIGNED_BYTE,
		image->pixelData);


		// DISPLACEMENT MAP
		GLuint displacementMapUniform = glGetUniformLocation(theProgram, "displacementMap");
		glUniform1i(displacementMapUniform, 2);

		GLuint displacementMapID;
		glActiveTexture(GL_TEXTURE2);
		glGenTextures(1, &displacementMapID); // Generate a unique texture ID
		glBindTexture(GL_TEXTURE_2D, displacementMapID); // Activate the texture

		image = read_png((char*)"assets/photosculpt-graystonewall-displace.png");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 
		0, 
		image->has_alpha ? GL_RGBA : GL_RGB,
		image->width,
		image->height, 
		0, 
		image->has_alpha ? GL_RGBA : GL_RGB,
		GL_UNSIGNED_BYTE,
		image->pixelData);

	glUseProgram(0);
}

void display()
{

	// Set the default color of the viewport
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// Tell OpenGL to clear the viewport to the specified clear color
	glClear(GL_COLOR_BUFFER_BIT); // The glClear() call affects the color buffer

	// Tell OpenGL to user the shader program at "theProgram"
	glUseProgram(theProgram);

	// Send the offset values to the shader - move vertices in shader
	glUniform1f(timeUniform, glutGet(GLUT_ELAPSED_TIME) / 1000.0f);

	// Bind the buffer object
	glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
	
	// 
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	//fprintf(stderr, "%i\n", (int)sizeof(VertexData));

	// Tell OpenGL how the data in memory is formatted
	glVertexAttribPointer(
							0,			// 
							4,			// How many values represent a single piece of data?
							GL_FLOAT,	// What base type does the data have?
							GL_FALSE,	// 
							sizeof(VertexData),			// Spacing from start to start; 4*4*2; sizeof(float) * n_floats * stream_offset
							0			// At what byte offset does the data begin?
						);

	// Tell OpenGL how the data in memory is formatted
	glVertexAttribPointer(
							1,			// 
							4,			// How many values represent a single piece of data?
							GL_FLOAT,	// What base type does the data have?
							GL_FALSE,	// 
							sizeof(VertexData),			// How much spacing is there between each set of values?
							(void*)16	// The data begins at 4*4*1; float_size * n_floats * stream_offset
						);

	// Tell OpenGL how the data in memory is formatted
	glVertexAttribPointer(
							2,			// 
							2,			// How many values represent a single piece of data?
							GL_FLOAT,	// What base type does the data have?
							GL_FALSE,	// 
							sizeof(VertexData),			// How much spacing is there between each set of values?
							(void*)32	// The data begins at 4*4*1; float_size * n_floats * stream_offset
						);

	// Tell OpenGL how the data in memory is formatted
	glVertexAttribPointer(
							3,			// 
							3,			// How many values represent a single piece of data?
							GL_FLOAT,	// What base type does the data have?
							GL_FALSE,	// 
							sizeof(VertexData),			// How much spacing is there between each set of values?
							(void*)40	// The data begins at 4*4*1; float_size * n_floats * stream_offset
						);

	// Tell OpenGL to draw the contents of the vertex buffer
	glDrawArrays(
				GL_TRIANGLES,	// The vertex data should be assembled into triangles
				0,				// Begin to read at position
				36				// Number of values to read
				);

	// Clean up the OpenGL "workspace" where we've changed stuff
	glDisableVertexAttribArray(0);	// 
	glDisableVertexAttribArray(1);	// 
	glDisableVertexAttribArray(2);	// 
	glDisableVertexAttribArray(3);	// 
	glUseProgram(0);				// Unbind the shader program

	// We use double buffering, so glutSwapBuffers() shows the rendered image
	glutSwapBuffers();
	glutPostRedisplay();
}

void reshape(int w, int h)
{
	// 
	perspectiveMatrix[0] = frustumScale / (w / (float)h);
	perspectiveMatrix[5] = frustumScale;

	glUseProgram(theProgram);
		glUniformMatrix4fv(perspectiveMatrixUni, 1, GL_TRUE, perspectiveMatrix);
	glUseProgram(0);

	// Tell OpenGL what area of the available area we are rendering to
	// Note: This is bottom-left oriented, so (0,0) is at the bottom-left corner
	glViewport(
				0,				// Starting width-coordinate
				0,				// Starting height-coordinate
				(GLsizei) w,	// The width (here we use the whole window width)
				(GLsizei) h		// The height (here we use the whole window height)
			);
}

//Called whenever a key on the keyboard was pressed.
//The key is given by the ''key'' parameter, which is in ASCII.
//It's often a good idea to have the escape key (ASCII value 27) call glutLeaveMainLoop() to 
//exit the program.
void keyboard(unsigned char key, int x, int y)
{	
	switch (key)
	{
		case 27:
			exit(1);

		default:
			fprintf(stderr, "Key: %i\n", (int)key);
	}
}


// Diskutera denna skit!
int main(int argc, char *argv[]){

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);

	
	glutInitWindowPosition(850, 20);
	glutInitWindowSize(560, 315);
	glutCreateWindow("Demo");

	// ?!?!?!
	glewExperimental = GL_TRUE;
	glewInit();

	// 
	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	glutMainLoop();
}