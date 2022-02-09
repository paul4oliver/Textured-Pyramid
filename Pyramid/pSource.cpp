#include <iostream>         // Allow for input/output
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

// GLM Libraries
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp> 

// STB Library to load an image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Shader program Macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

using namespace std;

int width, height;

// Functions to create and complie shader program
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader);
static GLuint CompileShader(const string& source, GLuint shaderType);

// Input fucntions 
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);

// Function to reset camera
void initCamera();

// Vertex Shader Source Code
const GLchar* vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position;	// Declare attribute locations
	layout(location = 2) in vec2 textureCoordinate;

	out vec2 vertexTextureCoordinate;

	//Global variables for transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transform vertices to clip coordinates
	vertexTextureCoordinate = textureCoordinate;	
}
);

// Fragment Shader Source Code
const GLchar* fragmentShaderSource = GLSL(440,
	in vec2 vertexTextureCoordinate;

	out vec4 fragmentColor;

	uniform sampler2D texture0;	// Allow shader access to texture object

void main()
{
	fragmentColor = texture(texture0, vertexTextureCoordinate); // Send texture to GPU
}
);

// Delcare View matrix
glm::mat4 viewMatrix = glm::mat4(1.0f);

// Initialize field of view
GLfloat fov = 45.0f;

// Define camera attributes
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.5f, 3.0f);
glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraDirection = glm::normalize(cameraPosition - target);
glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
glm::vec3 cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

GLfloat yaw = -90.0f, pitch = 0.0f; // Pitch, yaw,
// Variables for scroll and cursor
GLfloat cameraMovement = 10.0f;		
GLfloat cameraSpeed = 0.5f;
// Variables to ensure application runs the same on all hardware
GLfloat delataTime = 0.0f, lastFrame = 0.0f;	
GLfloat lastX = 320, lastY = 240, xChange, yChange;
bool firstMouseMove = true;	// Detect initial mouse movement

