#include "Angel.h"
#include "TriMesh.h"
#include "Camera.h"
#include <vector>
#include <string>
#include <algorithm>

int WIDTH = 600;
int HEIGHT = 600;

int mainWindow;
bool is_click = false;

struct openGLObject
{
	// 顶点数组对象
	GLuint vao;
	// 顶点缓存对象
	GLuint vbo;

	// 着色器程序
	GLuint program;
	// 着色器文件
	std::string vshader;
	std::string fshader;
	// 着色器变量
	GLuint pLocation;
	GLuint cLocation;
	GLuint nLocation;

	// 投影变换变量
	GLuint modelLocation;
	GLuint viewLocation;
	GLuint projectionLocation;

	// 阴影变量
	GLuint shadowLocation;
};

openGLObject mesh_object;  // 用于球体/皮卡丘等主物体
openGLObject plane_object; // 用于地面

Light *light = new Light();

TriMesh *mesh = new TriMesh();	// 主物体
TriMesh *plane = new TriMesh(); // 地面

Camera *camera = new Camera();

// 绑定数据通用的函数
void bindObjectAndData(TriMesh *mesh, openGLObject &object, const std::string &vshader, const std::string &fshader)
{
	// 创建顶点数组对象
	glGenVertexArrays(1, &object.vao);
	glBindVertexArray(object.vao);

	// 创建并初始化顶点缓存对象
	glGenBuffers(1, &object.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, object.vbo);

	// 计算总大小：点 + 颜色 + 法向量
	int dataSize = (mesh->getPoints().size() + mesh->getColors().size() + mesh->getNormals().size()) * sizeof(glm::vec3);
	glBufferData(GL_ARRAY_BUFFER, dataSize, NULL, GL_STATIC_DRAW);

	// 分别上传数据
	glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->getPoints().size() * sizeof(glm::vec3), &mesh->getPoints()[0]);
	glBufferSubData(GL_ARRAY_BUFFER, mesh->getPoints().size() * sizeof(glm::vec3), mesh->getColors().size() * sizeof(glm::vec3), &mesh->getColors()[0]);
	// Task 1: 必须取消注释
	glBufferSubData(GL_ARRAY_BUFFER, (mesh->getPoints().size() + mesh->getColors().size()) * sizeof(glm::vec3), mesh->getNormals().size() * sizeof(glm::vec3), &mesh->getNormals()[0]);

	object.vshader = vshader;
	object.fshader = fshader;
	object.program = InitShader(object.vshader.c_str(), object.fshader.c_str());

	// 从顶点着色器中初始化顶点的坐标
	object.pLocation = glGetAttribLocation(object.program, "vPosition");
	glEnableVertexAttribArray(object.pLocation);
	glVertexAttribPointer(object.pLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	// 从顶点着色器中初始化顶点的颜色
	object.cLocation = glGetAttribLocation(object.program, "vColor");
	glEnableVertexAttribArray(object.cLocation);
	glVertexAttribPointer(object.cLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(mesh->getPoints().size() * sizeof(glm::vec3)));

	// Task 1: 从顶点着色器中初始化顶点的法向量
	object.nLocation = glGetAttribLocation(object.program, "vNormal");
	glEnableVertexAttribArray(object.nLocation);
	glVertexAttribPointer(object.nLocation, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET((mesh->getPoints().size() + mesh->getColors().size()) * sizeof(glm::vec3)));

	// 获得矩阵位置
	object.modelLocation = glGetUniformLocation(object.program, "model");
	object.viewLocation = glGetUniformLocation(object.program, "view");
	object.projectionLocation = glGetUniformLocation(object.program, "projection");

	object.shadowLocation = glGetUniformLocation(object.program, "isShadow");
}

