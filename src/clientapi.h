class Client;
typedef struct lua_State lua_State;
#include <string>

void clientapi_step(lua_State *L, float dtime);
void clientapi_export(lua_State *L, Client *client);
bool clientapi_loadmod(lua_State *L, const std::string &scriptpath, const std::string &modname);
