#pragma once

typedef struct {
	void* window;
	void* context;
	void* device;
} WindowImpl;

typedef struct {
	void* handle;
	int msg;
	int wparam;
	int lparam;
} EventImpl;

#ifndef __cplusplus
void c_init(WindowImpl window);
void c_update(EventImpl event);
void c_draw(void);
#else
#include "SohConsole.h"

struct SoHConfigType {
	bool soh = false;
	bool n64mode = false;
	bool menu_bar = false;
	bool soh_sink = true;
};

extern SoHConfigType SohSettings;

namespace SohImGui {
	extern Console* console;
	void Init(WindowImpl window_impl);
	void Update(EventImpl event);
	void Draw(void);
	std::string GetDebugSection();
	void BindCmd(const std::string& cmd, CommandEntry entry);
}
#endif