#include "include/GL/glew.h"
#include "include/GLFW/glfw3.h"
#include "stb_image.h"
#include <iostream>
#include "opencv2/core/core.hpp" // Mat class�� ���� data structure �� ��� ��ƾ�� �����ϴ� ���
#include "opencv2/highgui/highgui.hpp" // GUI�� ���õ� ��Ҹ� �����ϴ� ���(imshow ��)
#include "opencv2/imgproc/imgproc.hpp" // ���� �̹��� ó�� �Լ��� �����ϴ� ���
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <condition_variable>

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "lib/glew32.lib")
#pragma comment(lib, "lib/glfw3.lib")
#pragma commnet(lib, "lib/FreeImage.lib")

#define endl '\n'

using namespace cv;
using namespace std;

void window_resized(GLFWwindow* window, int width, int height);


int framebufferWidth, framebufferHeight;
GLuint triangleVertexArrayObject, triangleShaderPromgramID;
GLuint triangleShaderProgramID, trianglePositionVertexBufferObjectID, triangleColorVertexBufferObjectID;
GLuint triTexCoord_id;
GLuint EBO;

GLuint texID;
Mat frame;
VideoCapture cap("./Student - 69624.mp4");
//VideoCapture cap("./videoplayback.mp4");
//VideoCapture cap("./Giraffe.mp4");
uchar* frame_data;
int frame_width, frame_height;
queue<GLubyte*> q_frame;
bool is_eov = false;

GLuint CreateTexture(char const* filename)
{
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	GLubyte* textureData = stbi_load(filename, &width, &height, &nrChannels, 0);

	GLuint tex_id;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, 0);

	return tex_id;
}

void read_video()
{
	if (!cap.read(frame))
		is_eov = true;

	frame_data = frame.data;
}

void packet_extractor(mutex* mtx, condition_variable* cv)
{
	while (cap.read(frame))
	{
		{
			unique_lock<mutex> ul(*mtx);
			cout << "thread called" << endl;
			//read_video();
			GLubyte* tmp = new GLubyte[frame_width * frame_height * 3];
			memcpy(tmp, frame.data, frame_width * frame_height * 3);
			
			q_frame.push(tmp);
			(*cv).notify_one();
		}
	}
	is_eov = true;
	cout << "thread_termination -- " << q_frame.size() << endl;
}

void Frame_to_Texture(GLubyte* data)
{
	glDeleteTextures(1, &texID);
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame_width, frame_height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

bool initShaderProgram()
{
	const char* vertexShaderSource =
		"#version 330 core\n" // ����� shader ��� ����
		"in vec3 positionAttribute;" // vertex shader�� �Է°�(vertex ��ġ �Ӽ�)
		"in vec3 colorAttribute;" // vertex ���� �Ӽ�
		"in vec2 texture;"
		"out vec3 passColorAttribute;" // vertex shader�� ���, frag shader�� �Է�
		"out vec2 out_texture;"
		"void main()"
		"{"
		"gl_Position = vec4(positionAttribute, 1.0);" // ������ǥ��� �����.
		"passColorAttribute = colorAttribute;"
		"out_texture = texture;"
		"}";

	const char* fragmentShaderSource =
		"#version 330 core\n"
		"in vec3 passColorAttribute;"
		"in vec2 out_texture;"
		"out vec4 fragmentColor;"
		"uniform sampler2D tex;"
		"void main()"
		"{"
		//"fragmentColor = vec4(passColorAttribute, 1.0f);"
		"fragmentColor = texture(tex, out_texture);"
		"}";

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); // ���� ���̴� id
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // ���� ���̴� �ҽ� �ۼ�
	glCompileShader(vertexShader); // �� ������

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "vertex shader ������ ����" << errorLog << endl;
		glDeleteShader(vertexShader);
		return false;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		cerr << "fragmentShader shader ������ ����" << errorLog << endl;
		glDeleteShader(fragmentShader);
		return false;
	}

	triangleShaderProgramID = glCreateProgram(); // ���α׷� ��ü ����
	glAttachShader(triangleShaderProgramID, vertexShader); // ���α׷��� ���� ���̴� �Ҵ�
	glAttachShader(triangleShaderProgramID, fragmentShader); // ���α׷��� ������ ���̴� �Ҵ�

	glLinkProgram(triangleShaderProgramID); // GL �ھ�� ���α׷��� ����

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader); // �ʼ��� �ƴ�����, �޸� ������ ���� �ʿ�

	glGetProgramiv(triangleShaderProgramID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(triangleShaderProgramID, 512, NULL, errorLog);
		cerr << "shader program ���� ����: " << errorLog << endl;
		return false;
	}

	return true;
}

