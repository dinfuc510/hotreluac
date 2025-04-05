#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "raylib.h"

#define MAX_ITER 64
#define BUF_SIZE 1024

static char *read_file(const char *filepath) {
	FILE *f = fopen(filepath, "rb");
	if (f == NULL) {
		return NULL;
	}

	size_t count = 0, capacity = BUF_SIZE;
	char *data = (char*) malloc(capacity * sizeof(char));
	char buf[BUF_SIZE];
	size_t n = 0;
	while ((n = fread(buf, 1, BUF_SIZE, f)) > 0) {
		size_t old_count = count;
		count += n;
		if (count > capacity) {
			int iter = 0;
			while (count > capacity && iter++ < MAX_ITER) {
				capacity *= 2;
			}
			if (count > capacity) {
				capacity = count;
			}
			data = realloc(data, capacity * sizeof(char));
		}
		memmove(data + old_count, buf, n);
	}
	data[count - 1] = 0;

	fclose(f);
	return data;
}

static int draw_rect(lua_State *L) {
    lua_Number color = lua_tonumber(L, -1);
    lua_Number h = lua_tonumber(L, -2);
    lua_Number w = lua_tonumber(L, -3);
    lua_Number y = lua_tonumber(L, -4);
    lua_Number x = lua_tonumber(L, -5);

	DrawRectangleV((Vector2) { x, y }, (Vector2) { w, h }, GetColor(color));
	return 1;
}

static int draw_circle(lua_State *L) {
    lua_Number color = lua_tonumber(L, -1);
    lua_Number r = lua_tonumber(L, -2);
    lua_Number y = lua_tonumber(L, -3);
    lua_Number x = lua_tonumber(L, -4);

	DrawCircleV((Vector2) { x, y }, r, GetColor(color));
	return 1;
}

static void print_table(lua_State *L, int level) {
	if (!lua_istable(L, -1)) {
		printf("[C] expect table, got %s\n", lua_typename(L, lua_type(L, -1)));
		return;
	}

	lua_pushnil(L); // First key is nil
	while (lua_next(L, -2) != LUA_TNIL) {
		if (lua_type(L, -2) == LUA_TSTRING) {
			const char *key = lua_tostring(L, -2);
			printf("%*s[%s]: ", level*4, "", key);
		}
		else if (lua_type(L, -2) == LUA_TNUMBER) {
			printf("%*s[%f]: ", level*4, "", lua_tonumber(L, -2));
		}
		else {
			printf("Unreachable\n");
			return;
		}

		switch(lua_type(L, -1)) {
			case LUA_TNUMBER: {
				printf("%g\n", lua_tonumber(L, -1));
				break;
			}
			case LUA_TSTRING: {
				printf("\"%s\"\n", lua_tostring(L, -1));
				break;
			}
			case LUA_TBOOLEAN: {
				printf(lua_toboolean(L, -1) ? "true" : "false");
				break;
			}
			case LUA_TTABLE: {
				printf("{\n");
				print_table(L, level + 1);
				printf("%*s}\n", level*4, "");
				break;
			}
			default: {
				printf("%s\n", lua_typename(L, lua_type(L, -1)));
			}
		}
		lua_pop(L, 1);
	}
}

static void print_stack(lua_State *L) {
	printf("Stack: ");
	int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) {  /* repeat for each level */
		int t = lua_type(L, i);
		switch (t) {
			case LUA_TSTRING:
				printf("`%s'", lua_tostring(L, i));
				break;
			case LUA_TBOOLEAN:
				printf(lua_toboolean(L, i) ? "true" : "false");
				break;
			case LUA_TNUMBER:
				printf("%g", lua_tonumber(L, i));
				break;
			default:
				printf("%s", lua_typename(L, t));
				break;
		}
		printf("  ");
	}
	printf("\n");
}

static void err(lua_State *L) {
	fprintf(stderr, "%s\n", lua_tostring(L, -1));
	lua_pop(L, 1);
}

static bool pcall0n(lua_State *L, const char *func, int nresults) {
	if (lua_getglobal(L, func) == LUA_TNIL) {
		fprintf(stderr, "[Lua] not found function \"%s\"\n", func);
		lua_pop(L, 1);
		return false;
	}

	int errcode = lua_pcall(L, 0, nresults, 0);
	if (errcode != 0) {
		if (errcode == LUA_ERRRUN) {
			err(L);
		}
		else {
			fprintf(stderr, "[C] could not execute function \"%s\": %d\n", func, errcode);
			lua_pop(L, 1);
		}
	}

	return errcode == 0;
}

int main(void) {
	static const char *filename = "hrl.lua";
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	lua_pushcfunction(L, draw_rect);
	lua_setglobal(L, "draw_rect");
	lua_pushcfunction(L, draw_circle);
	lua_setglobal(L, "draw_circle");
	char *src = read_file(filename);
	if (luaL_dostring(L, src) != LUA_OK) {
		err(L);
		lua_close(L);
		return 1;
	}

	InitWindow(700, 500, "Lua in Raylib");
	SetTargetFPS(60);

	pcall0n(L, "init", 0);

	bool is_executable = false;
	while(!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);

		// if (IsKeyDown(KEY_R)) {
		if (IsKeyPressed(KEY_R)) {
			int r = 0;
			if (pcall0n(L, "getcontext", 1)) {
				r = luaL_ref(L, LUA_REGISTRYINDEX);
			}

			if (src) free(src);
			src = read_file(filename);
			if (luaL_dostring(L, src) != LUA_OK) {
				err(L);
			}

			is_executable = false;
			if (src == NULL) {
				printf("[C] could not load file\n");
			}
			else if (luaL_dostring(L, src) != LUA_OK)  {
				printf("[Lua]: %s\n", lua_tostring(L, -1));
			}
			else {
				lua_rawgeti(L, LUA_REGISTRYINDEX, r);
				print_table(L, 0);
				print_stack(L);

				lua_getglobal(L, "setcontext");
				lua_rawgeti(L, LUA_REGISTRYINDEX, r);
				if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
					printf("[Lua] could not execute setcontext: %s\n", lua_tostring(L, -1));
					lua_pop(L, 1);
				}
				lua_pop(L, 1);

				luaL_unref(L, LUA_REGISTRYINDEX, r);
				is_executable = true;
			}
		}

		if (is_executable) {
			if (!pcall0n(L, "draw", 0)) {
				is_executable = false;
			}
		}
		EndDrawing();
	}
	pcall0n(L, "cleanup", 0);

	if (src) free(src);
	CloseWindow();
	lua_close(L);

	return 0;
}
