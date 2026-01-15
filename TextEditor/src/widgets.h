#pragma once
#include <imgui/imgui.h>
#include "data_structures.h"

namespace ImGui {
void GetWindowRegion(ImVec2* min, ImVec2* max) {
	*min = ImGui::GetWindowContentRegionMin();
	*max = ImGui::GetWindowContentRegionMax();

	min->x += ImGui::GetWindowPos().x;
	min->y += ImGui::GetWindowPos().y;
	max->x += ImGui::GetWindowPos().x;
	max->y += ImGui::GetWindowPos().y;
}

void Text(String str) {
	ImGui::TextUnformatted(StrFirst(str), StrLast(str) + 1);
}

bool Button(String label, const ImVec2& size = ImVec2(0, 0)) {
	char buf[1024];
	te_assert(sizeof(buf) > label.length);
	StrToCstr(label, buf, sizeof(buf));

	return ImGui::Button(buf, size);
}

void ArenaWidget(Arena* arena, const char* name) {
	f32 divider = Kilobytes(1);
	ImGui::Text("%s: size/comm/res %.2fKB/%.2fKB/%.2fKB",
							name,
							arena->used / divider, arena->commited / divider, arena->reserved / divider);
}

void DrawCursor(int cursorIndex, String text, ImU32 color = IM_COL32_WHITE) {
	int lineBreakCount = 0;	 // количество строк перед курсором
	int lineStart = 0;			 // индекс, с которого начинается строка
	int lineLen = 0;				 // длина строки

	// находим количество строк перед курсором, начало последней строки и длину этой строки
	for (s64 i = 0; i < cursorIndex && i < text.length; i++) {
		lineLen++;
		if (text[i] == '\n') {
			lineBreakCount++;
			lineLen = 0;
		}
		if (i > 0 && text[i - 1] == '\n') {
			lineStart = i;
		}
	}

	// находим размер строки до курсора в пикселях, чтобы отрисовать курсор в нужном месте
	ImVec2 lineSize = ImGui::CalcTextSize(&text[lineStart], &text[lineStart] + lineLen);

	ImVec2 vMin, vMax;
	GetWindowRegion(&vMin, &vMax);

	ImVec2 cursorPos1(lineSize.x + vMin.x, lineBreakCount * lineSize.y + vMin.y);
	ImVec2 cursorPos2(cursorPos1);
	cursorPos2.y += lineSize.y;

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddLine(cursorPos1, cursorPos2, color, 1.0f);
}
}	 // namespace ImGui

void SetDarkTheme() {
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// Primary background
	colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);		// #131318
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);	// #131318

	colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);

	// Headers
	colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.40f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.35f, 1.00f);

	// Buttons
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.32f, 0.40f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.38f, 0.50f, 1.00f);

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.50f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.38f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

	// Borders
	colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	// Text
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);

	// Highlights
	colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.80f, 1.00f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.70f, 1.00f, 0.50f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.80f, 1.00f, 0.75f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.70f, 0.90f, 1.00f, 1.00f);

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.55f, 1.00f);

	// Style tweaks
	style.WindowRounding = 5.0f;
	style.FrameRounding = 5.0f;
	style.GrabRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.PopupRounding = 5.0f;
	style.ScrollbarRounding = 5.0f;
	style.WindowPadding = ImVec2(10, 10);
	style.FramePadding = ImVec2(6, 4);
	style.ItemSpacing = ImVec2(8, 6);
	style.PopupBorderSize = 0.f;
}