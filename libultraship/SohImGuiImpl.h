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

namespace SohImGui {
	void init(WindowImpl window_impl);
	void update(EventImpl event);
	void draw(void);
	void BindCmd(const std::string cmd, CommandFunc func);
}
#endif