void bindLightAndMaterial(TriMesh *mesh, openGLObject &object, Light *light, Camera *camera)
{
	// 传递相机的位置
	glUniform3fv(glGetUniformLocation(object.program, "eye_position"), 1, &camera->eye[0]);

	// 传递物体的材质
	glm::vec4 meshAmbient = mesh->getAmbient();
	glm::vec4 meshDiffuse = mesh->getDiffuse();
	glm::vec4 meshSpecular = mesh->getSpecular();
	float meshShininess = mesh->getShininess();

	glUniform4fv(glGetUniformLocation(object.program, "material.ambient"), 1, &meshAmbient[0]);
	glUniform4fv(glGetUniformLocation(object.program, "material.diffuse"), 1, &meshDiffuse[0]);
	glUniform4fv(glGetUniformLocation(object.program, "material.specular"), 1, &meshSpecular[0]);
	glUniform1f(glGetUniformLocation(object.program, "material.shininess"), meshShininess);

	// 传递光源信息
	glm::vec4 lightAmbient = light->getAmbient();
	glm::vec4 lightDiffuse = light->getDiffuse();
	glm::vec4 lightSpecular = light->getSpecular();
	glm::vec3 lightPosition = light->getTranslation();
	glUniform4fv(glGetUniformLocation(object.program, "light.ambient"), 1, &lightAmbient[0]);
	glUniform4fv(glGetUniformLocation(object.program, "light.diffuse"), 1, &lightDiffuse[0]);
	glUniform4fv(glGetUniformLocation(object.program, "light.specular"), 1, &lightSpecular[0]);
	glUniform3fv(glGetUniformLocation(object.program, "light.position"), 1, &lightPosition[0]);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void init()
{
	std::string vshader = "shaders/vshader.glsl";
	std::string fshader = "shaders/fshader.glsl";

	// 1. 设置光源 (Task 3)
	light->setTranslation(glm::vec3(1.0, 2.0, 1.0)); // 调高一点，方便产生阴影
	light->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0));
	light->setDiffuse(glm::vec4(1.0, 1.0, 1.0, 1.0));
	light->setSpecular(glm::vec4(1.0, 1.0, 1.0, 1.0));

	// 2. 设置主物体 (mesh)
	mesh->setTranslation(glm::vec3(0.0, 0.5, 0.0)); // 放在地面上方
	mesh->setRotation(glm::vec3(0, 0.0, 0.0));
	mesh->setScale(glm::vec3(0.8, 0.8, 0.8));

	// 设置主物体默认材质 (红色高光球)
	mesh->setAmbient(glm::vec4(0.2, 0.0, 0.0, 1.0));
	mesh->setDiffuse(glm::vec4(0.8, 0.1, 0.1, 1.0));
	mesh->setSpecular(glm::vec4(1.0, 1.0, 1.0, 1.0));
	mesh->setShininess(50.0);

	bindObjectAndData(mesh, mesh_object, vshader, fshader);

	// 3. 准备地面 (正方形)
	plane->generateSquare(glm::vec3(0.6, 0.6, 0.6)); // 灰色地面
	// 【关键修改】旋转 -90 度，使法向量朝上
	plane->setRotation(glm::vec3(-90, 0, 0));
	// 【关键修改】下移一点点，防止与阴影 Z-Fighting
	plane->setTranslation(glm::vec3(0, -0.01, 0));
	plane->setScale(glm::vec3(4.0, 4.0, 4.0)); // 地面放大
	// 设置地面材质 (哑光)
	plane->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0));
	plane->setDiffuse(glm::vec4(0.6, 0.6, 0.6, 1.0));
	plane->setSpecular(glm::vec4(0.0, 0.0, 0.0, 1.0));
	plane->setShininess(1.0);

	bindObjectAndData(plane, plane_object, vshader, fshader);

	// 背景色设置为灰色
	glClearColor(0.3, 0.3, 0.3, 1.0);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 相机矩阵计算
	camera->updateCamera();
	camera->viewMatrix = camera->getViewMatrix();
	camera->projMatrix = camera->getProjectionMatrix(false);

	// ==========================================
	// 1. 绘制主物体 (Phong 光照)
	// ==========================================
	glBindVertexArray(mesh_object.vao);
	glUseProgram(mesh_object.program);

	glm::mat4 modelMatrix = mesh->getModelMatrix();

	glUniformMatrix4fv(mesh_object.modelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	glUniformMatrix4fv(mesh_object.viewLocation, 1, GL_FALSE, &camera->viewMatrix[0][0]);
	glUniformMatrix4fv(mesh_object.projectionLocation, 1, GL_FALSE, &camera->projMatrix[0][0]);

	glUniform1i(mesh_object.shadowLocation, 0); // 正常绘制
	bindLightAndMaterial(mesh, mesh_object, light, camera);
	glDrawArrays(GL_TRIANGLES, 0, mesh->getPoints().size());

	// ==========================================
	// 2. 绘制地面 (Phong 光照)
	// ==========================================
	glBindVertexArray(plane_object.vao);
	glUseProgram(plane_object.program);

	glm::mat4 planeModelMatrix = plane->getModelMatrix();
	glUniformMatrix4fv(plane_object.modelLocation, 1, GL_FALSE, &planeModelMatrix[0][0]);
	glUniformMatrix4fv(plane_object.viewLocation, 1, GL_FALSE, &camera->viewMatrix[0][0]);
	glUniformMatrix4fv(plane_object.projectionLocation, 1, GL_FALSE, &camera->projMatrix[0][0]);

	glUniform1i(plane_object.shadowLocation, 0); // 正常绘制
	bindLightAndMaterial(plane, plane_object, light, camera);
	glDrawArrays(GL_TRIANGLES, 0, plane->getPoints().size());

	// ==========================================
	// 3. 绘制阴影 (硬阴影)
	// ==========================================
	// 切换回主物体的 VAO
	glBindVertexArray(mesh_object.vao);
	glUseProgram(mesh_object.program);

	// 计算阴影投影矩阵 (复用 3.2 的逻辑)
	float lx = light->getTranslation().x;
	float ly = light->getTranslation().y;
	float lz = light->getTranslation().z;

	glm::mat4 shadowProjMatrix = glm::mat4(0.0f);
	shadowProjMatrix[0][0] = ly;
	shadowProjMatrix[1][0] = -lx;
	shadowProjMatrix[1][2] = -lz;
	shadowProjMatrix[1][3] = -1.0f;
	shadowProjMatrix[2][2] = ly;
	shadowProjMatrix[3][3] = ly;

	// 阴影变换 = 阴影投影 * 物体变换
	glm::mat4 shadowModelMatrix = shadowProjMatrix * modelMatrix;

	glUniformMatrix4fv(mesh_object.modelLocation, 1, GL_FALSE, &shadowModelMatrix[0][0]);
	// 视图和投影保持不变
	glUniformMatrix4fv(mesh_object.viewLocation, 1, GL_FALSE, &camera->viewMatrix[0][0]);
	glUniformMatrix4fv(mesh_object.projectionLocation, 1, GL_FALSE, &camera->projMatrix[0][0]);

	// 关键：设置为阴影模式 (绘制纯黑)
	glUniform1i(mesh_object.shadowLocation, 1);

	glDrawArrays(GL_TRIANGLES, 0, mesh->getPoints().size());
}

