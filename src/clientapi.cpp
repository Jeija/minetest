#include "clientapi.h"

#include <iostream>
#include <list>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "client.h"
#include "tool.h"
#include "util/string.h"
#include "log.h"

void clientapi_step(lua_State *L, float dtime)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_environment_step"<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.registered_globalsteps
	lua_getglobal(L, "client");
	lua_getfield(L, -1, "on_step");
	luaL_checktype(L, -1, LUA_TFUNCTION);
	// Call on_step
	lua_pushnumber(L, dtime);
	lua_pcall(L, 1, 0, 0);
	printf("called\n");
}

static int l_debug(lua_State *L)
{
	std::string text = lua_tostring(L, 1);
	dstream << text << std::endl;
	return 0;
} 

static const struct luaL_Reg client_f [] = {
	{"debug", l_debug},
	{NULL, NULL}
};

bool clientapi_loadmod(lua_State *L, const std::string &scriptpath,
		const std::string &modname)
{
	ModNameStorer modnamestorer(L, modname);

	if(!string_allowed(modname, "abcdefghijklmnopqrstuvwxyz"
			"0123456789_")){
		errorstream<<"Error loading client mod \""<<modname
				<<"\": modname does not follow naming conventions: "
				<<"Only chararacters [a-z0-9_] are allowed."<<std::endl;
		return false;
	}
	
	bool success = false;

	try{
		success = script_load(L, scriptpath.c_str());
	}
	catch(LuaError &e){
		errorstream<<"Error loading mod \""<<modname
				<<"\": "<<e.what()<<std::endl;
	}

	return success;
}

void clientapi_export(lua_State *L, Client *client)
{
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Store server as light userdata in registry
	// lua_pushlightuserdata(L, client);
	// lua_setfield(L, LUA_REGISTRYINDEX, "minetest_server");

	// Register global functions in table minetest
	lua_newtable(L);
	luaL_register(L, NULL, client_f);
	lua_setglobal(L, "client");
	
	// Get the main minetest table
	lua_getglobal(L, "client");

	// Add tables to minetest
	// lua_newtable(L);
	// lua_setfield(L, -2, "object_refs");
	// lua_newtable(L);
	// lua_setfield(L, -2, "luaentities");

	// Register wrappers
	// LuaItemStack::Register(L);
	// InvRef::Register(L);
	// NodeMetaRef::Register(L);
	// NodeTimerRef::Register(L);
	// ObjectRef::Register(L);
	// EnvRef::Register(L);
	// LuaPseudoRandom::Register(L);
	// LuaPerlinNoise::Register(L);
}
