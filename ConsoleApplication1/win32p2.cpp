#define _CRT_SECURE_NO_WARNINGS

// Std. Includes
#include <string>
#include <algorithm>
#include <ctime>
using namespace std;

// GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GL includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H

// Other Libs
#include <SOIL/SOIL.h>

// Audio
#include <inc/fmod.h>

#include <windows.h>

//ljt
#include "particle_system_tf.h"
//#include "texture.h"

#define START_COUNT 60
#define CARCOUNT  500
#define carcount1  6

//for Collision detection
int carpass;
double carx;

// Properties
GLuint screenWidth = 800, screenHeight = 600;
const GLuint WIDTH = 800, HEIGHT = 600;
bool startGame;
bool gameOver;

// Light attributes
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
// Positions of the point lights
glm::vec3 pointLightPositions[] = {
	glm::vec3(0.7f,  0.2f,  2.0f),
	glm::vec3(2.3f, -3.3f, -4.0f),
	glm::vec3(-4.0f,  2.0f, -12.0f),
	glm::vec3(0.0f,  0.0f, -3.0f)
};

// Function prototypes
void setupLight(Shader &shader);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void Do_Movement();
void move_project(glm::mat4 &model, glm::mat4 &modelPar);
GLuint loadTexture(GLchar* path);
GLuint loadCubemap(vector<const GLchar*> faces);

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Audio
FSOUND_SAMPLE* bgm_handle;

// Global Speed
float globalSpeed = 30.0f;

// holds all state information relevant to a character as loaded using freetype
struct Character {
	GLuint TextureID;   // id handle of the glyph texture
	glm::ivec2 Size;    // size of glyph
	glm::ivec2 Bearing;  // offset from baseline to left/top of glyph
	GLuint Advance;    // horizontal offset to advance to next glyph
};

std::map<GLchar, Character> characters;
GLuint textVAO, textVBO;

void RenderText(Shader &shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color);

//ljt
clock_t tLastFrame;
float fFrameInterval;
void UpdateTimer() {
	clock_t tCur = clock();
	fFrameInterval = float(tCur - tLastFrame) / float(CLOCKS_PER_SEC);
	tLastFrame = tCur;
}
float sof(float fVal)
{
	return fVal*fFrameInterval;
}