bool defineVertexArrayObject()
{
	// �ﰢ���� �����ϴ� ������ ��ġ
	float position[] = {
		1.0f, 1.0f, 0.0f, // ���� ���
		1.0f, -1.0f, 0.0f, // ���� �ϴ�
		-1.0f, -1.0f, 0.0f, //���� �ϴ�
		-1.0f, 1.0f, 0.0f // ���� ���
	};

	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	// �ﰢ���� �����ϴ� ������ ����
	float color[] = {
		1.f, 0.f, 0.f,
		0.f, 1.f, 0.f,
		0.f, 0.f, 1.f
	};

	// ���� �����ͷ� ���� �ؽ�ó ��ǥ
	float texCoord[] = {
		1.0f, 0.0f, // ���� �ϴ�
		1.0f, 1.0f, // ���� ���
		0.0f, 1.0f, // // ���� ���
		0.0f, 0.0f, // ���� �ϴ�
	};

	// VBO�� ���� �����͸� GPU �޸𸮿� �����ϱ� ���� �̿��
	// position �迭�� color �迭 ���� "�Ӽ�" �����͸� VBO�� �����Ѵ�.

	// 1���� VBO �����ϰ�, ��ü ������ �̿�� ID ��ȯ
	glGenBuffers(1, &trianglePositionVertexBufferObjectID); // GLsizei n, GLuint* buffers
	// ���� ��ü�� ������ ������ ������ �����ϰ�, ������ �޸𸮸� ���j ��ü�� �Ҵ��� �غ���
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	cout << "trianglePositionVertexBufferObjectID: " << trianglePositionVertexBufferObjectID << endl;
	glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);
	// GL_STATIC_DRAW --> �ѹ� vertex data ���ε� ��, ������ ���� ��� ���
	// GL_DYNAMIC_DRAW --> �ִϸ��̼� ó�� vertex �����Ͱ� ���� �ٲ�� ��� ���
	// GL_STREAM_DRAW --> ����� �������̽�ó�� vertex �����Ͱ� ����ؼ� ����Ǵ� ��� ���
	// vertex ������ �ٲ� ������ �ٽ� ���ε� �ȴ�.

	glGenBuffers(1, &triangleColorVertexBufferObjectID);
	cout << "triangleColorVertexBufferObjectID: " << triangleColorVertexBufferObjectID << endl;
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);

	glGenBuffers(1, &triTexCoord_id);
	glBindBuffer(GL_ARRAY_BUFFER, triTexCoord_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoord), texCoord, GL_STATIC_DRAW);

	// ���� �Ӽ��� ���� VERTEX BUFFER OBJECT�� ���ε��ϰ�
	// VBO���� VAO�� ���� �� ����
	glGenVertexArrays(1, &triangleVertexArrayObject);
	glBindVertexArray(triangleVertexArrayObject);

	GLint positionAttribute = glGetAttribLocation(triangleShaderProgramID, "positionAttribute");
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	// ������ ���� ���̴� �Ӽ� ������ VBO ���� �����͸� VAO�� ����
	glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionAttribute);

	GLint colorAttribute = glGetAttribLocation(triangleShaderProgramID, "colorAttribute");
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colorAttribute);

	GLint texCoordAttribute = glGetAttribLocation(triangleShaderProgramID, "texture");
	glBindBuffer(GL_ARRAY_BUFFER, triTexCoord_id);
	glVertexAttribPointer(texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texCoordAttribute);
	
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	return true;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	framebufferWidth = width;
	framebufferHeight = height;
}

