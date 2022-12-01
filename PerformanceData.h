#pragma once
struct PerformanceData
{
	unsigned int numDrawsThisFrame;
	unsigned int numQuadsThisFrame;

	float fps = 0.0f;

	float avgfps = 0.0f;
	float alpha = 0.9f;

	float drawTime = 0.0f;
};

