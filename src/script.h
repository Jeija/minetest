/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef SCRIPT_HEADER
#define SCRIPT_HEADER

#include <exception>
#include <string>

typedef struct lua_State lua_State;

class LuaError : public std::exception
{
public:
	LuaError(lua_State *L, const std::string &s);

	virtual ~LuaError() throw()
	{}
	virtual const char * what() const throw()
	{
		return m_s.c_str();
	}
	std::string m_s;
};

class StackUnroller
{
private:
	lua_State *m_lua;
	int m_original_top;
public:
	StackUnroller(lua_State *L);
	~StackUnroller();
};

class ModNameStorer
{
private:
	lua_State *L;
public:
	ModNameStorer(lua_State *L_, const std::string modname);
	~ModNameStorer();
};

lua_State* script_init();
void script_deinit(lua_State *L);
std::string script_get_backtrace(lua_State *L);
void script_error(lua_State *L, const char *fmt, ...);
bool script_load(lua_State *L, const char *path);
void stackDump(lua_State *L, std::ostream &o);
void realitycheck(lua_State *L);

// What scriptapi_run_callbacks does with the return values of callbacks.
// Regardless of the mode, if only one callback is defined,
// its return value is the total return value.
// Modes only affect the case where 0 or >= 2 callbacks are defined.

#endif

