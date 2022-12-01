#include "BatchRenderer.h"
#include "GLErrorHandler.h"
#include <iostream>

#include "Shader.h"

void BatchRenderer::Init(PerformanceData* p)
{
	pData = p;
	Init();
}

void BatchRenderer::Init()
{
	ASSERT(data.VertexBuffer == nullptr) // VertexBuffer already assigned, Init was already called.
	data.VertexBuffer = new Vertex[MaxVertexCount];

	GLCall(glGenVertexArrays(1, &data.VA));
	GLCall(glBindVertexArray(data.VA));

	GLCall(glCreateBuffers(1, &data.VB));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, data.VB));
	GLCall(glBufferData(GL_ARRAY_BUFFER, MaxVertexCount*sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW));

	GLCall(glEnableVertexArrayAttrib(data.VA, 0));
	GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Position)));

	GLCall(glEnableVertexArrayAttrib(data.VA, 1));
	GLCall(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Color)));

	GLCall(glEnableVertexArrayAttrib(data.VA, 2));
	GLCall(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexCoord)));

	GLCall(glEnableVertexArrayAttrib(data.VA, 3));
	GLCall(glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexID)));


	std::vector<unsigned int> indices;
	indices.reserve(MaxIndexCount);
	unsigned int offset = 0;
	for (int i = 0; i < MaxIndexCount; i += 6)
	{
		indices.emplace_back(0 + offset);
		indices.emplace_back(1 + offset);
		indices.emplace_back(2 + offset);
		indices.emplace_back(2 + offset);
		indices.emplace_back(3 + offset);
		indices.emplace_back(0 + offset);

		offset += 4;
	}

	GLCall(glCreateBuffers(1, &data.IB));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.IB));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW));

	GLCall(glCreateTextures(GL_TEXTURE_2D, 1, &data.DefaultTexture));
	GLCall(glBindTexture(GL_TEXTURE_2D, data.DefaultTexture));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	uint32_t colour = 0xffffffff;
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &colour));
	memset(data.TextureSlots, 0, 32 * sizeof(uint32_t));
	data.TextureSlots[0] = data.DefaultTexture;

	int sampler[32];
	for (int i = 0; i < 32; i++)
		sampler[i] = i;
	auto loc = glGetUniformLocation(1, "u_Textures");
	GLCall(glUniform1iv(loc, 32, sampler));
};

void BatchRenderer::Close()
{
	GLCall(glDeleteVertexArrays(1, &data.VA));
	GLCall(glDeleteBuffers(1, &data.VB));
	GLCall(glDeleteBuffers(1, &data.IB));

	GLCall(glDeleteTextures(1, &data.DefaultTexture));

	delete[] data.VertexBuffer;
}

void BatchRenderer::Clear() const
{
	GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

void BatchRenderer::DrawQuad(const glm::vec2& postion, const glm::vec2& size, const glm::vec4& color)
{
	if (data.IndexCount >= MaxIndexCount)
	{
		EndBatch();
		Flush();
		BeginBatch();
	}

	float textureIndex = 0.0f;

	data.VertexBufferPointer->Position = { postion.x, postion.y };
	data.VertexBufferPointer->Color = color;
	data.VertexBufferPointer->TexCoord = {0.0f, 0.0f};
	data.VertexBufferPointer->TexID = textureIndex;
	data.VertexBufferPointer++;

	data.VertexBufferPointer->Position = { postion.x+size.x, postion.y };
	data.VertexBufferPointer->Color = color;
	data.VertexBufferPointer->TexCoord = {1.0f, 0.0f};
	data.VertexBufferPointer->TexID = textureIndex;
	data.VertexBufferPointer++;

	data.VertexBufferPointer->Position = { postion.x+size.x, postion.y+size.y };
	data.VertexBufferPointer->Color = color;
	data.VertexBufferPointer->TexCoord = {1.0f, 1.0f};
	data.VertexBufferPointer->TexID = textureIndex;
	data.VertexBufferPointer++;

	data.VertexBufferPointer->Position = { postion.x, postion.y + size.y };
	data.VertexBufferPointer->TexCoord = {0.0f, 1.0f};
	data.VertexBufferPointer->TexID = textureIndex;
	data.VertexBufferPointer->Color = color;
	data.VertexBufferPointer++;

	data.IndexCount += 6;

	if(pData) pData->numQuadsThisFrame++;
}

void BatchRenderer::DrawQuad(const glm::vec2& postion, const glm::vec2& size, uint32_t texID)
{
	if (data.IndexCount >= MaxIndexCount || data.TextureSlotIndex > 31)
	{
		EndBatch();
		Flush();
		BeginBatch();
	}

	glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	float textureIndex = 0.0f;
	for (uint32_t i = 1; i < data.TextureSlotIndex; i++)
	{
		if (data.TextureSlots[i] == texID)
		{
			textureIndex = (float)i;
			break;
		}
	}

	if (textureIndex == 0.0f)
	{
		textureIndex = (float)data.TextureSlotIndex;
		data.TextureSlots[data.TextureSlotIndex] = texID;
		data.TextureSlotIndex++;
	}

	data.VertexBufferPointer->Position = { postion.x, postion.y };
	data.VertexBufferPointer->Color = color;
	data.VertexBufferPointer->TexCoord = { 0.0f, 0.0f };
	data.VertexBufferPointer->TexID = textureIndex;
	data.VertexBufferPointer++;

	data.VertexBufferPointer->Position = { postion.x + size.x, postion.y };
	data.VertexBufferPointer->Color = color;
	data.VertexBufferPointer->TexCoord = { 1.0f, 0.0f };
	data.VertexBufferPointer->TexID = textureIndex;
	data.VertexBufferPointer++;

	data.VertexBufferPointer->Position = { postion.x + size.x, postion.y + size.y };
	data.VertexBufferPointer->Color = color;
	data.VertexBufferPointer->TexCoord = { 1.0f, 1.0f };
	data.VertexBufferPointer->TexID = textureIndex;
	data.VertexBufferPointer++;

	data.VertexBufferPointer->Position = { postion.x, postion.y + size.y };
	data.VertexBufferPointer->Color = color;
	data.VertexBufferPointer->TexCoord = { 0.0f, 1.0f };
	data.VertexBufferPointer->TexID = textureIndex;
	data.VertexBufferPointer++;

	data.IndexCount += 6;

	if (pData) pData->numQuadsThisFrame++;
}

void BatchRenderer::BeginBatch()
{
	data.VertexBufferPointer = data.VertexBuffer;
}

void BatchRenderer::Flush()
{
	GLCall(glBindVertexArray(data.VA));
	GLCall(glBindVertexArray(data.VB));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.IB));

	glDrawElements(GL_TRIANGLES, data.IndexCount, GL_UNSIGNED_INT, nullptr);

	data.IndexCount = 0;
	if (pData) pData->numDrawsThisFrame++;
}

void BatchRenderer::EndBatch() const
{
	GLsizeiptr size = (uint8_t*)data.VertexBufferPointer - (uint8_t*)data.VertexBuffer; //cast to byte
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, data.VB));
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, 0, size, data.VertexBuffer));
}