void errorCallback(int errorCode, const char* errorDescription)
{
	cerr << "Error: " << errorDescription << endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void read_img()
{
	frame_data = frame.data;
}

int main()
{
	mutex mtx;
	condition_variable cv;

	read_video();
	frame_width = frame.cols, frame_height = frame.rows;

	frame_data = frame.data;
	int width = frame.cols, height = frame.rows;
	
	glfwSetErrorCallback(errorCallback);

	if (!glfwInit()) // ���̺귯�� �ʱ�ȭ
	{
		cerr << "GLFW �ʱ�ȭ ����" << endl;
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // opengl ���� 3.3�� �̿��ϱ� ���� context�� �ʿ����� �˸�
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// core profile - ȣȯ���̳� deprecated ��� x
	// compat profile - ȣȯ���̳� deprecated ��� o
	// any profile - opengl 3.2 ���� ���� ���
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4); // ��Ƽ�ٸ����

	GLFWwindow* window = glfwCreateWindow(frame.cols, frame.rows, "OpenGL Example", NULL, NULL);

	if (!window)
	{
		cerr << "Window ���� ����" << endl;
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window); // current context ���� // cc�� ���μ����ϸ� ������ ����� window ���´� ��� ����
	glfwSetWindowSizeCallback(window, window_resized);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, keyCallback);

	glewExperimental = GL_TRUE;
	GLenum errorCode = glewInit();

	if (errorCode != GLEW_OK)
	{
		cerr << "GLEW �ʱ�ȭ ����" << glewGetErrorString(errorCode) << endl;
		glfwTerminate();
		exit(-1);
	}

	cout << glGetString(GL_VERSION) << endl;

	int nr_extenstion = 0;
	for (int i = 0; i < nr_extenstion; i++)
		cout << glGetStringi(GL_EXTENSIONS, i) << endl;

	initShaderProgram();
	defineVertexArrayObject();

	glfwSwapInterval(1);

	GLdouble lastTime = glfwGetTime();
	int frame_num = 0, count = 0;

	glUseProgram(triangleShaderProgramID); // ���α׷� ���� ���̴��� �̿��Ұ����� �̸� ����
	glBindVertexArray(triangleVertexArrayObject); // �ش� VAO �� VBO�� draw�� ������ �̸� ����
	//GLuint texid = CreateTexture("awesomeface2.png");
	glUniform1i(glGetUniformLocation(triangleShaderProgramID, "tex"), 0); // tex��� uniform ������ �ؽ�ó�� ����

	thread extractor(packet_extractor, &mtx, &cv);
	this_thread::sleep_for(chrono::milliseconds(100));

	while (!glfwWindowShouldClose(window))
	{
		{
			unique_lock<mutex> ul(mtx);
			cout << "Frame Q Empty - " << q_frame.size() << endl;
			if (q_frame.empty())
				cv.wait(ul, [&]()->bool {return !q_frame.empty(); });
		}
		while (!q_frame.empty())
		{
			cout << "Q size - " << q_frame.size() << endl;
			GLubyte* tmp = q_frame.front();
			Frame_to_Texture(tmp);
			q_frame.pop();
			delete[] tmp;
			glActiveTexture(GL_TEXTURE0); // �������� ����� �ؽ�ó Ȱ��ȭ
			glBindTexture(GL_TEXTURE_2D, texID); // �� ���ε�
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		if (is_eov)
			exit(0);
	}

	glUseProgram(0);
	glBindVertexArray(0);

	glDeleteProgram(triangleShaderProgramID);
	glDeleteBuffers(1, &trianglePositionVertexBufferObjectID);
	glDeleteBuffers(1, &triangleColorVertexBufferObjectID);
	glDeleteVertexArrays(1, &triangleVertexArrayObject);
	glfwTerminate();

	//glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void window_resized(GLFWwindow* window, int width, int height)
{
	cout << "Window resized, new window size: " << width << " x " << height << endl;

	glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // blue
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(window);
}
