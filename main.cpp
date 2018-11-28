#include "ShaderProgram.h"
#include "ObjMesh.h"
#include "UVCylinder.h"

#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <map>
#include <GL/glew.h>
#include <soil/src/SOIL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "apis/stb_image.h"

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#define SCALE_FACTOR 2.0f

int width = 1, height = 1;

float lightRotation = 0.0f;

GLuint programId;
glm::vec4 lightPosDir;
GLuint skyboxTexture = GL_NONE;

struct MeshBuffers
{
	GLuint positions;
	GLuint textureCoords;
	GLuint normals;
	GLuint colours;

	GLuint index;
};

float lerp(float startValue, float endValue, float t) {
	return startValue * (1.0f - t) + endValue * t;
}

glm::vec3 lerp(glm::vec3 start, glm::vec3 end, float t) {
	return glm::vec3(lerp(start.x, end.x, t), lerp(start.y, end.y, t), lerp(start.z, end.z, t));
}

struct Frame {
	glm::vec3 position;
	unsigned int duration;

	Frame(glm::vec3 p, unsigned int d) {
		position = p;
		duration = d;
	}
};

class Animation {
	std::vector<Frame> frames;
	unsigned int lastFrame;
	unsigned int currentFrame;
	unsigned int frameSwitchTime;
	bool animating;

public:
	Animation(std::vector<Frame> fv) {
		frames = fv;
		lastFrame = 0;
		currentFrame = 1;
		frameSwitchTime = 0.0f;
		animating = true;
	}

	void AddFrame(Frame frame) {
		frames.push_back(frame);
	}

	bool Update(unsigned int time, glm::mat4 &outTransform) {
		if (!animating) return false;
		if (frames.size() < 2) return false; // frame[0] == initial position, frame[1..n] == transitions

		// Get frames
		Frame last = frames[lastFrame];
		Frame current = frames[currentFrame];

		unsigned int delta = time - frameSwitchTime;
		if (delta > current.duration) {
			// Get next frame
			if (currentFrame + 1 > frames.size() - 1) {
				animating = false;
				return false;
			}

			// Set new frames
			last = frames[++lastFrame];
			current = frames[++currentFrame];

			// Update time since frame was last changed
			frameSwitchTime = time;
		}

		float p = glm::min((float)delta / (float)current.duration, 1.0f);

		// Apply transformation
		outTransform = glm::translate(outTransform, lerp(last.position, current.position, p));

		return true;
	}

	bool IsAnimating() {
		return animating;
	}

	void Reset(int time) {
		animating = true;
		lastFrame = 0;
		currentFrame = 1;
		frameSwitchTime = time;
	}
};

struct Mesh {
	MeshBuffers *buffers;
	unsigned int numVertices;

	std::string name;
	glm::vec3 color;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	glm::mat4 transform;
	GLuint texture = GL_NONE;

	Mesh(std::string n, MeshBuffers *b, unsigned int v) {
		buffers = b;
		numVertices = v;

		name = n;
		color = glm::vec3(0.0f, 0.0f, 0.0f);
		position = glm::vec3(0.0f);
		rotation = glm::vec3(0.0f);
		scale = glm::vec3(0.0f);
		transform = glm::mat4(0.0f);
	}
};

static GLuint createTexture(std::string filename) {
	int imageWidth, imageHeight;
	int numComponents;

	unsigned char* bitmap = stbi_load(filename.c_str(),
		&imageWidth,
		&imageHeight,
		&numComponents, 4);

	GLuint textureId;
	glGenTextures(1, &textureId);

	glBindTexture(GL_TEXTURE_2D, textureId);

	// resizing settings
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	// provide the image data to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
		imageWidth, imageHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);

	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);

	// free up the bitmap
	stbi_image_free(bitmap);

	return textureId;
}

// Time
float previousTime = 0.0f;

// Torus
MeshBuffers torusBuffers;
unsigned int torusNumVertices;

// Cube
MeshBuffers cubeBuffers;
unsigned int cubeNumVertices;

// Cylinder
MeshBuffers cylinderBuffers;
unsigned int cylinderNumVertices;

// Skybox
MeshBuffers skyboxBuffers;
unsigned int skyboxNumVertices;

bool scaling = false;
bool rotating = false;

float xAngle = 0.0f;
float yAngle = 0.0f;
float zAngle = 0.0f;
float lightOffsetY = 0.0f;

