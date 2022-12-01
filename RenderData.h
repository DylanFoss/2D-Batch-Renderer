#pragma once
#include "Vertex.h"
#include "GL/glew.h"
struct RenderData
{
	GLuint VA=0;
	GLuint VB=0;
	GLuint IB=0;

	uint32_t IndexCount = 0;

	Vertex* VertexBuffer = nullptr;
	Vertex* VertexBufferPointer = nullptr;

	uint32_t DefaultTexture;
	uint32_t TextureSlots[32];
	unsigned int TextureSlotIndex = 1;
};