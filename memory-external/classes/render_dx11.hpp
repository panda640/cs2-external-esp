#pragma once
#include <string>
#include <vector>
#include "imgui.h"
#include "vector.hpp"

namespace render
{
	inline void DrawLine(ImDrawList* drawList, float x1, float y1, float x2, float y2, ImU32 color, float thickness = 2.0f)
	{
		drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
	}

	inline void DrawCircle(ImDrawList* drawList, float x, float y, float radius, ImU32 color, float thickness = 2.0f)
	{
		drawList->AddCircle(ImVec2(x, y), radius, color, 0, thickness);
	}

	inline void DrawBorderBox(ImDrawList* drawList, float x, float y, float w, float h, ImU32 color, float thickness = 2.0f)
	{
		drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.0f, 0, thickness);
	}

	inline void DrawFilledBox(ImDrawList* drawList, float x, float y, float w, float h, ImU32 color)
	{
		drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color);
	}

	inline void RenderText(ImDrawList* drawList, float x, float y, const char* text, ImU32 color, float size = 15.0f, bool outline = true)
	{
		if (outline)
		{
			drawList->AddText(NULL, size, ImVec2(x + 1, y + 1), ImColor(0, 0, 0, 255), text);
			drawList->AddText(NULL, size, ImVec2(x - 1, y - 1), ImColor(0, 0, 0, 255), text);
			drawList->AddText(NULL, size, ImVec2(x + 1, y - 1), ImColor(0, 0, 0, 255), text);
			drawList->AddText(NULL, size, ImVec2(x - 1, y + 1), ImColor(0, 0, 0, 255), text);
		}
		drawList->AddText(NULL, size, ImVec2(x, y), color, text);
	}
}
