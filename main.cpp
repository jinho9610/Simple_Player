#include "include/GL/glew.h"
#include "include/GLFW/glfw3.h"
#include "stb_image.h"
#include <iostream>
#include "opencv2/core/core.hpp" // Mat class와 각종 data structure 및 산술 루틴을 포함하는 헤더
#include "opencv2/highgui/highgui.hpp" // GUI와 관련된 요소를 포함하는 헤더(imshow 등)
#include "opencv2/imgproc/imgproc.hpp" // 각종 이미지 처리 함수를 포함하는 헤더
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
		"#version 330 core\n" // 사용할 shader 언어 버전
		"in vec3 positionAttribute;" // vertex shader의 입력값(vertex 위치 속성)
		"in vec3 colorAttribute;" // vertex 색상 속성
		"in vec2 texture;"
		"out vec3 passColorAttribute;" // vertex shader의 출력, frag shader의 입력
		"out vec2 out_texture;"
		"void main()"
		"{"
		"gl_Position = vec4(positionAttribute, 1.0);" // 동차좌표계로 만든다.
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

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); // 정점 쉐이더 id
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // 정점 쉐이더 소스 작성
	glCompileShader(vertexShader); // 및 컴파일

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "vertex shader 컴파일 실패" << errorLog << endl;
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
		cerr << "fragmentShader shader 컴파일 실패" << errorLog << endl;
		glDeleteShader(fragmentShader);
		return false;
	}

	triangleShaderProgramID = glCreateProgram(); // 프로그램 객체 생성
	glAttachShader(triangleShaderProgramID, vertexShader); // 프로그램에 정점 쉐이더 할당
	glAttachShader(triangleShaderProgramID, fragmentShader); // 프로그램에 프래그 쉐이더 할당

	glLinkProgram(triangleShaderProgramID); // GL 코어와 프로그램의 연결

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader); // 필수는 아니지만, 메모리 관리를 위해 필요

	glGetProgramiv(triangleShaderProgramID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(triangleShaderProgramID, 512, NULL, errorLog);
		cerr << "shader program 연결 실패: " << errorLog << endl;
		return false;
	}

	return true;
}

bool defineVertexArrayObject()
{
	// 삼각형을 구성하는 정점의 위치
	float position[] = {
		1.0f, 1.0f, 0.0f, // 우측 상단
		1.0f, -1.0f, 0.0f, // 우측 하단
		-1.0f, -1.0f, 0.0f, //좌측 하단
		-1.0f, 1.0f, 0.0f // 좌측 상단
	};

	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	// 삼각형을 구성하는 정점의 색상
	float color[] = {
		1.f, 0.f, 0.f,
		0.f, 1.f, 0.f,
		0.f, 0.f, 1.f
	};

	// 정점 데이터로 사용될 텍스처 좌표
	float texCoord[] = {
		1.0f, 0.0f, // 우측 하단
		1.0f, 1.0f, // 우측 상단
		0.0f, 1.0f, // // 좌측 상단
		0.0f, 0.0f, // 좌측 하단
	};

	// VBO는 정점 데이터를 GPU 메모리에 복사하기 위해 이용됨
	// position 배열과 color 배열 내의 "속성" 데이터를 VBO에 복사한다.

	// 1개의 VBO 생성하고, 객체 참조시 이용될 ID 반환
	glGenBuffers(1, &trianglePositionVertexBufferObjectID); // GLsizei n, GLuint* buffers
	// 버퍼 객체에 저장할 데이터 종류를 지정하고, 최적의 메모리를 버펖 객체에 할당한 준비를함
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	cout << "trianglePositionVertexBufferObjectID: " << trianglePositionVertexBufferObjectID << endl;
	glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);
	// GL_STATIC_DRAW --> 한번 vertex data 업로드 후, 변경이 없는 경우 사용
	// GL_DYNAMIC_DRAW --> 애니메이션 처럼 vertex 데이터가 자주 바뀌는 경우 사용
	// GL_STREAM_DRAW --> 사용자 인터페이스처럼 vertex 데이터가 계속해서 변경되는 경우 사용
	// vertex 데이터 바뀔 때마다 다시 업로드 된다.

	glGenBuffers(1, &triangleColorVertexBufferObjectID);
	cout << "triangleColorVertexBufferObjectID: " << triangleColorVertexBufferObjectID << endl;
	glBindBuffer(GL_ARRAY_BUFFER, triangleColorVertexBufferObjectID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);

	glGenBuffers(1, &triTexCoord_id);
	glBindBuffer(GL_ARRAY_BUFFER, triTexCoord_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoord), texCoord, GL_STATIC_DRAW);

	// 정점 속성을 개별 VERTEX BUFFER OBJECT에 바인딩하고
	// VBO들을 VAO로 묶을 수 있음
	glGenVertexArrays(1, &triangleVertexArrayObject);
	glBindVertexArray(triangleVertexArrayObject);

	GLint positionAttribute = glGetAttribLocation(triangleShaderProgramID, "positionAttribute");
	glBindBuffer(GL_ARRAY_BUFFER, trianglePositionVertexBufferObjectID);
	// 지정한 정점 쉐이더 속성 변수와 VBO 내의 데이터를 VAO에 저장
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

	if (!glfwInit()) // 라이브러리 초기화
	{
		cerr << "GLFW 초기화 실패" << endl;
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // opengl 버전 3.3을 이용하기 위한 context가 필요함을 알림
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// core profile - 호환성이나 deprecated 고려 x
	// compat profile - 호환성이나 deprecated 고려 o
	// any profile - opengl 3.2 이하 버전 사용
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4); // 안티앨리어싱

	GLFWwindow* window = glfwCreateWindow(frame.cols, frame.rows, "OpenGL Example", NULL, NULL);

	if (!window)
	{
		cerr << "Window 생성 실패" << endl;
		glfwTerminate();
		exit(-1);
	}

	glfwMakeContextCurrent(window); // current context 설정 // cc를 새로설정하면 이전에 사용한 window 상태는 모두 무시
	glfwSetWindowSizeCallback(window, window_resized);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, keyCallback);

	glewExperimental = GL_TRUE;
	GLenum errorCode = glewInit();

	if (errorCode != GLEW_OK)
	{
		cerr << "GLEW 초기화 실패" << glewGetErrorString(errorCode) << endl;
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

	glUseProgram(triangleShaderProgramID); // 프로그램 내의 쉐이더를 이용할것임을 미리 밝힘
	glBindVertexArray(triangleVertexArrayObject); // 해당 VAO 내 VBO를 draw할 것임을 미리 밝힘
	//GLuint texid = CreateTexture("awesomeface2.png");
	glUniform1i(glGetUniformLocation(triangleShaderProgramID, "tex"), 0); // tex라는 uniform 변수에 텍스처를 전달

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
			glActiveTexture(GL_TEXTURE0); // 렌더링에 사용할 텍스처 활성화
			glBindTexture(GL_TEXTURE_2D, texID); // 및 바인딩
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
