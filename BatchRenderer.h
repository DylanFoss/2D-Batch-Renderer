#pragma once
#include "Shader.h"

#include "RenderData.h"
#include "PerformanceData.h"

class BatchRenderer
{
private:

	RenderData data;
	PerformanceData* pData = nullptr;

	size_t MaxQuadCount = 1000;
	size_t MaxVertexCount = MaxQuadCount * 4;
	size_t MaxIndexCount = MaxQuadCount * 6;
public:

	void Init();
	void Init(PerformanceData* perfData);
	void Close();

	void Clear() const;
	void DrawQuad(const glm::vec2& postion, const glm::vec2& size, const glm::vec4& color);
	void DrawQuad(const glm::vec2& postion, const glm::vec2& size, uint32_t texID);

	void BeginBatch();
	void Flush();
	void EndBatch() const;
};

