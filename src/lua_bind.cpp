#include "lua_bind.h"

#include <windows.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "log.h"
#include "patch_engine.h"
#include "pe_helpers.h"

namespace mp::lua_bind {

namespace {

// Pull an optional string field from a Lua table at index `tableIdx`.
std::string LuaTableString(lua_State* L, int tableIdx, const char* key,
                           const char* fallback = "") {
    lua_getfield(L, tableIdx, key);
    std::string out = fallback;
    if (lua_isstring(L, -1)) out = lua_tostring(L, -1);
    lua_pop(L, 1);
    return out;
}

int LuaTableInt(lua_State* L, int tableIdx, const char* key, int fallback) {
    lua_getfield(L, tableIdx, key);
    int out = fallback;
    if (lua_isnumber(L, -1)) out = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return out;
}

bool LuaTableBool(lua_State* L, int tableIdx, const char* key, bool fallback) {
    lua_getfield(L, tableIdx, key);
    bool out = fallback;
    if (lua_isboolean(L, -1)) out = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return out;
}

int Lua_ScanAndWrite(lua_State* L) {
    if (!lua_istable(L, 1)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "MemPatch.ScanAndWrite: expected a table argument");
        return 2;
    }

    mp::patch::PatchEntry e;
    e.sourceFile = "<lua>";
    e.name = LuaTableString(L, 1, "name", "lua_runtime");
    e.description = LuaTableString(L, 1, "description");
    e.priority = LuaTableInt(L, 1, "priority", 100);
    e.module = LuaTableString(L, 1, "module", "WHGame.dll");
    e.offset = LuaTableInt(L, 1, "offset", 0);
    e.idempotent = LuaTableBool(L, 1, "idempotent", true);

    std::string patternStr = LuaTableString(L, 1, "pattern");
    std::string originalStr = LuaTableString(L, 1, "original");
    std::string replacementStr = LuaTableString(L, 1, "replacement");
    std::string contextStr = LuaTableString(L, 1, "context");
    std::string anchorString = LuaTableString(L, 1, "anchor_string");

    try {
        if (patternStr.empty() || originalStr.empty() || replacementStr.empty()) {
            throw std::runtime_error("missing required field pattern / original / replacement");
        }
        e.pattern = mp::patch::ParsePattern(patternStr);
        e.original = mp::patch::ParseBytes(originalStr);
        e.replacement = mp::patch::ParseBytes(replacementStr);
        if (!contextStr.empty()) e.context = mp::patch::ParsePattern(contextStr);
        if (!anchorString.empty()) {
            e.anchor = mp::patch::AnchorString{anchorString};
        }
        e.maxAnchorDistance = static_cast<uint32_t>(
            LuaTableInt(L, 1, "max_anchor_distance", 4096));
    } catch (const std::exception& ex) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, "MemPatch.ScanAndWrite: %s", ex.what());
        return 2;
    }

    bool ok = mp::patch::ApplyPatch(e);
    lua_pushboolean(L, ok ? 1 : 0);
    lua_pushstring(L, ok ? "ok" : "see mempatch.log");
    return 2;
}

int Lua_ReadBytes(lua_State* L) {
    lua_Integer addr = luaL_checkinteger(L, 1);
    lua_Integer n = luaL_checkinteger(L, 2);
    if (addr <= 0 || n <= 0 || n > 4096) {
        lua_pushnil(L);
        lua_pushstring(L, "MemPatch.ReadBytes: invalid address or length");
        return 2;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == 0 ||
        mbi.State != MEM_COMMIT ||
        (mbi.Protect & PAGE_NOACCESS) ||
        (mbi.Protect & PAGE_GUARD)) {
        lua_pushnil(L);
        lua_pushstring(L, "MemPatch.ReadBytes: address not readable");
        return 2;
    }

    auto* bytes = reinterpret_cast<const uint8_t*>(addr);
    std::string out;
    out.reserve(static_cast<size_t>(n) * 3);
    for (lua_Integer i = 0; i < n; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), i ? " %02X" : "%02X", bytes[i]);
        out += buf;
    }
    lua_pushlstring(L, out.data(), out.size());
    return 1;
}

int Lua_GetWHGameBase(lua_State* L) {
    mp::pe::ModuleView mv;
    if (!mp::pe::OpenModule(L"WHGame.dll", mv)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushinteger(L, static_cast<lua_Integer>(
        reinterpret_cast<uintptr_t>(mv.baseBytes)));
    return 1;
}

}  // namespace

void RegisterMemPatchTable(lua_State* L) {
    if (!L) {
        log::Warn("RegisterMemPatchTable called with null L");
        return;
    }
    lua_newtable(L);
    lua_pushcfunction(L, Lua_ScanAndWrite);
    lua_setfield(L, -2, "ScanAndWrite");
    lua_pushcfunction(L, Lua_ReadBytes);
    lua_setfield(L, -2, "ReadBytes");
    lua_pushcfunction(L, Lua_GetWHGameBase);
    lua_setfield(L, -2, "GetWHGameBase");
    lua_setglobal(L, "MemPatch");
    log::Info("MemPatch Lua API registered (ScanAndWrite, ReadBytes, GetWHGameBase)");
}

}  // namespace mp::lua_bind