void printHelp()
{
	std::cout << "================================================" << std::endl;
	std::cout << "Use mouse to controll the light position (drag)." << std::endl;
	std::cout << "================================================" << std::endl
			  << std::endl;

	std::cout << "Keyboard Usage" << std::endl;
	std::cout << "[Window]" << std::endl
			  << "ESC:      Exit" << std::endl
			  << "h:        Print help message" << std::endl
			  << std::endl
			  << "[Model]" << std::endl
			  << "-:        Reset material parameters" << std::endl
			  << "(shift) + 1/2/3:  Change ambient parameters" << std::endl
			  << "(shift) + 4/5/6:  Change diffuse parameters" << std::endl
			  << "(shift) + 7/8/9:  Change specular parameters" << std::endl
			  << "(shift) + 0:      Change shininess parameters" << std::endl
			  << std::endl
			  << "q:        Load sphere model" << std::endl
			  << "a:        Load Pikachu model" << std::endl
			  << "w:        Load Squirtle model" << std::endl
			  << "s:        Load sphere_coarse model" << std::endl
			  << std::endl
			  << "[Camera]" << std::endl
			  << "SPACE:        Reset camera parameters" << std::endl
			  << "u/(shift+u):      Increase/Decrease the rotate angle" << std::endl
			  << "i/(shift+i):      Increase/Decrease the up angle" << std::endl
			  << "o/(shift+o):      Increase/Decrease the camera radius" << std::endl
			  << std::endl;
}