int main(void)
{
	// Set values for screen dimensions
	width = 640; height = 480;

	// Declare new window object
	GLFWwindow* window;

	// Initialize glfw library 
	if (!glfwInit())
	{
		return -1;
	}

	// Create GLFW window
	window = glfwCreateWindow(width, height, "Textured 3D Pyramid by Paul K.", NULL, NULL);

	// Terminate program if window is not created
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// Set callback functions
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// Make context current for calling thread
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		cout << "Error!" << endl;
	}

	// Define vertex data for triangles that make the pyramid
	GLfloat vertices[] = {
		// position(x, y, z) and texture(x, y) coordinates
		-1.0f, 0.0f, -1.0f,		0.0f, 0.0f,	// Base Triangle 1
		-1.0f, 0.0f,  1.0f,		0.0f, 1.0f,
		 1.0f, 0.0f,  1.0f,		1.0f, 1.0f,

		 1.0f, 0.0f,  1.0f,		1.0f, 1.0f,    // Base Triangle 2
		 1.0f, 0.0f, -1.0f,		1.0f, 0.0f,
		-1.0f, 0.0f, -1.0f,		0.0f, 0.0f,

		-1.0f, 0.0f, -1.0f,		0.0f, 0.0f,    // Side 1
		-1.0f, 0.0f,  1.0f,		1.0f, 0.0f,
		 0.0f, 1.0f,  0.0f,		0.5f, 1.0f,

		-1.0f, 0.0f, -1.0f,		0.0f, 0.0f,    // Side 2
		 1.0f, 0.0f, -1.0f,		1.0f, 0.0f,
		 0.0f, 1.0f,  0.0f,		0.5f, 1.0f,

		 1.0f, 0.0f,  1.0f,		0.0f, 0.0f,    // Side 3
		 1.0f, 0.0f, -1.0f,		1.0f, 0.0f,
		 0.0f, 1.0f,  0.0f,		0.5f, 1.0f,

		-1.0f, 0.0f, 1.0f,		0.0f, 0.0f,     // Side 4
		 1.0f, 0.0f, 1.0f,		1.0f, 0.0f,
		 0.0f, 1.0f, 0.0f,		0.5f, 1.0f,
	};

	// Identify how many floats for Position and Texture coordinates
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerUV = 2;

	// Wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Create variables for reference
	GLuint VBO, VAO;

	// Create new Vertex Buffer Object, Vertex Array Object, and Element Buffer Object
	glGenBuffers(1, &VBO); // (# of buffers, where to store id for buffer when created)
	glGenVertexArrays(1, &VAO);

	// Activate VAO
	glBindVertexArray(VAO);

	// Activate Vertex Buffer Object and send data to GPU
	glBindBuffer(GL_ARRAY_BUFFER, VBO); 
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); 

	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV);

	// Tell GPU the arrangment (layout) of position attributes 
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);	// Tell GPU index (location) of attributes

	// Tell GPU the arrangment (layout) of texture attributes
	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2); // Tell GPU index (location) of attributes

	// Create shader program
	GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
	glUseProgram(shaderProgram); // Call Shader per-frame when updating attributes

	unsigned int texture0;
	glGenTextures(1, &texture0);				// Create texture ID
	glBindTexture(GL_TEXTURE_2D, texture0);		// Bind texture

	// Specify how to wrap texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Specify how to filter texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	stbi_set_flip_vertically_on_load(true);	// Flip y-axis during image loading so that image is not upside down
	int t_width, t_height, nrChannels; // Declare texture variables
	unsigned char* data = stbi_load("Brick.jpg", &t_width, &t_height, &nrChannels, 0); // Load image to data variable

	if (data) {
		// Generate texture - we use base formart RGB and bitdpeth per component 8
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, t_width, t_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D); // Generate all  required mipmaps for currently bound texture
	}
	else {
		cout << "Error: unable to load texture." << endl;
	}

	stbi_image_free(data);				// Free image memory
	glBindTexture(GL_TEXTURE_2D, 0);	// Unbind texture

	glUseProgram(shaderProgram); // // Set the shader to be used 
	glUniform1i(glGetUniformLocation(shaderProgram, "texture0"), 0);	// Assign texture unit

	//Set background color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Render loop (infinite loop until user closes window)
	while (!glfwWindowShouldClose(window))
	{
		// Set delta time
		GLfloat currentFrame = glfwGetTime();
		delataTime = currentFrame - lastFrame;  // Ensure we are transforming at consistent rate
		lastFrame = currentFrame;

		processInput(window);	// Call function to capture user input

		// Resize window and graphics simultaneously
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		
		glEnable(GL_DEPTH_TEST);	// Allows for depth comparisons and to update the depth buffer
		
		// Clear z and depth buffer and frame
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

		// Declare identity matrix
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 projectionMatrix = glm::mat4(1.0f);

		// Initialize transforms
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix = glm::rotate(modelMatrix, glm::degrees(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.7f, 0.7f, 0.7f));

		// lookAt functin used to create view matrix that transforms all world coordinates to view space
		viewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
		
		// Create perspective projection
		projectionMatrix = glm::perspective(fov, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
		
		glUseProgram(shaderProgram); // Set shader

		// Select uniform shader and variable
		GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

		// Pass transform to shader
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		
		// Activate texture unit
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture0);	// Bind texture to active texture unit

		// Activate VBOs contained in VAO
		glBindVertexArray(VAO);

		// Draw pyramid
		glDrawArrays(GL_TRIANGLES, 0, 18);

		// Deactivate VAO
		glBindVertexArray(0);
		
		// Deactivate program object
		glUseProgram(0);

		// Swap front and back buffers of window
		glfwSwapBuffers(window);

		// Process events
		glfwPollEvents();
	}

	// Delete Vertex Array Object, Vertex Buffer Object, and Element Buffer Object
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	glGenTextures(1, &texture0); //Release texture

	// End program
	glfwTerminate();
	return 0;
}

