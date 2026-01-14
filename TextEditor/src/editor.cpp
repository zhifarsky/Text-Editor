#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_internal.h>

#include "data_structures.h"
#include "editor.h"
#include "localization.h"
#include "text.h"
#include "tools.h"
#include "widgets.h"

#define MIN_FONT_SIZE 14
#define MAX_FONT_SIZE 40
#define DEFAULT_FONT_SIZE 20

#define MAX_TABS_COUNT 128

ImFont* g_fontRegular;
// const char* g_fontRegularPath = ".\\res\\fonts\\Roboto-Regular.ttf";
const char* g_fontRegularPath = "C:/Windows/Fonts/msyh.ttc";

u32 g_lastDockNodeId;
bool g_isSettingsOpen;
bool g_isCommandPaletteOpen;
bool g_commandPaletteFocusInput;

void OpenCommandPalette() {
	g_isCommandPaletteOpen = true;
	g_commandPaletteFocusInput = true;
}

//
// Editor State
//

struct editor_state {
	memory_arena arena;
	memory_arena frameArena;

	array_dynamic<text_tab> tabs;	 // TODO: указатели next в text_tab для freeList
	u32 tabIDCounter;

	u32 fontSize;
	s32 currentTextTabID;

	bool isInitialized;
};

text_tab* GetCurrentTab(editor_state* editor) {
	if (editor->currentTextTabID < 0)
		return NULL;

	for (u64 i = 0; i < editor->tabs.count; i++) {
		if (editor->tabs[i].id == editor->currentTextTabID)
			return &editor->tabs[i];
	}

	return NULL;
}

void AddTextTab(editor_state* editor) {
	// text_tab newTab = {0};

	// newTab.arena = ArenaAlloc(Megabytes(1));

	// newTab.encoding = Encoding_UTF8;
	// newTab.id = editor->tabIDCounter++;

	// newTab.added = Array<char>(&newTab.arena);
	// newTab.nodes = Array<text_node>(&newTab.arena);

	// newTab.isOpen = true;

	// editor->currentTextTabID = newTab.id;
	// Push<text_tab>(&editor->tabs, &editor->arena, newTab);

	text_tab newTab = TextTab(editor->tabIDCounter++);
	editor->currentTextTabID = newTab.id;
	Push<text_tab>(&editor->tabs, &editor->arena, newTab);
}

void CloseTextTab(editor_state* editor, u32 tabIndex) {
	text_tab* tab = &editor->tabs[tabIndex];
	ArenaRelease(&tab->arena);
	RemoveFast<text_tab>(&editor->tabs, tabIndex);
}

void ExecuteCommand(command_type commandType, editor_state* editorState) {
	switch (commandType) {
		case Command_New:
			AddTextTab(editorState);
			break;
		case Command_ShowCommandPalette:
			OpenCommandPalette();
			break;
		case Command_OpenSettings:
			g_isSettingsOpen = true;
			break;
		case Command_IncreaseFontSize:
			editorState->fontSize += 5;
			break;
		case Command_DecreaseFontSize:
			editorState->fontSize -= 5;
			break;
		default:
			platform_Print("Not implemented: ");
			platform_Print(g_hotkeyMappings[commandType].label);
			platform_Print("\n");
			break;
	}
}

void TestCode(program_input* input) {
}

static editor_state g_editorState = {0};

//
// Editor Main
//

void EditorUpdate(event_queue* eventQueue, program_input* input) {
	TestCode(input);

	editor_state* editorState = &g_editorState;

	//
	// Init State
	//

	if (!editorState->isInitialized) {
		//
		//  Init Editor
		//

		editorState->arena = ArenaAlloc(Megabytes(64), Gigabytes(64));
		editorState->frameArena = ArenaAlloc(Megabytes(64), Gigabytes(64));

		editorState->currentTextTabID = -1;
		editorState->tabs = Array<text_tab>(&editorState->arena, 128);

		//
		// Init Fonts & Theme
		//

		editorState->fontSize = DEFAULT_FONT_SIZE;
		g_fontRegular = ImGui::GetIO().Fonts->AddFontFromFileTTF(g_fontRegularPath);

		SetDarkTheme();

		//
		// Init Language
		//

		SetLanguage(Lang_ENG);

		editorState->isInitialized = true;
	}

	memory_arena* frameArena = &editorState->frameArena;

	//
	// Process Event Queue
	//

	text_tab* textTab = GetCurrentTab(editorState);

	if (textTab) {
		u8* i = (u8*)eventQueue->base;

		while (i < (u8*)eventQueue->base + eventQueue->size) {
			event_type eventType = *(event_type*)i;

			switch (eventType) {
				case Event_Char: {
					char_event* event = (char_event*)i;
					TextInsertChar(textTab, event->utf8CodePoint, textTab->cursorIndex);
					textTab->cursorIndex++;
					i += sizeof(*event);
				} break;

				case Event_Key: {
					key_event* event = (key_event*)i;
					if (event->key == Key_ArrowRight && event->isDown) {
						textTab->cursorIndex++;
					} else if (event->key == Key_ArrowLeft && event->isDown) {
						if (textTab->cursorIndex > 0)
							textTab->cursorIndex--;
					} else if (event->key == Key_Enter && event->isDown) {
						code_point cp = CodePoint('\n');
						TextInsertChar(textTab, cp, textTab->cursorIndex);
					}

					i += sizeof(*event);
				} break;

				default:
					platform_Print("Unknown event\n");
					break;
			}
		}
	}

	//
	// Commands / Hotkeys
	//

	bool ctrlDown = IsButtonPushed(input->keys[Key_Ctrl]);
	bool shiftDown = IsButtonPushed(input->keys[Key_Shift]);
	bool altDown = IsButtonPushed(input->keys[Key_Alt]);

	for (size_t i = 1; i < ArrayCount(g_hotkeyMappings); i++) {
		command_type commandType = (command_type)i;
		command* c = &g_hotkeyMappings[i];

		if (c->ctrl == ctrlDown && c->shift == shiftDown && c->alt == altDown && IsButtonDown(input->keys[c->key])) {
			ExecuteCommand(commandType, editorState);
		}
	}
}

