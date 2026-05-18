#pragma once

extern "C" {
struct lua_State;
}

namespace mp::lua_bind {

// Registers a global `MemPatch` table with three functions:
//   MemPatch.ScanAndWrite(spec)  -- spec is a table mirroring the TOML schema.
//                                    Runs the patch through patch_engine::ApplyPatch
//                                    so all safety checks apply. Returns (ok, msg).
//   MemPatch.ReadBytes(addr, n)  -- read n bytes at absolute address `addr`,
//                                    return them as a string of hex pairs separated
//                                    by spaces. Validates the address is readable.
//   MemPatch.GetWHGameBase()     -- returns WHGame.dll base address as a Lua number.
//
// Safe to call once after the lua_State* is captured.
void RegisterMemPatchTable(lua_State* L);

}  // namespace mp::lua_bind
