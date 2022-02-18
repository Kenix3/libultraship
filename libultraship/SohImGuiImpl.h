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

struct SoHSettings {
	bool soh = false;
	bool console = false;
	bool n64mode = false;
	bool menu_bar = false;
	bool soh_sink = true;
};

extern SoHSettings soh_settings;

namespace SohImGui {
	extern Console* console;
	void init(WindowImpl window_impl);
	void update(EventImpl event);
	void draw(void);
	void BindCmd(const std::string& cmd, CommandEntry entry);
}
#endif