//
// Draw UI
//

void EditorRender(program_input* input) {
	editor_state* editorState = &g_editorState;
	if (!editorState->isInitialized)
		return;

	//
	// Frame Setup
	//

	memory_arena* frameArena = &editorState->frameArena;

	platform_StartFrame();

	// limit font size
	editorState->fontSize = te_Min(te_Max(editorState->fontSize, MIN_FONT_SIZE), MAX_FONT_SIZE);
	ImGui::PushFont(g_fontRegular, editorState->fontSize);

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();

	//
	// Toolbar
	//

	float toolbarHeight = ImGui::GetTextLineHeight() +
												style.FramePadding.y * 2.0f +
												style.WindowPadding.y * 2.0f;
	{
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, toolbarHeight));	// Высота панели инструментов
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags toolbarFlags =
				ImGuiWindowFlags_NoDocking |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoSavedSettings;

		if (ImGui::Begin("Toolbar", nullptr, toolbarFlags)) {
			if (ImGui::Button(GetStrings().settings)) {
				ExecuteCommand(Command_OpenSettings, editorState);
			}
			ImGui::SameLine();

			if (ImGui::Button(GetStrings().newFile)) {
				ExecuteCommand(Command_New, editorState);
			}
			ImGui::SameLine();

			if (ImGui::Button(GetStrings().loadFile)) {
				// if (g_editor.textTabs.size > 0) {
				//     g_editor.getCurrentTextTab().openFile(); // TODO: проверка, найден ли currentTextTab?
				// }
				// else {
				//     g_editor.AddTextTab();
				//     g_editor.textTabs[0].openFile();
				// }
			}
			ImGui::SameLine();

			if (ImGui::Button(GetStrings().saveFile)) {
				ExecuteCommand(Command_Save, editorState);
				// if (g_editor.textTabs.size > 0) {
				// g_editor.getCurrentTextTab().saveFile(); // TODO: проверка, найден ли currentTextTab?
				// }
			}
		}
		ImGui::End();
	}

	//
	// Dock panel
	//

	{
		ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + toolbarHeight));
		ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - toolbarHeight));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags dockspaceFlags =
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", nullptr, dockspaceFlags);
		ImGui::PopStyleVar();

		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
		if (g_lastDockNodeId == 0)
			g_lastDockNodeId = dockspace_id;

		ImGui::End();
	}

	//
	// Text tabs
	//

	// cleanup
	for (size_t i = 0; i < editorState->tabs.count; i++) {
		text_tab* tab = &editorState->tabs[i];

		if (!tab->isOpen) {
			CloseTextTab(editorState, i);
		}
	}

	for (size_t i = 0; i < editorState->tabs.count; i++) {
		text_tab* tab = &editorState->tabs[i];

		string_builder builder = {0};
		StrAppend(frameArena, &builder, "New ");
		IntAppend(frameArena, &builder, tab->id);
		char label[256];
		ToCString(label, 256, &builder);

		if (ImGui::Begin(label, &tab->isOpen, ImGuiWindowFlags_NoSavedSettings)) {
			if (ImGui::IsWindowFocused()) {
				editorState->currentTextTabID = tab->id;
			}
			// записываем id, по которому можно будет присоеденить окно
			if (ImGui::IsWindowDocked()) {
				if (ImGui::IsWindowFocused()) {
					g_lastDockNodeId = ImGui::GetWindowDockID();
				}
			}
			// проверяем есть ли окно, к которому можно присоединить новое
			else if (g_lastDockNodeId != 0) {
				// проверяем перетаскивается ли окно. если нет - автоматически присоединяем к существующему окну
				if (!(ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))) {
					ImGui::DockBuilderDockWindow(label, g_lastDockNodeId);
				}
			}

			string filenameStub = String("filename_stub.txt");
			ImGui::Text(filenameStub);

			// TODO: билдится каждый кадр, улучшить
			s64 length = TextGetLength(tab);
			char* buf = (char*)ArenaPushArray(frameArena, length, char);
			s64 bytesWritten = TextBuild(tab, buf);
			string text = String(buf, bytesWritten);

			if (ImGui::BeginChild("TextChild", ImVec2(0, 0), 1)) {
				ImGui::Text(text);

				// drawCursor(text.cursorIndex2, buffer.mem, IM_COL32(255, 255, 255, 180));
				ImGui::DrawCursor(tab->cursorIndex, text);
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	//
	// Settings
	//

	if (g_isSettingsOpen) {
		if (ImGui::Begin(GetStrings().settings, &g_isSettingsOpen)) {
			ImGui::SliderInt(GetStrings().fontSize, (int*)&editorState->fontSize, MIN_FONT_SIZE, MAX_FONT_SIZE);

			static s32 currentItem = 0;
			if (ImGui::Combo(GetStrings().language, &currentItem, g_languageStrings, ArrayCount(g_languageStrings))) {
				SetLanguage((localization_language)currentItem);
			}
		}
		ImGui::End();
	}

	//
	// Command Palette
	//

	if (g_isCommandPaletteOpen) {
		// закрыть
		if (IsButtonDown(input->keys[Key_Esc])) {
			g_isCommandPaletteOpen = false;
		}

		u32 buttonWidth = editorState->fontSize * 10;

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImVec2 size = ImGui::GetMainViewport()->Size;
		ImGui::SetNextWindowPos(center, 0, ImVec2(0.5, 0.5));
		ImGui::SetNextWindowSize(ImVec2(
				te_Max(size.x * 0.4, buttonWidth * 3),
				size.y * 0.7));

		if (ImGui::Begin("Commands", &g_isCommandPaletteOpen)) {
			static char searchBuf[1024];
			bool executeFirst = false;

			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputText("##CommandsSearch", searchBuf, sizeof(searchBuf), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
				executeFirst = true;	// если нажат Enter, выполняем первую команду из списка
			}

			// фокус на поле ввода при открытии окна
			if (g_commandPaletteFocusInput) {
				ImGui::SetKeyboardFocusHere(-1);
				g_commandPaletteFocusInput = false;
			}

			for (size_t i = 1; i < ArrayCount(g_hotkeyMappings); i++) {
				command* c = &g_hotkeyMappings[i];

				// фильтрация
				// TODO: fuzzy search
				if (StrLen(searchBuf) && !StrFind(c->label, searchBuf, StrFind_ToLower)) {
					continue;
				}

				if (executeFirst) {
					ExecuteCommand((command_type)i, editorState);
					g_isCommandPaletteOpen = false;
					break;
				}

				string_builder sb = {0};

				// hotkey string
				{
					s32 count = 0;
					if (c->ctrl) {
						StrAppend(frameArena, &sb, GetKeyString(Key_Ctrl));
						count++;
					}
					if (c->shift) {
						if (count > 0)
							StrAppend(frameArena, &sb, " + ");
						StrAppend(frameArena, &sb, GetKeyString(Key_Shift));
						count++;
					}
					if (c->alt) {
						if (count > 0)
							StrAppend(frameArena, &sb, " + ");
						StrAppend(frameArena, &sb, GetKeyString(Key_Alt));
						count++;
					}
					if (c->key) {
						if (count > 0)
							StrAppend(frameArena, &sb, " + ");
						StrAppend(frameArena, &sb, GetKeyString(c->key));
					}
				}

				ImGui::PushID(i);
				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0.5));
				if (ImGui::Button(c->label, ImVec2(buttonWidth, 0))) {
					ExecuteCommand((command_type)i, editorState);
					g_isCommandPaletteOpen = false;
				}
				ImGui::PopStyleVar();
				ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - buttonWidth);
				// ImGui::Text(sb.buffer);
				ImGui::Button(sb.buffer, ImVec2(buttonWidth, 0));
				ImGui::PopID();
			}
		}
		ImGui::End();
	}

	// test
	if (ImGui::Begin("Debug")) {
		f32 divider = Kilobytes(1);
		ImGui::Arena(&editorState->arena, "Main Arena");
		ImGui::Arena(&editorState->frameArena, "Frame Arena");
	}
	ImGui::End();

	//
	// Frame Cleanup
	//

	ImGui::PopFont();
	platform_EndFrame();

	ArenaClear(frameArena);
}