#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <array>
#include <sstream>

#include "Vertex.h"
#include <glm\gtc\matrix_transform.hpp>

#include "Shader.h"

#include "BatchRenderer.h"
#include "PerformanceData.h"

#include <chrono>

#include "stb_image.h"

#include "GLErrorHandler.h"

PerformanceData pData;

GLuint LoadTexture(std::string path, int slot)
{
	int width;
	int height;
	int bits;

	stbi_set_flip_vertically_on_load(1);
	unsigned char* pixelBuffer; 
	pixelBuffer = stbi_load(path.c_str(), &width, &height, &bits, STBI_rgb);

	GLuint texID;

	GLCall(glCreateTextures(GL_TEXTURE_2D, 1, &texID));
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, texID));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer))

	if (pixelBuffer)
		stbi_image_free(pixelBuffer);

	return texID;
};

const float UpdateWindowHeaderTime = 0.05f;

int main()
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(800, 800, "Batched Rendering Test", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0); // 1 to cap to refresh rate

	std::cout << glGetString(GL_VERSION) << '\n';

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	if (glewInit() != GLEW_OK)
		std::cout << "ERROR!" << std::endl;

	Shader shader("BatchColour.shader");
	shader.Bind();

	BatchRenderer br;
	br.Init(&pData);

	GLuint don = LoadTexture("res/textures/don2.png", 1);
	GLuint don2 = LoadTexture("res/textures/don2Prime.png", 2);

	std::chrono::time_point <std::chrono::high_resolution_clock> t1;
	t1 = std::chrono::high_resolution_clock::now();

	std::chrono::time_point <std::chrono::high_resolution_clock> windowUpdate = std::chrono::high_resolution_clock::now();


	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{

		auto t2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsedTime = t2 - t1;

		//Draw
		//if (elapsedTime.count() >= 0.01667f)                        //only draw 60 frames/second
		{
			std::chrono::time_point <std::chrono::high_resolution_clock> dt1;
			dt1 = std::chrono::high_resolution_clock::now();

			pData.numDrawsThisFrame = 0;
			pData.numQuadsThisFrame = 0;

			br.Clear();

			br.BeginBatch();

			float xOffset = 0;
			float yOffset = 0;
			float size = 0.005f;
			for (float y = -1; y < 1; y += size)
			{
				for (float x = -1; x < 1; x += size)
				{
					glm::vec4 color = { (x + 1) / 2.0f, 0.2f, (y + 1) / 2.0f, 1.0f };

					br.DrawQuad({ -1.0f + xOffset, -1.0f + yOffset }, { size, size }, color);
					xOffset += size;
				}
				xOffset = 0;
				yOffset += size;
			}

			xOffset = 0;
			yOffset = 0;
			size = 0.5f;
			for (int y = 0; y < 3; y++)
			{
				for (int x = 0; x < 3; x++)
				{
					//glm::vec4 color = ((x + y) % 2 == 0) ? glm::vec4(0.0f, 0.0f, 0.0f, 0.4f) : glm::vec4(1.0f, 1.0f, 1.0f, 0.4f);
					GLuint texId = ((x + y) % 2 == 0) ? don : don2;

					br.DrawQuad({ -0.75f + xOffset, -0.75f + yOffset }, { size, size }, texId);
					xOffset += size;
				}
				xOffset = 0;
				yOffset += size;
			}

			br.EndBatch();

			shader.Bind();
			br.Flush();

			/* Swap front and back buffers */
			glfwSwapBuffers(window);

			/* Poll for and process events */
			glfwPollEvents();

			t1 = t2;

			pData.fps = 1.0 / elapsedTime.count();
			pData.avgfps = pData.alpha * pData.avgfps + (1.0f - pData.alpha) * pData.fps;

			auto dt2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> dTime = dt2 - dt1;
			pData.drawTime = dTime.count() * 1000;

			std::chrono::duration<double> windoUp = t1 - windowUpdate;
			if (windoUp.count() > UpdateWindowHeaderTime)
			{
				std::stringstream ss;
				ss.precision(4);
				ss << "Batched Render Test - [ Quads: " << pData.numQuadsThisFrame << ", Draws: " << pData.numDrawsThisFrame << std::fixed << ", FPS: " << pData.fps << ", Avg FPS: " << pData.avgfps << ", Draw Time (ms): " << pData.drawTime << " ]";
				glfwSetWindowTitle(window, ss.str().c_str());
				windowUpdate = t1;
			}
		}
	}

	glfwTerminate();
	return 0;
}