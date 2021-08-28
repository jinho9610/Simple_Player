//#include "include/GL/glew.h"
//#include "include/GLFW/glfw3.h"
//#include <iostream>
//
//#pragma comment(lib, "OpenGL32.lib")
//#pragma comment(lib, "lib/glew32.lib")
//#pragma comment(lib, "lib/glfw3.lib")
//
//using namespace std;
//
//void window_resized(GLFWwindow* window, int width, int height);
//void key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods);
//void show_glfw_error(int error, const char* description);
//
//int framebufferWidth, framebufferHeight;
//GLuint triangleVertexArrayObject, triangleShaderPromgramID;
//GLuint triangleShaderProgramID, trianglePositionVertexBufferObjectID, triangleColorVertexBufferObjectID;
//
//bool initShaderProgram()
//{
//	const char* vertexShaderSource =
//		"#version 330 core\n" // ����� shader ��� ����
//		"in vec3 positionAttribute;" // vertex shader�� �Է°�(vertex ��ġ �Ӽ�)
//		"in vec3 colorAttribute;" // vertex ���� �Ӽ�
//		"out vec3 passColorAttribute;" // vertex shader�� ���, frag shader�� �Է�
//		"void main()"
//		"{"
//		"gl_Position = vec4(positionAttribute, 1.0);" // ������ǥ��� �����.
//		"passColorAttribute = colorAttribute;"
//		"}";
//
//	const char* fragmentShaderSource =
//		"#version 330 core\n"
//		"in vec3 passColorAttribute;"
//		"out vec4 fragmentColor;"
//		"void main()"
//		"{"
//		"fragmentColor = vec4(passColorAttribute, 1.0f);"
//		"}";
//
//	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); // ���� ���̴� id
//	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // ���� ���̴� �ҽ� �ۼ�
//	glCompileShader(vertexShader); // �� ������
//
//	GLint result;
//	GLchar errorLog[512];
//	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
//	if (!result)
//	{
//		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
//		cerr << "vertex shader ������ ����" << errorLog << endl;
//		glDeleteShader(vertexShader);
//		return false;
//	}
//
//	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
//	glCompileShader(fragmentShader);
//
//	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
//	if (!result)
//	{
//		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
//		cerr << "fragmentShader shader ������ ����" << errorLog << endl;
//		glDeleteShader(fragmentShader);
//		return false;
//	}
//
//	triangleShaderProgramID = glCreateProgram(); // ���α׷� ��ü ����
//	glAttachShader(triangleShaderProgramID, vertexShader); // ���α׷��� ���� ���̴� �Ҵ�
//	glAttachShader(triangleShaderProgramID, fragmentShader); // ���α׷��� ������ ���̴� �Ҵ�
//
//	glLinkProgram(triangleShaderProgramID); // GL �ھ�� ���α׷��� ����
//
//	glDeleteShader(vertexShader);
//	glDeleteShader(fragmentShader); // �ʼ��� �ƴ�����, �޸� ������ ���� �ʿ�
//
//	glGetProgramiv(triangleShaderProgramID, GL_LINK_STATUS, &result);
//	if (!result)
//	{
//		glGetProgramInfoLog(triangleShaderProgramID, 512, NULL, errorLog);
//		cerr << "shader program ���� ����: " << errorLog << endl;
//		return false;
//	}
//
//	return true;
//}
//
//bool defineVertexArrayObject()
//{
//	// �ﰢ���� �����ϴ� ������ ��ġ
//	float position[] = {
//	0.f, 0.5f, 0.f,
//	0.5f, -0.5f, 0.0f,
//	-0.5f, -0.5f, 0.0f
//	};
//
//	// �ﰢ���� �����ϴ� ������ ����
//	float color[] = {
//		1.f, 0.f, 0.f,
//		0.f, 1.f, 0.f,
//		0.f, 0.f, 1.f
//	};
//
//	// VBO�� ���� �����͸� GPU �޸𸮿� �����ϱ� ���� �̿��
//	// position �迭�� color �迭 ���� "�Ӽ�" �����͸� VBO�� �����Ѵ�.
//
//	// 1���� VBO �����ϰ�, ��ü ������ �̿�� ID ��ȯ
//	glGenBuffers(1, &trianglePositionVertexBufferObjectID); // GLsizei n, GLuint* buffers
//	// ���� ��ü�� ������ ������ ������ �����ϰ�, ������ �޸𸮸� ���j ��ü�� �Ҵ��� �غ���
//	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
//	cout << "trianglePositionVertexBufferObjectID: " << trianglePositionVertexBufferObjectID << endl;
//	glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);
//	// GL_STATIC_DRAW --> �ѹ� vertex data ���ε� ��, ������ ���� ��� ���
//	// GL_DYNAMIC_DRAW --> �ִϸ��̼� ó�� vertex �����Ͱ� ���� �ٲ�� ��� ���
//	// GL_STREAM_DRAW --> ����� �������̽�ó�� vertex �����Ͱ� ����ؼ� ����Ǵ� ��� ���
//	// vertex ������ �ٲ� ������ �ٽ� ���ε� �ȴ�.
//
//	glGenBuffers(1, &triangleColorVertexBufferObjectID);
//	cout << "triangleColorVertexBufferObjectID: " << triangleColorVertexBufferObjectID << endl;
//	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);
//
//	// ���� �Ӽ��� ���� VERTEX BUFFER OBJECT�� ���ε��ϰ�
//	// VBO���� VAO�� ���� �� ����
//	glGenVertexArrays(1, &triangleVertexArrayObject);
//	glBindVertexArray(triangleVertexArrayObject);
//
//	GLint positionAttribute = glGetAttribLocation(triangleShaderProgramID, "positionAttribute");
//	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
//	// ������ ���� ���̴� �Ӽ� ������ VBO ���� �����͸� VAO�� ����
//	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
//	glEnableVertexAttribArray(positionAttribute);
//
//	GLint colorAttribute = glGetAttribLocation(triangleShaderProgramID, "colorAttribute");
//	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
//	glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
//	glEnableVertexAttribArray(colorAttribute);
//
//	glBindVertexArray(0);
//
//	return true;	
//}
//
//void framebufferSizeCallback(GLFWwindow* window, int width, int height)
//{
//	glViewport(0, 0, width, height);
//	framebufferWidth = width;
//	framebufferHeight = height;
//}
//
//void errorCallback(int errorCode, const char* errorDescription)
//{
//	cerr << "Error: " << errorDescription << endl;
//}
//
//void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
//		glfwSetWindowShouldClose(window, GLFW_TRUE);
//}
//
//int main()
//{
//	glfwSetErrorCallback(errorCallback);
//
//	if (!glfwInit()) // ���̺귯�� �ʱ�ȭ
//	{
//		cerr << "GLFW �ʱ�ȭ ����" << endl;
//		exit(EXIT_FAILURE);
//	}
//
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // opengl ���� 3.3�� �̿��ϱ� ���� context�� �ʿ����� �˸�
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//	// core profile - ȣȯ���̳� deprecated ��� x
//	// compat profile - ȣȯ���̳� deprecated ��� o
//	// any profile - opengl 3.2 ���� ���� ���
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//	glfwWindowHint(GLFW_SAMPLES, 4); // ��Ƽ�ٸ����
//
//	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Example", NULL, NULL);
//
//	if (!window)
//	{
//		cerr << "Window ���� ����" << endl;
//		glfwTerminate();
//		exit(-1);
//	}
//
//	glfwMakeContextCurrent(window); // current context ���� // cc�� ���μ����ϸ� ������ ����� window ���´� ��� ����
//	glfwSetWindowSizeCallback(window, window_resized);
//	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
//	glfwSetKeyCallback(window, keyCallback);
//
//	glewExperimental = GL_TRUE;
//	GLenum errorCode = glewInit();
//
//	if (errorCode != GLEW_OK)
//	{
//		cerr << "GLEW �ʱ�ȭ ����" << glewGetErrorString(errorCode) << endl;
//		glfwTerminate();
//		exit(-1);
//	}
//
//	cout << glGetString(GL_VERSION) << endl;
//
//	int nr_extenstion = 0;
//	for (int i= 0; i < nr_extenstion; i++)
//		cout << glGetStringi(GL_EXTENSIONS, i) << endl;
//
//	initShaderProgram();
//	defineVertexArrayObject();
//
//	glfwSwapInterval(1);
//		
//	GLdouble lastTime = glfwGetTime();
//	int frame_num = 0, count = 0;
//
//	glUseProgram(triangleShaderProgramID); // ���α׷� ���� ���̴��� �̿��Ұ����� �̸� ����
//	glBindVertexArray(triangleVertexArrayObject); // �ش� VAO �� VBO�� draw�� ������ �̸� ����
//
//	while (!glfwWindowShouldClose(window))
//	{
//		double curTime = glfwGetTime();
//		frame_num++;
//
//		if (curTime - lastTime >= 1.0f)
//		{
//			lastTime = curTime;
//		}
//
//		glClearColor(0, 0, 0, 1);
//		glClear(GL_COLOR_BUFFER_BIT);
//
//		glDrawArrays(GL_TRIANGLES, 0, 3);
//
//		count++;
//
//		glfwSwapBuffers(window);
//		glfwPollEvents();
//	}
//
//	glUseProgram(0);
//	glBindVertexArray(0);
//
//	glDeleteProgram(triangleShaderProgramID);
//	glDeleteBuffers(1, &trianglePositionVertexBufferObjectID);
//	glDeleteBuffers(1, &triangleColorVertexBufferObjectID);
//	glDeleteVertexArrays(1, &triangleVertexArrayObject);
//	glfwTerminate();
//
//	//glfwDestroyWindow(window);
//	glfwTerminate();
//
//	return 0;
//}
//
//void window_resized(GLFWwindow* window, int width, int height)
//{
//	cout << "Window resized, new window size: " << width << " x " << height << endl;
//
//	glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // blue
//	glClear(GL_COLOR_BUFFER_BIT);
//	glfwSwapBuffers(window);
//}