// The MAIN function, from here we start our application and run our Game loop
int main()
{
	srand(time(0));

	cout << "init GLFW" << endl;
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "LearnOpenGL", nullptr, nullptr); // Windowed
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Options
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize GLEW to setup the OpenGL Function pointers
	glewExperimental = GL_TRUE;
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, screenWidth, screenHeight);

	// Setup some OpenGL options
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);


	// Setup and compile our shaders
	Shader shader("shaders/advanced.vs", "shaders/advanced.frag");
	Shader skyboxShader("shaders/skybox.vs", "shaders/skybox.frag");
	Shader roadShader("shaders/road.vs", "shaders/road.frag");
	//Shader carShader("shaders/car.vs", "shaders/car.frag");
	 Shader carShader("shaders/car_with_light.vs", "shaders/car_with_light.frag");
	Shader normalShader("shaders/normals.vs", "shaders/normals.frag", "shaders/normals.gs");
	Shader stone("shaders/instancing.vs", "shaders/instancing.frag");

	cout << "load shader success!" << endl;

	// text shader setup
	Shader textShader("shaders/text.vs", "shaders/text.frag");
	glm::mat4 text_projection = glm::ortho(0.0f, static_cast<GLfloat>(WIDTH), 0.0f, static_cast<GLfloat>(HEIGHT));

	// FreeType
	FT_Library ft;
	// All functions return a value different than 0 whenever an error occurred
	if (FT_Init_FreeType(&ft))
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

	// Load font as face
	FT_Face face;
	if (FT_New_Face(ft, "fonts/arial.ttf", 0, &face))
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

	// Set size to load glyphs as
	FT_Set_Pixel_Sizes(face, 0, 48);

	// Disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load first 128 characters of ASCII set
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
			);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		characters.insert(std::pair<GLchar, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	// Destroy FreeType once we're finished
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// Configure VAO/VBO for texture quads
	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	cout << "init text success " << endl;



#pragma region "object_initialization"
	// Set the object data (buffers, vertex attributes)	
	GLfloat skyboxVertices[] = {
		// Positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f
	};

	GLfloat roadVertices[] = {
		-1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f
	};

	// Setup road VAO
	GLuint roadVAO, roadVBO;
	glGenVertexArrays(1, &roadVAO);
	glGenBuffers(1, &roadVBO);
	glBindVertexArray(roadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, roadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(roadVertices), &roadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glBindVertexArray(0);

	// Setup skybox VAO
	cout << "Setup skybox VAO" << endl;
	GLuint skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glBindVertexArray(0);

	// Load textures
	cout << "load textures" << endl;
	GLuint cubeTexture = loadTexture("textures/wall.jpg");
	GLuint roadTexture = loadTexture("textures/road.jpg");
	//ljt
	GLuint partilceTexture = loadTexture("data/textures/particle.bmp");

	//ljt
	CParticleSystemTransformFeedback psMainParticleSystem;

	psMainParticleSystem.InitalizeParticleSystem();

	psMainParticleSystem.SetGeneratorProperties(
		///	glm::vec3(-10.0f, 17.5f, 0.0f), // Where the particles are generated
		glm::vec3(0.0f, -1.0f, 0.0f), // Where the particles are generated
		glm::vec3(-5, -5, 0), // Minimal velocity
		glm::vec3(5, 5, 20), // Maximal velocity
		glm::vec3(0, -5, 0), // Gravity force applied to particles
		glm::vec3(0.0f, 0.5f, 1.0f), // Color (light blue)
		1.5f, // Minimum lifetime in seconds
		3.0f, // Maximum lifetime in seconds
			  //0.75f, // Rendered size
		0.75f, // Rendered size
		0.02f, // Spawn every 0.05 seconds
		30); // And spawn 30 particles

#pragma endregion

	// Cubemap (Skybox)
	cout << "load skybox" << endl;
	vector<const GLchar*> faces;
	faces.push_back("skybox/purplenebula_rt.jpg");
	faces.push_back("skybox/purplenebula_lf.jpg");
	faces.push_back("skybox/purplenebula_up.jpg");
	faces.push_back("skybox/purplenebula_dn.jpg");
	faces.push_back("skybox/purplenebula_bk.jpg");
	faces.push_back("skybox/purplenebula_ft.jpg");
	GLuint cubemapTexture = loadCubemap(faces);

	// Draw as wireframe
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// load model
	//Model myCar("model/Tank_noGun.obj");
	Model myCar("model2/Mach-5.obj");
	Model otherCar = myCar;
	Model rock("objects/rock.obj");

	glm::mat4 stone_projection = glm::perspective(45.0f, (GLfloat)screenWidth / (GLfloat)screenHeight, 1.0f, 10000.0f);
	stone.Use();
	glUniformMatrix4fv(glGetUniformLocation(stone.Program, "projection"), 1, GL_FALSE, glm::value_ptr(stone_projection));

	// Game loop
	cout << "start game loop" << endl;
	startGame = true;
	gameOver = false;
	int gameOverStay = 60;

	// myCar's model matrix;
	carx = 0;
	glm::mat4 model;
	glm::mat4 model2;
	//ljt
	glm::mat4 myScalingMatrix = glm::scale(glm::mat4(), glm::vec3(0.01, 0.01, 0.01));
	myScalingMatrix = glm::translate(myScalingMatrix, glm::vec3(0.0f, -85.0f, 0.0f));
	glm::mat4 modelParticle = glm::scale(glm::mat4(), glm::vec3(0.5, 0.5, 0.5));
	modelParticle = modelParticle*model;
	
	//model = myScalingMatrix * model;
	model2 = myScalingMatrix * model;
	myScalingMatrix = glm::rotate(myScalingMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = myScalingMatrix * model;
	
	// road's model matrix
	glm::mat4 roadModel[10];
	glm::mat4 myTranslate;

	int carCount = CARCOUNT;
	int roadCount = START_COUNT;

	carpass = -6;
	double carsx[carcount1], carsz[carcount1];
	for (int i = 0; i<carcount1; i++)
		carsz[i] = 1000;
	// otherCar's model matrix
	glm::mat4 otherCarModel[carcount1];
	int otherCarSpeed[carcount1];

	//     rain of stone
	GLuint stone_amount = 200;
	glm::mat4* stone_modelMatrices;
	stone_modelMatrices = new glm::mat4[stone_amount];
	srand(glfwGetTime()); // initialize random seed	
	GLfloat radius = 50.0;
	GLfloat offset = 2.5f;
	for (GLuint i = 0; i < stone_amount; i++)
	{
		glm::mat4 stone_model;
		// 1. Translation: displace along circle with 'radius' in range [-offset, offset]
		GLfloat angle = (GLfloat)i / (GLfloat)stone_amount * 360.0f;
		GLfloat displacement = (rand() % (GLint)(2 * offset * 20)) / 100.0f - offset;
		GLfloat x = sin(angle) * radius + displacement;
		displacement = (rand() % (GLint)(2 * offset * 100)) / 100.0f - offset;
		GLfloat y = displacement * 0.4f; // Keep height of asteroid field smaller compared to width of x and z
		displacement = (rand() % (GLint)(2 * offset * 100)) / 100.0f - offset;
		GLfloat z = cos(angle) * radius + displacement;
		stone_model = glm::translate(stone_model, glm::vec3(x, y, z));

		// 2. Scale: Scale between 0.05 and 0.25f
		GLfloat scale = (rand() % 20) / 100.0f + 0.05;
		stone_model = glm::scale(stone_model, glm::vec3(scale));

		// 3. Rotation: add random rotation around a (semi)randomly picked rotation axis vector
		GLfloat rotAngle = (rand() % 360);
		stone_model = glm::rotate(stone_model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		// 4. Now add to list of matrices
		stone_modelMatrices[i] = stone_model;
	}
	int stone_count = 0;


	// 初始化
	FSOUND_Init(44100, 32, 0);
	// 装载和播放
	bgm_handle = FSOUND_Sample_Load(0, "sound/bgm.mp3", 0, 0, 0);
	FSOUND_PlaySound(0, bgm_handle);


	while (!glfwWindowShouldClose(window))
	{
		
		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// road Speed (as well as myCar's speed)
		GLfloat roadSpeed = globalSpeed * deltaTime;

		// Check and call events
		glfwPollEvents();
		Do_Movement();
		if (!startGame)
			continue;

		// Clear buffers
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw myCar
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(camera.Zoom, (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
		move_project(model, modelParticle);

		carShader.Use();
		setupLight(carShader);
		glUniformMatrix4fv(glGetUniformLocation(carShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(carShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(carShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		if (gameOver)
		{
			// Now set transformation matrices for drawing normals
			normalShader.Use();
			glUniformMatrix4fv(glGetUniformLocation(normalShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(normalShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
			//model = glm::mat4();
			glUniformMatrix4fv(glGetUniformLocation(normalShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));

			// And draw model again, this time only drawing normal vectors using the geometry shaders (on top of previous model)
			myCar.Draw(normalShader);
			gameOverStay--;
			if (gameOverStay == 0)
			{
				//break;
			}
				
		}
		else
			myCar.Draw(carShader);
		

		// Draw stone
		stone_count++;
		if (stone_count == 99999999)
			stone_count = 0;
		for (GLuint i = 0; i < stone_amount; i++)
		{
			if (stone_count % 1000 == rand() % 1000)
				stone_modelMatrices[i] = glm::translate(stone_modelMatrices[i], glm::vec3(0.0f, 0.0f, float(stone_count)));
			stone_modelMatrices[i] = glm::translate(stone_modelMatrices[i], glm::vec3(0.0f, 0.0f, -1.0f));
			glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(stone_modelMatrices[i]));
			rock.Draw(shader);
		}

		// Draw other car
		carShader.Use();	
		for (int i = 0; i<carcount1; i++) {
			if (carsz[i]>800) {
				carpass++;
				carsx[i] = rand() % 126;
				carsz[i] = -5000;
				carsx[i] -= 63;
				//carsx[i] = 0;
				otherCarModel[i] = glm::translate(model2, glm::vec3(carsx[i] * 6, 0, carsz[i]));
				otherCarSpeed[i] = rand() % 1200 + 300;
			}
		}
		for (int i = 0; i < carcount1; i++)
		{
			float carSpeed = (otherCarSpeed[i] + globalSpeed) * deltaTime;
			otherCarModel[i] = glm::translate(otherCarModel[i], glm::vec3(0, 0, carSpeed));
			carsz[i] += carSpeed;
			//Collision detection
			if (carsz[i] >= -210 && carsz[i] <= 210) {
				double x=0;
				if(carx>=carsx[i])
				x=carx-carsx[i]+2.5;
				else
				x=carsx[i]-carx-6.5;
				if (x <=15) {
					cout<<carx<<endl;
					cout << "Collision detection" << endl;
					gameOver = true;
					continue;
					carsz[i] = 1000;
					carpass--;
				}
			}
			glUniformMatrix4fv(glGetUniformLocation(carShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(otherCarModel[i]));
			otherCar.Draw(carShader);
		}

		// Draw the road
		roadShader.Use();
		myTranslate = glm::translate(glm::mat4(), glm::vec3(0, 0, roadSpeed));
		// Move the road
		for (int i = 0; i < 10; i++)
		{
			roadModel[i] = myTranslate * roadModel[i];
		}
		// reinit the road
		if (roadCount++ == START_COUNT)
		{
			roadCount = 0;

			// ﹞??車
			myScalingMatrix = glm::scale(glm::mat4(), glm::vec3(5, 5, 15));
			// ??℅?
			myScalingMatrix = glm::rotate(myScalingMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

			int road_pos = 5;
			for (int i = 0; i < 10; i++)
			{
				roadModel[i] = glm::mat4();

				myTranslate = glm::translate(glm::mat4(), glm::vec3(0, -0.8, road_pos));
				roadModel[i] = myTranslate * myScalingMatrix * roadModel[i];

				// ?﹞???∟??﹞??車15㊣????????﹞??????????2*15????﹞??辰??????????
				road_pos -= 30;
			}
		}
		glUniformMatrix4fv(glGetUniformLocation(roadShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(roadShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		// road 
		glBindVertexArray(roadVAO);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(roadShader.Program, "texture1"), 0);
		glBindTexture(GL_TEXTURE_2D, roadTexture);
		for (int i = 0; i < 10; i++)
		{
			glUniformMatrix4fv(glGetUniformLocation(roadShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(roadModel[i]));
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		glBindVertexArray(0);

		// Draw skybox as last
		glDepthFunc(GL_LEQUAL);  // Change depth function so depth test passes when values are equal to depth buffer's content
		skyboxShader.Use();
		// change the model
		view = glm::mat4(glm::mat3(camera.GetViewMatrix()));	// Remove any translation component of the view matrix
		glUniformMatrix4fv(glGetUniformLocation(skyboxShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(skyboxShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		// skybox cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(shader.Program, "skybox"), 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // Set depth function back to default


		// render the text
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
							  
		textShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(textShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(text_projection));
		char disp_score[15];
		RenderText(textShader, "stupid car", 540.0f, 570.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f));
		if (gameOver)
		{
			RenderText(textShader, "GAME OVER", 200.0f, 200.0f, 1.5f, glm::vec3(1.0f, 0.0f, 0.0f));
			RenderText(textShader, "SCORE: ", 200.0f, 350.0f, 1.0f, glm::vec3(1.0, 0.0f, 0.0f));
			RenderText(textShader, disp_score, 500.0f, 350.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
		}
		else
		{
			_itoa(carpass, disp_score, 10);
			RenderText(textShader, disp_score, 25.0f, 25.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
		}

		//ljt
		glBindTexture(GL_TEXTURE_2D, partilceTexture);
		//glm::mat4 modelParticle = glm::scale(model, glm::vec3(50, 50, 50));
		psMainParticleSystem.SetMatrices(&projection, camera.Position, camera.Front, camera.Up);
		psMainParticleSystem.matModel = modelParticle;
		UpdateTimer();
		psMainParticleSystem.UpdateParticles(sof(1.0f));
		psMainParticleSystem.RenderParticles();
			
		//glDepthFunc(GL_LESS); // Set depth function back to default

		// Swap the buffers
		glfwSwapBuffers(window);
	}
	
	// 释放
	FSOUND_Sample_Free(bgm_handle);
	FSOUND_Close();

	glfwTerminate();

	return 0;
}

void move_project(glm::mat4 &model, glm::mat4 &modelPar)
{
	//glm::mat4 myMatrix;
	if (keys[GLFW_KEY_J] && carx >= -63) {
		carx -= 1;
		model = glm::translate(model, glm::vec3(6, 0, 0));
		modelPar = glm::translate(modelPar, glm::vec3(6, 0, 0));
		//myMatrix = glm::translate(glm::mat4(), glm::vec3(-0.03, 0, 0));
	}
	else if (keys[GLFW_KEY_K] && carx <= 63) {
		carx += 1;
		model = glm::translate(model, glm::vec3(-6, 0, 0));
		modelPar = glm::translate(modelPar, glm::vec3(-6, 0, 0));
		//myMatrix = glm::translate(glm::mat4(), glm::vec3(0.03, 0, 0));
	}
	//model = myMatrix * model;
}

// Loads a cubemap texture from 6 individual texture faces
// Order should be:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
GLuint loadCubemap(vector<const GLchar*> faces)
{
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width, height;
	unsigned char* image;

	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	for (GLuint i = 0; i < faces.size(); i++)
	{
		image = SOIL_load_image(faces[i], &width, &height, 0, SOIL_LOAD_RGB);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		SOIL_free_image_data(image);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return textureID;
}


// This function loads a texture from file. Note: texture loading functions like these are usually 
// managed by a 'Resource Manager' that manages all resources (like textures, models, audio). 
// For learning purposes we'll just define it as a utility function.
GLuint loadTexture(GLchar* path)
{
	//Generate texture ID and load texture data 
	GLuint textureID;
	glGenTextures(1, &textureID);
	int width, height;
	unsigned char* image = SOIL_load_image(path, &width, &height, 0, SOIL_LOAD_RGB);
	// Assign texture to ID
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	SOIL_free_image_data(image);
	return textureID;
}

#pragma region "User input"

// Moves/alters the camera positions based on user input
void Do_Movement()
{
	// Camera controls
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);

	if (keys[GLFW_KEY_LEFT_SHIFT])
		globalSpeed = 45.0f;
	else
		globalSpeed = 30.0f;

	if (keys[GLFW_KEY_B])
		startGame = true;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (action == GLFW_PRESS)
		keys[key] = true;
	else if (action == GLFW_RELEASE)
		keys[key] = false;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

void RenderText(Shader &shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
	// Activate corresponding render state	
	shader.Use();
	glUniform3f(glGetUniformLocation(shader.Program, "textColor"), color.x, color.y, color.z);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	// Iterate through all characters
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		Character ch = characters[*c];

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;
		// Update VBO for each character
		GLfloat vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
		};
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void setupLight(Shader &shader)
{
	// == ==========================
	// Here we set all the uniforms for the 5/6 types of lights we have. We have to set them manually and index 
	// the proper PointLight struct in the array to set each uniform variable. This can be done more code-friendly
	// by defining light types as classes and set their values in there, or by using a more efficient uniform approach
	// by using 'Uniform buffer objects', but that is something we discuss in the 'Advanced GLSL' tutorial.
	// == ==========================
	// Directional light
	glUniform3f(glGetUniformLocation(shader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
	glUniform3f(glGetUniformLocation(shader.Program, "dirLight.ambient"), 0.05f, 0.05f, 0.05f);
	glUniform3f(glGetUniformLocation(shader.Program, "dirLight.diffuse"), 0.4f, 0.4f, 0.4f);
	glUniform3f(glGetUniformLocation(shader.Program, "dirLight.specular"), 0.5f, 0.5f, 0.5f);
	// Point light 1
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[0].position"), pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[0].ambient"), 0.05f, 0.05f, 0.05f);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[0].diffuse"), 0.8f, 0.8f, 0.8f);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[0].specular"), 1.0f, 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[0].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[0].linear"), 0.09);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[0].quadratic"), 0.032);
	// Point light 2
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[1].position"), pointLightPositions[1].x, pointLightPositions[1].y, pointLightPositions[1].z);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[1].ambient"), 0.05f, 0.05f, 0.05f);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[1].diffuse"), 0.8f, 0.8f, 0.8f);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[1].specular"), 1.0f, 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[1].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[1].linear"), 0.09);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[1].quadratic"), 0.032);
	// Point light 3
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[2].position"), pointLightPositions[2].x, pointLightPositions[2].y, pointLightPositions[2].z);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[2].ambient"), 0.05f, 0.05f, 0.05f);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[2].diffuse"), 0.8f, 0.8f, 0.8f);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[2].specular"), 1.0f, 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[2].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[2].linear"), 0.09);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[2].quadratic"), 0.032);
	// Point light 4
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[3].position"), pointLightPositions[3].x, pointLightPositions[3].y, pointLightPositions[3].z);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[3].ambient"), 0.05f, 0.05f, 0.05f);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[3].diffuse"), 0.8f, 0.8f, 0.8f);
	glUniform3f(glGetUniformLocation(shader.Program, "pointLights[3].specular"), 1.0f, 1.0f, 1.0f);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[3].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[3].linear"), 0.09);
	glUniform1f(glGetUniformLocation(shader.Program, "pointLights[3].quadratic"), 0.032);
	// SpotLight
	//glUniform3f(glGetUniformLocation(shader.Program, "spotLight.position"), camera.Position.x, camera.Position.y, camera.Position.z);
	//glUniform3f(glGetUniformLocation(shader.Program, "spotLight.direction"), camera.Front.x, camera.Front.y, camera.Front.z);
	//glUniform3f(glGetUniformLocation(shader.Program, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
	//glUniform3f(glGetUniformLocation(shader.Program, "spotLight.diffuse"), 1.0f, 1.0f, 1.0f);
	//glUniform3f(glGetUniformLocation(shader.Program, "spotLight.specular"), 1.0f, 1.0f, 1.0f);
	//glUniform1f(glGetUniformLocation(shader.Program, "spotLight.constant"), 1.0f);
	//glUniform1f(glGetUniformLocation(shader.Program, "spotLight.linear"), 0.09);
	//glUniform1f(glGetUniformLocation(shader.Program, "spotLight.quadratic"), 0.032);
	//glUniform1f(glGetUniformLocation(shader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
	//glUniform1f(glGetUniformLocation(shader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(15.0f)));
}

#pragma endregion