glm::vec3 eyePosition(20.0f, 0.0f, 0.0f);//glm::vec3 eyePosition(0, 30, 30);
glm::mat4 publicViewMatrix;
glm::mat4 publicProjectionMatrix;

// Colors
glm::vec3 colorYellow(0.9, 1.0, 0.1);
glm::vec3 colorPink(1.0, 0.0, 1.0);
glm::vec3 colorRed(0.8, 0.0, 0.0);
glm::vec3 colorGreen(0.0, 0.8, 0.0);
glm::vec3 colorBlue(0.0, 0.0, 0.8);

// Meshes
std::vector<Mesh *> meshes;
Mesh *skybox = new Mesh("Skybox", &skyboxBuffers, skyboxNumVertices);

// Animations
unsigned int currentAnimation = 0;
bool animating = true;
std::vector<std::pair<Mesh *, Animation *>> animations;

bool animateLight = true;

float scaleFactor = 1.0f;
float lastX = std::numeric_limits<float>::infinity();
float lastY = std::numeric_limits<float>::infinity();

// Forward declarations
void drawMesh(MeshBuffers &buffers, unsigned int numVertices, glm::mat4 &modelMatrix, glm::vec3 &colour, GLuint texture);

static void createGeometry(const char *fileName, MeshBuffers &buffers, unsigned int &numVertices) {
	// Load mesh
	ObjMesh mesh;
	mesh.load(fileName, true, true);

	numVertices = mesh.getNumIndexedVertices();
	Vector3* vertexPositions = mesh.getIndexedPositions();
	Vector2* vertexTextureCoords = mesh.getIndexedTextureCoords();
	Vector3* vertexNormals = mesh.getIndexedNormals();

	glGenBuffers(1, &buffers.positions);
	glBindBuffer(GL_ARRAY_BUFFER, buffers.positions);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), vertexPositions, GL_STATIC_DRAW);

	glGenBuffers(1, &buffers.textureCoords);
	glBindBuffer(GL_ARRAY_BUFFER, buffers.textureCoords);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector2), vertexTextureCoords, GL_STATIC_DRAW);

	glGenBuffers(1, &buffers.normals);
	glBindBuffer(GL_ARRAY_BUFFER, buffers.normals);
	glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vector3), vertexNormals, GL_STATIC_DRAW);

	unsigned int* indexData = mesh.getTriangleIndices();
	int numTriangles = mesh.getNumTriangles();

	glGenBuffers(1, &buffers.index);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * numTriangles * 3, indexData, GL_STATIC_DRAW);
}

