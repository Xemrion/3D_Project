//--------------------------------------------------------------------------------------
// BTH - Stefan Petersson 2014.
//     - updated by FLL
//--------------------------------------------------------------------------------------
#include <windows.h>

#include <string>
#include <fstream>
#include <streambuf>
#include <ctime>

#include <GL\glm\glm\glm.hpp>
#include <GL\glm\glm\gtc\type_ptr.hpp>
#include <GL\glm\glm\gtc\matrix_transform.hpp>
#include <gl/glew.h>
#include <gl/GL.h>

#include "bth_image.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")

using namespace std;

//vec4(frag_color, 1.0f)
//texture(texture_sampler, texture_coord_geo)


HWND InitWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HGLRC CreateOpenGLContext(HWND wndHandle);

// OpenGL uses unsigned integers to keep track of
// created resources (shaders, vertices, textures, etc)
// For simplicity, we make them global here, but it is
// safe to put them in a class and pass around...
GLuint gVertexBuffer = 0;
GLuint gVertexAttribute = 0;
GLuint gShaderProgram = 0;

//=================================================================================================================================
//Matrixes
glm::mat4 world = glm::mat4(1.0f);
glm::mat4 groundWorld = glm::mat4(1.0f);

glm::mat4 view = glm::lookAt(
	glm::vec3(2.0f, 2.0f, 4.0f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));

glm::mat4 projection = glm::perspective(glm::radians(45.0f), float(640 / 480), 0.1f, 20.0f);
//=================================================================================================================================
// macro that returns "char*" with offset "i"
// BUFFER_OFFSET(5) transforms in "(char*)nullptr+(5)"
#define BUFFER_OFFSET(i) ((char *)nullptr + (i))