// Function to process user keyboard input
void processInput(GLFWwindow* window)
{
	cameraSpeed = cameraMovement * delataTime;					// Calculate camera speed based on scroll

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)		// Exit application if escape key pressed	
	{
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)			// If 'W' pressed, move camera forward (toward object)	
	{
		cameraPosition += cameraSpeed * cameraFront;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)			// If 'S' pressed, move camera backward (away from object)	
	{
		cameraPosition -= cameraSpeed * cameraFront;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)			// If 'A' pressed, move camera left	
	{
		cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)			// If 'D' pressed, move camera right	
	{
		cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)			// If 'Q' pressed, move camera down	
	{
		cameraPosition -= cameraSpeed * cameraUp;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)			// If 'E' pressed, move camera up	
	{
		cameraPosition += cameraSpeed * cameraUp;
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)			// If 'F' pressed, call function to reset camera
	{
		initCamera();
	}
}

// Control speed at which camera moves with scroll; Adjust the speed of the movement
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Clamp cameraMovement
	if (cameraMovement >= 1.0f && cameraMovement <= 55.0f)
	{
		cameraMovement -= yoffset;
	}
	// Default cameraMovement
	if (cameraMovement < 1.0f)
	{
		cameraMovement = 1.0f;
	}
	if (cameraMovement > 55.0f)
	{
		cameraMovement = 55.0f;
	}
}

// Allows to change the orientation of the camera
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	// Update initial mouse positions if this is the first move from mouse
	if (firstMouseMove)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouseMove = false;
	}

	// Calculate cursor offset
	xChange = xpos - lastX;
	yChange = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	// Lessen sensitivity of mouse movement
	GLfloat sensitivity = 0.5f;
	xChange *= sensitivity;
	yChange *= sensitivity;

	// Add ofset values to global yaw and pitch
	yaw += xChange;
	pitch += yChange;

	// Prevent screen from flipping
	if (pitch > 89.0f)
	{
		pitch = 89.0f;
	}
	if (pitch < -89.0f)
	{
		pitch = -89.0f;
	}

	// Calculate actual direction vector to contain rotations from mouse movement
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

// Function with coordinates to reset camera to look at pyramid
void initCamera()
{
	cameraPosition = glm::vec3(0.0f, 0.5f, 3.0f);
	target = glm::vec3(0.0f, 0.0f, 0.0f);
	cameraDirection = glm::normalize(cameraPosition - target);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	cameraRight = glm::normalize(glm::cross(worldUp, cameraDirection));
	cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
	cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
	firstMouseMove = true;
}

// Create and compile shaders
static GLuint CompileShader(const string& source, GLuint shaderType)
{
	// Create shader object
	GLuint shaderID = glCreateShader(shaderType);
	const char* src = source.c_str();

	glShaderSource(shaderID, 1, &src, nullptr);	// Attach source code to shader object
	glCompileShader(shaderID);	// Compile shader

	return shaderID;	// Return shader ID
}

// Create program object to link shader objects 
static GLuint CreateShaderProgram(const string& vertexShader, const string& fragmentShader)
{
	// Compile vertex shader
	GLuint vertexShaderComp = CompileShader(vertexShader, GL_VERTEX_SHADER);

	// Compile fragment shader
	GLuint fragmentShaderComp = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

	GLuint shaderProgram = glCreateProgram();	// Create program object

	// Attach compiled vertex and fragment shaders to program object
	glAttachShader(shaderProgram, vertexShaderComp);
	glAttachShader(shaderProgram, fragmentShaderComp);

	glLinkProgram(shaderProgram);	// Link shaders to create final executable shader program

	// Delete vertex and fragment shaders
	glDeleteShader(vertexShaderComp);
	glDeleteShader(fragmentShaderComp);

	return shaderProgram;	// Return shader Program
}