void mainWindow_key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	float tmp;
	glm::vec4 ambient;

	// --- 窗口控制 ---
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (key == GLFW_KEY_H && action == GLFW_PRESS)
	{
		printHelp();
	}

	// --- 模型加载 (每次重新加载都会自动重新绑定VAO/VBO) ---
	else if (key == GLFW_KEY_Q && action == GLFW_PRESS)
	{
		std::cout << "read sphere.off" << std::endl;
		mesh->readOff("./assets/sphere.off");
		// 重新初始化会处理数据绑定
		init();
	}
	else if (key == GLFW_KEY_A && action == GLFW_PRESS)
	{
		std::cout << "read Pikachu.off" << std::endl;
		mesh->readOff("./assets/Pikachu.off");
		init();
	}
	else if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		std::cout << "read Squirtle.off" << std::endl;
		mesh->readOff("./assets/Squirtle.off");
		init();
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		std::cout << "read sphere_coarse.off" << std::endl;
		mesh->readOff("./assets/sphere_coarse.off");
		init();
	}

	// --- 材质控制 (Task 4) ---
	else if (key == GLFW_KEY_1 && action == GLFW_PRESS && mode == 0x0000)
	{
		ambient = mesh->getAmbient();
		ambient.x = std::min(ambient.x + 0.1f, 1.0f);
		mesh->setAmbient(ambient);
	}
	else if (key == GLFW_KEY_1 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		ambient = mesh->getAmbient();
		ambient.x = std::max(ambient.x - 0.1f, 0.0f);
		mesh->setAmbient(ambient);
	}
	else if (key == GLFW_KEY_2 && action == GLFW_PRESS && mode == 0x0000)
	{
		ambient = mesh->getAmbient();
		ambient.y = std::min(ambient.y + 0.1f, 1.0f);
		mesh->setAmbient(ambient);
	}
	else if (key == GLFW_KEY_2 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		ambient = mesh->getAmbient();
		ambient.y = std::max(ambient.y - 0.1f, 0.0f);
		mesh->setAmbient(ambient);
	}
	else if (key == GLFW_KEY_3 && action == GLFW_PRESS && mode == 0x0000)
	{
		ambient = mesh->getAmbient();
		ambient.z = std::min(ambient.z + 0.1f, 1.0f);
		mesh->setAmbient(ambient);
	}
	else if (key == GLFW_KEY_3 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		ambient = mesh->getAmbient();
		ambient.z = std::max(ambient.z - 0.1f, 0.0f);
		mesh->setAmbient(ambient);
	}

	// 4/5/6: 漫反射
	else if (key == GLFW_KEY_4 && action == GLFW_PRESS && mode == 0x0000)
	{
		glm::vec4 diff = mesh->getDiffuse();
		diff.x = std::min(diff.x + 0.1f, 1.0f);
		mesh->setDiffuse(diff);
	}
	else if (key == GLFW_KEY_4 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		glm::vec4 diff = mesh->getDiffuse();
		diff.x = std::max(diff.x - 0.1f, 0.0f);
		mesh->setDiffuse(diff);
	}
	else if (key == GLFW_KEY_5 && action == GLFW_PRESS && mode == 0x0000)
	{
		glm::vec4 diff = mesh->getDiffuse();
		diff.y = std::min(diff.y + 0.1f, 1.0f);
		mesh->setDiffuse(diff);
	}
	else if (key == GLFW_KEY_5 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		glm::vec4 diff = mesh->getDiffuse();
		diff.y = std::max(diff.y - 0.1f, 0.0f);
		mesh->setDiffuse(diff);
	}
	else if (key == GLFW_KEY_6 && action == GLFW_PRESS && mode == 0x0000)
	{
		glm::vec4 diff = mesh->getDiffuse();
		diff.z = std::min(diff.z + 0.1f, 1.0f);
		mesh->setDiffuse(diff);
	}
	else if (key == GLFW_KEY_6 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		glm::vec4 diff = mesh->getDiffuse();
		diff.z = std::max(diff.z - 0.1f, 0.0f);
		mesh->setDiffuse(diff);
	}

	// 7/8/9: 镜面反射
	else if (key == GLFW_KEY_7 && action == GLFW_PRESS && mode == 0x0000)
	{
		glm::vec4 spec = mesh->getSpecular();
		spec.x = std::min(spec.x + 0.1f, 1.0f);
		mesh->setSpecular(spec);
	}
	else if (key == GLFW_KEY_7 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		glm::vec4 spec = mesh->getSpecular();
		spec.x = std::max(spec.x - 0.1f, 0.0f);
		mesh->setSpecular(spec);
	}
	else if (key == GLFW_KEY_8 && action == GLFW_PRESS && mode == 0x0000)
	{
		glm::vec4 spec = mesh->getSpecular();
		spec.y = std::min(spec.y + 0.1f, 1.0f);
		mesh->setSpecular(spec);
	}
	else if (key == GLFW_KEY_8 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		glm::vec4 spec = mesh->getSpecular();
		spec.y = std::max(spec.y - 0.1f, 0.0f);
		mesh->setSpecular(spec);
	}
	else if (key == GLFW_KEY_9 && action == GLFW_PRESS && mode == 0x0000)
	{
		glm::vec4 spec = mesh->getSpecular();
		spec.z = std::min(spec.z + 0.1f, 1.0f);
		mesh->setSpecular(spec);
	}
	else if (key == GLFW_KEY_9 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		glm::vec4 spec = mesh->getSpecular();
		spec.z = std::max(spec.z - 0.1f, 0.0f);
		mesh->setSpecular(spec);
	}

	// 0: 高光系数
	else if (key == GLFW_KEY_0 && action == GLFW_PRESS && mode == 0x0000)
	{
		float shininess = mesh->getShininess();
		mesh->setShininess(shininess * 1.5f);
	}
	else if (key == GLFW_KEY_0 && action == GLFW_PRESS && mode == GLFW_MOD_SHIFT)
	{
		float shininess = mesh->getShininess();
		mesh->setShininess(std::max(shininess / 1.5f, 1.0f));
	}

	// 重置材质
	else if (key == GLFW_KEY_MINUS && action == GLFW_PRESS && mode == 0x0000)
	{
		mesh->setAmbient(glm::vec4(0.2, 0.2, 0.2, 1.0));
		mesh->setDiffuse(glm::vec4(0.7, 0.7, 0.7, 1.0));
		mesh->setSpecular(glm::vec4(0.2, 0.2, 0.2, 1.0));
		mesh->setShininess(1.0);
	}

	// --- 相机控制 ---
	else
	{
		camera->keyboard(key, action, mode);
	}
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double x, y;
		glfwGetCursorPos(window, &x, &y);

		float half_winx = WIDTH / 2.0;
		float half_winy = HEIGHT / 2.0;
		// 扩大控制范围，让阴影移动更明显
		float lx = float(x - half_winx) / half_winx * 4.0;
		float ly = float(HEIGHT - y - half_winy) / half_winy * 4.0;

		glm::vec3 pos = light->getTranslation();
		pos.x = lx;
		pos.z = -ly; // 屏幕 Y 对应 3D 场景 Z
		light->setTranslation(pos);
	}
}