void CreateShaders()
{
	// local buffer to store error strings when compiling.
	char buff[1024]; 
	memset(buff, 0, 1024);
	GLint compileResult = 0;

	//#############################################################################################################
	//create vertex shader "name" and store it in "vs"
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);

	// open .glsl file and put it in a string
	ifstream shaderFile("VertexShader.glsl");
	std::string shaderText((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	shaderFile.close();

	// glShaderSource requires a double pointer.
	// get the pointer to the c style string stored in the string object.
	const char* shaderTextPtr = shaderText.c_str();
	
	// ask GL to use this string a shader code source
	glShaderSource(vs, 1, &shaderTextPtr, nullptr);

	// try to compile this shader source.
	glCompileShader(vs);

	// check for compilation error
	glGetShaderiv(vs, GL_COMPILE_STATUS, &compileResult);
	if (compileResult == GL_FALSE) {
		// query information about the compilation (nothing if compilation went fine!)
		glGetShaderInfoLog(vs, 1024, nullptr, buff);
		// print to Visual Studio debug console output
		OutputDebugStringA(buff);
	}
	//#############################################################################################################
	// repeat process for Fragment Shader (or Pixel Shader)
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	shaderFile.open("Fragment.glsl");
	shaderText.assign((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	shaderFile.close();
	shaderTextPtr = shaderText.c_str();
	glShaderSource(fs, 1, &shaderTextPtr, nullptr);
	glCompileShader(fs);
	// query information about the compilation (nothing if compilation went fine!)
	compileResult = GL_FALSE;
	glGetShaderiv(fs, GL_COMPILE_STATUS, &compileResult);
	if (compileResult == GL_FALSE) {
		// query information about the compilation (nothing if compilation went fine!)
		memset(buff, 0, 1024);
		glGetShaderInfoLog(fs, 1024, nullptr, buff);
		// print to Visual Studio debug console output
		OutputDebugStringA(buff);
	}
	//#############################################################################################################

	GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
	shaderFile.open("geometryShader.glsl");
	shaderText.assign((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	shaderFile.close();
	shaderTextPtr = shaderText.c_str();
	glShaderSource(geometryShader, 1, &shaderTextPtr, nullptr);
	glCompileShader(geometryShader);
	// query information about the compilation (nothing if compilation went fine!)
	compileResult = GL_FALSE;
	glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &compileResult);
	if (compileResult == GL_FALSE)
	{
		// query information about the compilation (nothing if compilation went fine!)
		memset(buff, 0, 1024);
		glGetShaderInfoLog(geometryShader, 1024, nullptr, buff);
		// print to Visual Studio debug console output
		OutputDebugStringA(buff);
	}
	//#############################################################################################################
	//link shader program (connect vs and ps)
	gShaderProgram = glCreateProgram();
	glAttachShader(gShaderProgram, fs);
	glAttachShader(gShaderProgram, vs);
	glAttachShader(gShaderProgram, geometryShader);
	glLinkProgram(gShaderProgram);

	// check once more, if the Vertex Shader and the Fragment Shader can be used
	// together
	compileResult = GL_FALSE;
	glGetProgramiv(gShaderProgram, GL_LINK_STATUS, &compileResult);
	if (compileResult == GL_FALSE) {
		// query information about the compilation (nothing if compilation went fine!)
		memset(buff, 0, 1024);
		glGetProgramInfoLog(gShaderProgram, 1024, nullptr, buff);
		// print to Visual Studio debug console output
		OutputDebugStringA(buff);
	}
	// in any case (compile sucess or not), we only want to keep the 
	// Program around, not the shaders.
	glDetachShader(gShaderProgram, vs);
	glDetachShader(gShaderProgram, fs);
	glDetachShader(gShaderProgram, geometryShader);
	glDeleteShader(vs);
	glDeleteShader(fs);
	glDeleteShader(geometryShader);
}

void CreateTriangleData()
{
	// this is how we will structure the input data for the vertex shader
	// every six floats, is one vertex.
	struct TriangleVertex
	{
		float x, y, z;
		float r, g, b;
		float s, t;
		float id;
	};

	// create the actual data in plane Z = 0
	// This is called an Array of Structs (AoS) because we will 
	// end up with an array of many of these structs.
	TriangleVertex triangleVertices[42] =
	{
		//Face 1 Green
		// pos                color               texture coordinates for each vertex
		{  0.5f, -0.5f, 0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f },  //4
		{  0.5f,  0.5f, 0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 1.0f }, //2
		{ -0.5f, -0.5f, 0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 1.0f },  //3

		{  0.5f,  0.5f, 0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 1.0f }, //2
		{ -0.5f,  0.5f, 0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f },  //1
		{ -0.5f, -0.5f, 0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 1.0f },  //3

		//###################################################
		//Face 2 Red
		// pos                color               texture coordinates for each vertex
		{ -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f }, //7
		{  0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f }, //6
		{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 1.0f }, //8
		
		{ -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f }, //7
		{ -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 1.0f }, //5
		{  0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f }, //6

		//###################################################
		//Face 3 Blue
		// pos                color               texture coordinates for each vertex
		{  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f }, //2
		{  0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 1.0f }, //6
		{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f }, //5
		
		{ -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 1.0f },  //1
		{  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f },  //2
		{ -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f },  //5
		
		//###################################################
		//Face 4 Yellow
		// pos                color               texture coordinates for each vertex
		{ -0.5f,   0.5f,  0.5f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f, 1.0f }, //1
		{ -0.5f,   0.5f, -0.5f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f }, //5
		{ -0.5f,  -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f, 1.0f }, //7

		{ -0.5f,  -0.5f,  0.5f,    1.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f },  //3
		{ -0.5f,   0.5f,  0.5f,    1.0f, 1.0f, 0.0f,  1.0f, 0.0f, 1.0f },  //1
		{ -0.5f,  -0.5f, -0.5f,    1.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f },  //7

		//###################################################
		//Face 5 Purple
		// pos                color               texture coordinates for each vertex   ID
		
		{  0.5f,  -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,   1.0f, 0.0f, 1.0f }, //4
		{  0.5f,  -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f }, //8
		{ -0.5f,  -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,   0.0f, 1.0f, 1.0f }, //7
		
		{ -0.5f,  -0.5f,  0.5f,    1.0f, 0.0f, 1.0f,  0.0f, 0.0f , 1.0f },  //3
		{  0.5f,  -0.5f,  0.5f,    1.0f, 0.0f, 1.0f,  1.0f, 0.0f , 1.0f },  //4
		{ -0.5f,  -0.5f, -0.5f,    1.0f, 0.0f, 1.0f,  0.0f, 1.0f , 1.0f },  //7
		
		//##################################################
		//Face 6 White
		// pos                color               texture coordinates for each vertex
		{ 0.5f,  -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,   1.0f, 1.0f, 1.0f }, //8
		{ 0.5f,   0.5f, -0.5f,   1.0f, 1.0f, 1.0f,   1.0f, 0.0f, 1.0f }, //6
		{ 0.5f,   0.5f,  0.5f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f }, //2

		{ 0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f },  //8
		{ 0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f },  //2
		{ 0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 1.0f,  0.0f, 1.0f, 1.0f },  //4
		
		//##################################################
		//ground Orange
		// pos                color               texture coordinates for each vertex
		{  3.0f,  -0.5f,  3.0f,   1.0f, 0.5f, 0.0f,   1.0f, 1.0f, 2.0f }, //4
		{  3.0f,  -0.5f, -3.0f,   1.0f, 0.5f, 0.0f,   1.0f, 0.0f, 2.0f }, //8
		{ -3.0f,  -0.5f, -3.0f,   1.0f, 0.5f, 0.0f,   0.0f, 0.0f, 2.0f }, //7

		{ -3.0f, -0.5f,   3.0f,   1.0f, 0.5f, 0.0f,   0.0f, 1.0f, 2.0f },  //3
		{  3.0f, -0.5f,   3.0f,   1.0f, 0.5f, 0.0f,   1.0f, 1.0f, 2.0f },  //4
		{ -3.0f, -0.5f,  -3.0f,   1.0f, 0.5f, 0.0f,   0.0f, 0.0f, 2.0f },  //7
	};
	//=================================================================================================================================0
	// Vertex Array Object (VAO), description of the inputs to the GPU 
	glGenVertexArrays(1, &gVertexAttribute);
	// bind is like "enabling" the object to use it
	glBindVertexArray(gVertexAttribute);

	// this activates the first and second attributes of this VAO
	// think of "attributes" as inputs to the Vertex Shader
	glEnableVertexAttribArray(0); 
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	// create a vertex buffer object (VBO) id (out Array of Structs on the GPU side)
	glGenBuffers(1, &gVertexBuffer);

	// Bind the buffer ID as an ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);

	// This "could" imply copying to the GPU, depending on what the driver wants to do, and
	// the last argument (read the docs!)
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

	// query which "slot" corresponds to the input vertex_position in the Vertex Shader 
	GLint vertexPos = glGetAttribLocation(gShaderProgram, "vertex_position");
	// if this returns -1, it means there is a problem, and the program will likely crash.
	// examples: the name is different or missing in the shader

	if (vertexPos == -1) {
		OutputDebugStringA("Error, cannot find 'vertex_position' attribute in Vertex shader\n");
		return;
	}

	// tell OpenGL about layout in memory (input assembler information)
	glVertexAttribPointer(
		vertexPos,				// location in shader
		3,						// how many elements of type (see next argument)
		GL_FLOAT,				// type of each element
		GL_FALSE,				// integers will be normalized to [-1,1] or [0,1] when read...
		sizeof(TriangleVertex), // distance between two vertices in memory (stride)
		BUFFER_OFFSET(0)		// offset of FIRST vertex in the list.
	);

	// repeat the process for the second attribute.
	// query which "slot" corresponds to the input vertex_color in the Vertex Shader 
	GLuint vertexColor = glGetAttribLocation(gShaderProgram, "vertex_color");
	glVertexAttribPointer(
		vertexColor, 
		3, 
		GL_FLOAT, 
		GL_FALSE, 
		sizeof(TriangleVertex), // distance between two vertexColor 
		BUFFER_OFFSET(sizeof(float)*3)	// note, the first color starts after the last vertex.
	);

	GLuint vertexCoord = glGetAttribLocation(gShaderProgram, "texture_coord");
	glVertexAttribPointer(
		vertexCoord,
		2,
		GL_FLOAT,
		GL_FALSE,
		sizeof(TriangleVertex), // distance between two vertexCoords 
		BUFFER_OFFSET(sizeof(float) * 6)	// note, the first coordinate starts after the last color.
	);

	GLuint id = glGetAttribLocation(gShaderProgram, "OBJid");
	glVertexAttribPointer(
		id,
		1,
		GL_FLOAT,
		GL_FALSE,
		sizeof(TriangleVertex), // distance between two vertexCoords 
		BUFFER_OFFSET(sizeof(float) * 8)	// note, the id starts after the last textureCoord.
	);
	//=================================================================================================================================

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BTH_IMAGE_WIDTH, BTH_IMAGE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, BTH_IMAGE_DATA);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void SetViewport()
{
	// usually (not necessarily) this matches with the window size
	glViewport(0, 0, 640, 480);
}

void Render()
{

	GLint World = glGetUniformLocation(gShaderProgram, "world");
	glUniformMatrix4fv(World, 1, GL_FALSE, &world[0][0]);

	GLint GroundWorld = glGetUniformLocation(gShaderProgram, "groundWorld");
	glUniformMatrix4fv(GroundWorld, 1, GL_FALSE, &groundWorld[0][0]);

	GLint Projection = glGetUniformLocation(gShaderProgram, "projection");
	glUniformMatrix4fv(Projection, 1, GL_FALSE, &projection[0][0]);

	GLint View = glGetUniformLocation(gShaderProgram, "view");
	glUniformMatrix4fv(View, 1, GL_FALSE, &view[0][0]);

	// set the color TO BE used (this does not clear the screen right away)
	glClearColor(0, 0, 0, 1);

	// use the color to clear the color buffer (clear the color buffer only)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// tell opengl we want to use the gShaderProgram
	glUseProgram(gShaderProgram);

	// tell opengl we are going to use the VAO we described earlier
	glBindVertexArray(gVertexAttribute);
	
	// ask OpenGL to draw 3 vertices starting from index 0 in the vertex array 
	// currently bound (VAO), with current in-use shader. Use TOPOLOGY GL_TRIANGLES,
	// so for one triangle we need 3 vertices!

	glDrawArrays(GL_TRIANGLES, 0, 42);
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	clock_t t = clock();
	float dt = t;
	float oldDt = dt;

	MSG msg = { 0 };
	HWND wndHandle = InitWindow(hInstance); // 1. Skapa fönster
	
	//world = glm::rotate(world,0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
	if (wndHandle)
	{
		HDC hDC = GetDC(wndHandle);
		HGLRC hRC = CreateOpenGLContext(wndHandle); //2. Skapa och koppla OpenGL context

		glewInit(); //3. Initiera The OpenGL Extension Wrangler Library (GLEW)

		// which OpenGL did we get?
		GLint glMajor, glMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
		glGetIntegerv(GL_MAJOR_VERSION, &glMinor);
		{
			char buff[64] = {};
			sprintf(buff, "OpenGL context version %d.%d created\n", glMajor, glMinor);
			OutputDebugStringA(buff);
		}

		SetViewport(); //4. Sätt viewport

		CreateShaders(); //5. Skapa vertex- och fragment-shaders

		CreateTriangleData(); //6. Definiera triangelvertiser, 7. Skapa vertex buffer object (VBO), 8.Skapa vertex array object (VAO)
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		ShowWindow(wndHandle, nCmdShow);

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				t = clock();
				oldDt = dt;
				dt = t;
				
				world = glm::rotate(world, (0.05f * ((dt-oldDt)/40)), glm::vec3(0.0f, 1.0f, 0.0f));
				Render(); //9. Rendera

				SwapBuffers(hDC); //10. Växla front- och back-buffer
			}
		}

		// release OpenGL context
		wglMakeCurrent(NULL, NULL);
		// release device context handle
		ReleaseDC(wndHandle, hDC);
		// delete context
		wglDeleteContext(hRC);
		// kill window
		DestroyWindow(wndHandle);
	}

	return (int) msg.wParam;
}

// Win32 specific code. Create a window with certain characteristics
HWND InitWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.hInstance      = hInstance;
	wcex.lpszClassName = L"BTH_GL_DEMO";
	if( !RegisterClassEx(&wcex) )
		return false;

	// window size
	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
	
	// win32 call to create a window
	HWND handle = CreateWindow(
		L"BTH_GL_DEMO",
		L"BTH OpenGL Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);
	return handle;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch (message) 
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;		
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HGLRC CreateOpenGLContext(HWND wndHandle)
{
	//get handle to a device context (DC) for the client area
	//of a specified window or for the entire screen
	HDC hDC = GetDC(wndHandle);

	//details: http://msdn.microsoft.com/en-us/library/windows/desktop/dd318286(v=vs.85).aspx
	static  PIXELFORMATDESCRIPTOR pixelFormatDesc =
	{
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd  
		1,                                // version number  
		PFD_DRAW_TO_WINDOW |              // support window  
		PFD_SUPPORT_OPENGL |              // support OpenGL  
		PFD_DOUBLEBUFFER ,                // double buffered
		//PFD_DEPTH_DONTCARE,               // disable depth buffer <-- added by Stefan
		PFD_TYPE_RGBA,                    // RGBA type  
		32,                               // 32-bit color depth (4*8)  
		0, 0, 0, 0, 0, 0,                 // color bits ignored  
		0,                                // no alpha buffer  
		0,                                // shift bit ignored  
		0,                                // no accumulation buffer  
		0, 0, 0, 0,                       // accum bits ignored  
		24,    // 24                       // 0-bits for depth buffer <-- modified by Stefan      
		0,    // 8                        // no stencil buffer  
		0,                                // no auxiliary buffer  
		PFD_MAIN_PLANE,                   // main layer  
		0,                                // reserved  
		0, 0, 0                           // layer masks ignored  
	};

	//attempt to match an appropriate pixel format supported by a
	//device context to a given pixel format specification.
	int pixelFormat = ChoosePixelFormat(hDC, &pixelFormatDesc);

	//set the pixel format of the specified device context
	//to the format specified by the iPixelFormat index.
	SetPixelFormat(hDC, pixelFormat, &pixelFormatDesc);

	//create a new OpenGL rendering context, which is suitable for drawing
	//on the device referenced by hdc. The rendering context has the same
	//pixel format as the device context.
	HGLRC hRC = wglCreateContext(hDC);
	
	//makes a specified OpenGL rendering context the calling thread's current
	//rendering context. All subsequent OpenGL calls made by the thread are
	//drawn on the device identified by hdc. 
	wglMakeCurrent(hDC, hRC);

	return hRC;
}