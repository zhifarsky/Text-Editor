#pragma once
#include "editor.h"

//
// Text Tab
//

enum text_node_type : u8 {
	Node_Original,
	Node_Added
};

struct text_node {
	s64 start, length;
	text_node_type type;
};

enum encoding_type {
	Encoding_None = 0,
	Encoding_UTF8
};

// TODO:
// undo/redo stacks

struct text_tab {
	memory_arena arena;

	array_dynamic<char> added;
	array_dynamic<text_node> nodes;
	char* original;

	s64 cursorIndex;
	u32 id;
	encoding_type encoding;

	bool isOpen;
};

text_tab TextTab(s32 id) {
	text_tab newTab = {0};

	newTab.id = id;
	newTab.encoding = Encoding_UTF8;
	newTab.isOpen = true;

	newTab.arena = ArenaAlloc(Megabytes(1));

	newTab.added = Array<char>(&newTab.arena);
	newTab.nodes = Array<text_node>(&newTab.arena);

	return newTab;
}

void Release(text_tab* tab) {
	ArenaRelease(&tab->arena);
}

// TODO: заглушки, реализовать

u64 GetTextLength_utf8(text_tab* textTab) {
	return 0;
}

u64 GetText_utf8(text_tab* textTab, char* buffer) {
	return 0;
}

void TextInsertNode(text_tab* textTab, const text_node& newNode, s64 index) {
	if (newNode.length <= 0)
		return;

	array_dynamic<text_node>& nodes = textTab->nodes;

	// вставка в начало
	if (index == 0) {
		Insert(&nodes, &textTab->arena, newNode, 0);
		return;
	}

	s64 textLen = 0;
	for (s64 i = 0; i < textTab->nodes.count; i++) {
		text_node* curNode = &nodes[i];
		s64 start = curNode->start;
		s64 end = curNode->start + curNode->length;
		textLen += curNode->length;

		// вставка на дальней границе узла. добавляем узел после текущего
		if (textLen == index) {
			if (
					curNode->type == Node_Added &&
					curNode->start + curNode->length == newNode.start) {
				curNode->length += newNode.length;
			} else {
				Insert(&nodes, &textTab->arena, newNode, i + 1);
			}
			break;
		}

		// вставка не на границе узла
		if (textLen > index) {
			text_node node1;
			node1.type = curNode->type;
			node1.start = curNode->start;
			node1.length = curNode->length - (textLen - index);

			text_node node2;
			node2.type = curNode->type;
			node2.start = node1.start + node1.length;
			node2.length = textLen - index;

			te_assert(node1.length > 0 && node2.length > 0);

			*curNode = node1;
			Insert(&nodes, &textTab->arena, node2, i + 1);
			Insert(&nodes, &textTab->arena, newNode, i + 1);
			break;
		}
	}
}

void TextInsertString(text_tab* tab, string str, s64 index) {
	text_node newNode = {.start = tab->added.count, .length = str.size, .type = Node_Added};
	te_assert(newNode.length > 0);

	// TODO: добавить в Array append сразу нескольких элементов
	for (s64 i = 0; i < str.size; i++) {
		Push(&tab->added, &tab->arena, str.base[i]);
	}

	TextInsertNode(tab, newNode, index);
}

void TextInsertChar(text_tab* textTab, code_point utf8CodePoint, s64 pos) {
	s32 length = GetLength(utf8CodePoint);
	decltype(code_point::bytes) bytes = {0};
	for (s32 i = length - 1, j = 0; i >= 0; i--, j++) {
		bytes[j] = utf8CodePoint.bytes[i];
	}

	string str = String((char*)bytes, length);
	TextInsertString(textTab, str, pos);
}

void TextInsertTest(text_tab* textTab) {
	code_point c = {0};
	const unsigned char s1[] = "汉";
	MemCopy((void*)c.bytes, (void*)s1, StrLen(s1));
	TextInsertChar(textTab, c, 0);
}

s64 TextGetLength(text_tab* tab) {
	s64 len = 0;
	for (s64 i = 0; i < tab->nodes.count; i++)
		len += tab->nodes[i].length;
	return len;
}

// возвращает кол-во записанных символов
// не выставляет null-terminator
s64 TextBuild(text_tab* tab, char* buf) {
	array_dynamic<text_node>& nodes = tab->nodes;

	char* bufSlider = buf;
	for (size_t i = 0; i < nodes.count; i++) {
		text_node* node = &nodes[i];

		char* source = 0;
		if (node->type == Node_Original)
			source = tab->original;
		else if (node->type == Node_Added)
			source = tab->added.items;

		MemCopy(bufSlider, source + node->start, node->length);
		bufSlider += node->length;
	}

	return bufSlider - buf;	 // кол-во записанных элементов
}