void cleanData()
{
	mesh->cleanData();
	delete camera;
	camera = NULL;
	delete mesh;
	mesh = NULL;

	glDeleteVertexArrays(1, &mesh_object.vao);
	glDeleteBuffers(1, &mesh_object.vbo);
	glDeleteProgram(mesh_object.program);

	glDeleteVertexArrays(1, &plane_object.vao);
	glDeleteBuffers(1, &plane_object.vbo);
	glDeleteProgram(plane_object.program);
}

int main(int argc, char **argv)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow *mainwindow = glfwCreateWindow(600, 600, "2023270173_Elaine_lab3", NULL, NULL);
	if (mainwindow == NULL)
	{
		std::cout << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(mainwindow);
	glfwSetFramebufferSizeCallback(mainwindow, framebuffer_size_callback);
	glfwSetKeyCallback(mainwindow, mainWindow_key_callback);
	glfwSetMouseButtonCallback(mainwindow, mouse_button_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glEnable(GL_DEPTH_TEST);

	// 默认加载球体
	mesh->readOff("./assets/sphere.off");
	mesh->generateCube(); // 初始化可能需要的 Cube 数据（如果有用到）

	init();

	int width, height;
	glfwGetFramebufferSize(mainwindow, &width, &height);
	glViewport(0, 0, width, height);

	printHelp();

	while (!glfwWindowShouldClose(mainwindow))
	{
		display();
		glfwSwapBuffers(mainwindow);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}