static void initMeshes() {
	// Create geometry types
	createGeometry("meshes/torus.obj", torusBuffers, torusNumVertices);
	createGeometry("meshes/cube.obj", cubeBuffers, cubeNumVertices);

	// Cylinder generated Parametrically
	UVCylinder cylinder(1.0, 12);
	cylinder.save("meshes/my_cylinder.obj");
	createGeometry("meshes/my_cylinder.obj", cylinderBuffers, cylinderNumVertices);

	// Init meshes
	skybox = new Mesh("Skybox", &skyboxBuffers, skyboxNumVertices);
	Mesh *rectBase = new Mesh("Base", &cubeBuffers, cubeNumVertices);
	Mesh *poleOne = new Mesh("PoleOne", &cylinderBuffers, cylinderNumVertices);
	Mesh *poleTwo = new Mesh("PoleTwo", &cylinderBuffers, cylinderNumVertices);
	Mesh *poleThree = new Mesh("PoleThree", &cylinderBuffers, cylinderNumVertices);
	Mesh *diskOne = new Mesh("DiskOne", &torusBuffers, torusNumVertices);
	Mesh *diskTwo = new Mesh("DiskTwo", &torusBuffers, torusNumVertices);
	Mesh *diskThree = new Mesh("DiskThree", &torusBuffers, torusNumVertices);

	skybox->color = colorBlue;
	skybox->position = glm::vec3(0.0f, 0.0f, 0.0f);
	skybox->scale = glm::vec3(40.0f, 40.0f, 40.0f);
	skybox->rotation = glm::vec3(0.0f, 120.0f, 0.0f);
	skybox->texture = createTexture("textures/stars.jpeg");
	meshes.push_back(skybox);

	// Base
	rectBase->color = colorRed;
	rectBase->position = glm::vec3(0.0f, -2.5f, 0.0f);
	rectBase->scale = glm::vec3(5.0f, 0.5f, 15.0f);
	meshes.push_back(rectBase);

	// Pole one
	poleOne->color = colorBlue;
	poleOne->position = glm::vec3(0.0f, 0.0f, -5.0f);
	poleOne->rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	poleOne->scale = glm::vec3(1.0f, 1.0f, 5.0f);
	meshes.push_back(poleOne);

	// Pole two
	poleTwo->color = colorBlue;
	poleTwo->position = glm::vec3(0.0f, 0.0f, 0.0f);
	poleTwo->rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	poleTwo->scale = glm::vec3(1.0f, 1.0f, 5.0f);
	meshes.push_back(poleTwo);

	// Pole three
	poleThree->color = colorBlue;
	poleThree->position = glm::vec3(0.0f, 0.0f, 5.0f);
	poleThree->rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	poleThree->scale = glm::vec3(1.0f, 1.0f, 5.0f);
	meshes.push_back(poleThree);

	// Disk one
	diskOne->color = colorGreen;
	diskOne->scale = glm::vec3(4.0f);
	meshes.push_back(diskOne);

	// Disk two
	diskTwo->color = colorYellow;
	diskTwo->scale = glm::vec3(3.0f);
	meshes.push_back(diskTwo);

	// Disk three
	diskThree->color = colorPink;
	diskThree->scale = glm::vec3(2.2f);
	meshes.push_back(diskThree);

	// Init animations
	// Disk three: Part 1
	animations.push_back({
		diskThree,
		new Animation({
			Frame(glm::vec3(0.0f, -0.05f, 0.0f), 0), // Initial position
			Frame(glm::vec3(0.0f, 5.00f, 0.0f), 3000), // First position
			Frame(glm::vec3(0.0f, 5.00f, 5.00f), 1500), // Second position
			Frame(glm::vec3(0.0f, -1.80f, 5.00f), 3000), // Third position
			})
		});

	animations.push_back({
		diskTwo,
		new Animation({
			Frame(glm::vec3(0.0f, -0.8f, 0.0f), 0),
			Frame(glm::vec3(0.0f, 5.00f, 0.0f), 3000),
			Frame(glm::vec3(0.0f, 5.00f, -5.00f), 1500),
			Frame(glm::vec3(0.0f, -1.80f, -5.00f), 3000),
			})
		});

	animations.push_back({
		diskThree,
		new Animation({
			Frame(glm::vec3(0.0f, -1.80f, 5.0f), 0),
			Frame(glm::vec3(0.0f, 5.0f, 5.0f), 3000),
			Frame(glm::vec3(0.0f, 5.0f, -5.0f), 2000),
			Frame(glm::vec3(0.0f, -0.85f, -5.00f), 3000),
			})
		});

	animations.push_back({
		diskOne,
		new Animation({
			Frame(glm::vec3(0.0f, -1.8f, 0.0f), 0),
			Frame(glm::vec3(0.0f, 5.0f, 0.0f), 3000),
			Frame(glm::vec3(0.0f, 5.0f, 5.0f), 1500),
			Frame(glm::vec3(0.0f, -1.8f, 5.0f), 3000),
			})
		});

	animations.push_back({
		diskThree,
		new Animation({
			Frame(glm::vec3(0.0f, -0.85f, -5.00f), 0),
			Frame(glm::vec3(0.0f, 5.00f, -5.00f), 3000),
			Frame(glm::vec3(0.0f, 5.0f, 0.0f), 2000),
			Frame(glm::vec3(0.0f, -1.8f, 0.0f), 3000),
			})
		});

	animations.push_back({
		diskTwo,
		new Animation({
			Frame(glm::vec3(0.0f, -0.8f, -5.0f),0),
			Frame(glm::vec3(0.0f, 5.0f, -5.0f),2000),
			Frame(glm::vec3(0.0f, 5.0f, 5.00f), 3000),
			Frame(glm::vec3(0.0f, -0.8f, 5.00f), 3000)
			})
		});

	animations.push_back({
		diskThree,
		new Animation({
			Frame(glm::vec3(0.0f, -1.8f, 0.0f), 0),
			Frame(glm::vec3(0.0f, 5.0f, 0.0f), 3000),
			Frame(glm::vec3(0.0f, 5.0f, 5.0f), 2000),
			Frame(glm::vec3(0.0f, -0.05f, 5.0f), 3000),
			})
		});

	animations.push_back({
		diskThree,
		new Animation({
			Frame(glm::vec3(0.0f, -1.8f, 0.0f), 0),
			})
		});

	// Set initial transforms for animations
	std::map<Mesh *, bool> meshesUpdated;
	for (auto p : animations) {
		Mesh *m = p.first;

		// We only want to set the transform ONCE for each animated mesh, using the first animation
		if (meshesUpdated[m])
			continue;

		Animation *animation = p.second;

		// Reset transform
		m->transform = glm::mat4(1.0f);

		// Apply translations from first key frame
		animation->Update(0.0f, m->transform);

		// Apply scale
		m->transform = glm::scale(m->transform, m->scale);

		meshesUpdated[m] = true;
	}

	// Set static transforms for meshes that don't animate
	for (auto m : meshes) {
		if (meshesUpdated.find(m) == meshesUpdated.end()) {
			// Reset transform
			m->transform = glm::mat4(1.0f);

			// Apply translation
			m->transform = glm::translate(m->transform, m->position);

			// Apply rotation
			m->transform = glm::rotate(m->transform, glm::radians(m->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			m->transform = glm::rotate(m->transform, glm::radians(m->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			m->transform = glm::rotate(m->transform, glm::radians(m->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			// Apply scale
			m->transform = glm::scale(m->transform, m->scale);
		}
	}
}

void cleanupMeshes() {
	// Delete anims
	for (auto p : animations) {
		delete p.second;
	}

	animations.clear();

	// Delete meshes
	for (Mesh *m : meshes) {
		delete m;
	}

	meshes.clear();
}

static void update(void) {
	int timeMs = glutGet(GLUT_ELAPSED_TIME); // milliseconds
	int deltaTimeMs = timeMs - previousTime;

	lightRotation += 0.005f;

	// Move the light position over time along the x-axis, so we can see how it affects the shading
	if (animateLight) {
		lightPosDir = glm::vec4(5.0f, 10.0f, 3.0f, 0.0f);
	}
	else {
		lightPosDir = glm::vec4(cos(lightRotation), 1.0f, sin(lightRotation), 0.0f);
		lightPosDir = glm::normalize(lightPosDir);
	}

	float aspectRatio = (float)width / (float)height;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

	// view matrix - orient everything around our preferred view
	glm::mat4 view = glm::lookAt(
		eyePosition,
		glm::vec3(0, 0, 0), // target position
		glm::vec3(0, 1, 0)  // up
	);

	// Set view/proj matrices
	publicViewMatrix = view;
	publicProjectionMatrix = projection;

	// Check if we're animating
	if (animating && !animations.empty())
	{
		// Get current animation
		auto p = animations[currentAnimation];
		auto mesh = p.first;
		auto animation = p.second;

		// Create transform matrix and apply translations to it
		glm::mat4 transform(1.0f);
		if (animation->Update(timeMs, transform))
		{
			// Update current transform matrix with new one
			mesh->transform = transform;

			// Apply scale
			mesh->transform = glm::scale(mesh->transform, mesh->scale);
		}

		// Check if animation is done, then move onto next
		if (!animation->IsAnimating())
		{
			// If we've reached the end of animations stop animating
			if (++currentAnimation >= animations.size() - 1)
			{
				animating = false;
			}
			else
			{
				// Reset next animation with current time
				p = animations[currentAnimation];
				animation = p.second;
				animation->Reset(timeMs);
			}
		}
	}

	glutPostRedisplay();

	previousTime = timeMs;
}

static void render(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// activate our shader program
	glUseProgram(programId);

	// turn on depth buffering
	glEnable(GL_DEPTH_TEST);

	// Draw all meshes
	for (Mesh *m : meshes) {
		drawMesh(*m->buffers, m->numVertices, m->transform, m->color, m->texture);
	}

	// Swap front buffer with back buffer to display changes
	glutSwapBuffers();
}

void drawMesh(MeshBuffers &buffers, unsigned int numVertices, glm::mat4 &modelMatrix, glm::vec3 &colour, GLuint texture) {
	// model-view-projection matrix
	glm::mat4 mvp = publicProjectionMatrix * publicViewMatrix * modelMatrix;
	GLuint mvpMatrixId = glGetUniformLocation(programId, "u_MVP");
	glUniformMatrix4fv(mvpMatrixId, 1, GL_FALSE, &mvp[0][0]);

	GLuint lightPosId = glGetUniformLocation(programId, "u_lightPosDir");
	glUniform4fv(lightPosId, 1, &lightPosDir[0]);

	GLuint colourId = glGetUniformLocation(programId, "u_color");
	glUniform3fv(colourId, 1, (GLfloat*)&colour[0]);
	// TODO: ...

	if (texture == GL_NONE) {
		GLuint textured = glGetUniformLocation(programId, "u_textured");
		glUniform1i(textured, 0);
	}
	else {

		GLuint textured = glGetUniformLocation(programId, "u_textured");
		glUniform1i(textured, 1);

		GLuint textureId = glGetUniformLocation(programId, "u_texture");
		glUniform1i(textureId, 0); // Channel 0

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
	}
	// find the names (ids) of each vertex attribute
	GLint positionAttribId = glGetAttribLocation(programId, "position");
	GLint textureCoordsAttribId = glGetAttribLocation(programId, "textureCoords");
	GLint normalAttribId = glGetAttribLocation(programId, "normal");

	// provide the vertex positions to the shaders
	glBindBuffer(GL_ARRAY_BUFFER, buffers.positions);
	glEnableVertexAttribArray(positionAttribId);
	glVertexAttribPointer(positionAttribId, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// provide the vertex texture coordinates to the shaders
	if (textureCoordsAttribId > -1) {
		// NOTE: This is a ghetto fix until we figure out what's up with the shader, my money is on optimization
		glBindBuffer(GL_ARRAY_BUFFER, buffers.textureCoords);
		glEnableVertexAttribArray(textureCoordsAttribId);
		glVertexAttribPointer(textureCoordsAttribId, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	}

	// provide the vertex normals to the shaders
	if (normalAttribId > -1) {
		// NOTE: This is a ghetto fix until we figure out what's up with the shader, my money is on optimization
		glBindBuffer(GL_ARRAY_BUFFER, buffers.normals);
		glEnableVertexAttribArray(normalAttribId);
		glVertexAttribPointer(normalAttribId, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	}

	// draw the triangles
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.index);
	glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_INT, (void*)0);

	// disable the attribute arrays
	glDisableVertexAttribArray(positionAttribId);
	if (textureCoordsAttribId > -1) glDisableVertexAttribArray(textureCoordsAttribId);
	if (normalAttribId > -1) glDisableVertexAttribArray(normalAttribId);
}

static void reshape(int w, int h) {
	glViewport(0, 0, w, h);

	width = w;
	height = h;
}


static void keyboard(unsigned char key, int x, int y) {
	std::cout << "Key pressed: " << key << std::endl;
	if (key == 'l') {
		animateLight = !animateLight;
	}
}


static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	const char *sourceStr;
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		sourceStr = "API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		sourceStr = "Window";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		sourceStr = "Shader";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		sourceStr = "3rd Party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		sourceStr = "Application";
		break;
	default:
		sourceStr = "Other";
		break;
	}

	const char *typeStr;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		typeStr = "Error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		typeStr = "Deprecated Behaviour";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		typeStr = "Undefined Behaviour";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		typeStr = "Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		typeStr = "Performance";
		break;
	case GL_DEBUG_TYPE_MARKER:
		typeStr = "Marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		typeStr = "Push Group";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		typeStr = "Pop Group";
		break;
	default:
		typeStr = "Other";
		break;
	}

	const char *severityStr;
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		severityStr = "High";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		severityStr = "Medium";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		severityStr = "Low";
		break;
	default:
		severityStr = "Notification";
	}

	printf("[%s][%s][%s] %s\r\n", sourceStr, typeStr, severityStr, message);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutCreateWindow("Final Project");
	glutIdleFunc(&update);
	glutDisplayFunc(&render);
	glutReshapeFunc(&reshape);
	glutKeyboardFunc(&keyboard);

	glewInit();
	if (!GLEW_VERSION_2_0) {
		std::cerr << "OpenGL 2.0 not available" << std::endl;
		return 1;
	}
	std::cout << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
	std::cout << "Using OpenGL " << glGetString(GL_VERSION) << std::endl;

	// enable debugging
	glDebugMessageCallback(debugCallback, nullptr);
	glEnable(GL_DEBUG_OUTPUT);

	ShaderProgram program;
	/*program.loadShaders("shaders/vertex.glsl", "shaders/fragment.glsl");*/
	program.loadShaders("shaders/lambert_vertex.glsl", "shaders/lambert_fragment.glsl");

	programId = program.getProgramId();


	createGeometry("meshes/skybox.obj", skyboxBuffers, skyboxNumVertices);

	initMeshes();

	glutMainLoop();

	cleanupMeshes();

	return 0;
}
