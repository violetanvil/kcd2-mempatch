# kcd2-mempatch

> **Are you a player who just wants to install a mod?** This GitHub repo
> is the engine source. To install kcd2-mempatch as a user, grab it from
> [Nexus](https://www.nexusmods.com/kingdomcomedeliverance2/) (preferred)
> or the [Releases page](https://github.com/violetanvil/kcd2-mempatch/releases).
> The rest of this README is for modders writing plugins.

---

A surgical memory-patch engine for Kingdom Come: Deliverance II mods.
Plugin authors declare patches as TOML files; the engine applies them at
game startup with tiered locator-agreement safety checks. Forked from
[yobson1/kcd2lua](https://github.com/yobson1/kcd2lua) (MIT) with the
addressing model rewritten and the VS Code remoting stripped out.

## What it does

- Loaded by Ultimate-ASI-Loader's `dinput8.dll` proxy at game startup
- Reads `plugins/*/mempatch.toml` from its own folder
- For each `[[patch]]` block: resolves declared locators (pattern,
  optional context, optional anchor), verifies all agree on a single
  address, reads the bytes there, compares against the declared
  `original`, and only then writes `replacement` via `VirtualProtect`
- **Pre-flight conflict detection**: before applying anything, the
  engine resolves every plugin's locators against the pristine DLL and
  categorizes any byte overlap between plugins as one of three kinds:
  *incidental* (one plugin's read range happens to span another's
  write site — silent, both apply via cached pre-flight resolution),
  *write-on-original* (a plugin's verify target is modified by an
  earlier plugin — `[WARN]`, the later plugin aborts cleanly with the
  conflict context attached to the abort line), or *write-on-write*
  (both target the same address — `[INFO]` for full overlap,
  `[WARN]` for partial; both writes happen in priority order). All
  log lines are plain-English so a player reporting a bug can see
  *which* mod interacts with *which* other mod. See
  [`docs/writing-safe-patches.md`](docs/writing-safe-patches.md) for
  the full conflict-semantics reference.
- Logs every step (match counts, agreement check results, write outcome)
  to `mempatch.log`
- Also registers a `MemPatch` Lua table on the game's `lua_State*` so pak
  Lua mods can call `MemPatch.ScanAndWrite{...}` at runtime (same safety
  pipeline; runtime Lua patches don't participate in pre-flight)

## Writing a plugin

A plugin is a folder containing a single `mempatch.toml`. The minimum:

```toml
[[patch]]
name        = "your_patch_name"
pattern     = "AA BB CC ... 16+ unique bytes"  # required, must produce exactly 1 match
offset      = 13                                # bytes from pattern start to first byte written
original    = "DD EE FF"                        # what's currently at the patch site
replacement = "11 22 33"                        # what to write; same length as original
```

The engine refuses to write unless `pattern` matches exactly once in
`WHGame.dll`'s executable sections AND the bytes at the patch site equal
`original`. Add a `context` field (longer surrounding pattern) for a
second uniqueness check, and an `anchor_string` (unique `.rdata` literal)
for function-bounds verification via `.pdata` lookup.

**Read next:**

- [docs/finding-patch-sites.md](docs/finding-patch-sites.md) — **start
  here if you don't yet know what bytes to patch**. The general
  methodology for finding a patch site in `WHGame.dll`: Ghidra setup,
  choosing the right anchor string, reading the decompile, identifying
  the gate, finding the corresponding assembly, verifying in x64dbg.
  Outfit-swap-in-combat runs through it as a concrete worked example.
  Includes a Shortcuts section pointing at the KCD2 reverse-engineering
  references that have already mapped CryEngine infrastructure (gEnv,
  Lua, console) so you don't redo that work.
- [examples/](examples/) — two reference TOMLs:
  - [`outfit-swap-in-combat/`](examples/outfit-swap-in-combat/) — a real,
    working patch as a complete example
  - [`template-fully-commented/`](examples/template-fully-commented/) —
    schema reference with every field annotated, using safe non-matching
    bytes
- [docs/config-schema.md](docs/config-schema.md) — formal TOML schema
  reference
- [docs/writing-safe-patches.md](docs/writing-safe-patches.md) —
  methodology: how to pick a unique pattern, when to add a context, when
  to use an anchor

## Lua API

When the game's first `lua_pcall` fires, the engine captures the live
`lua_State*` and registers a global `MemPatch` table:

```lua
-- Apply a patch at runtime from pak Lua. Same safety pipeline as TOML patches.
local ok, msg = MemPatch.ScanAndWrite{
    name        = "my_patch",
    pattern     = "48 8B 01 FF 50 08 ...",
    offset      = 13,
    original    = "44 8A F0",
    replacement = "45 31 F6",
    context     = "...",            -- optional Tier-2
    anchor_string = "...",          -- optional Tier-3
    idempotent  = true,
}

-- Read raw bytes at an absolute address (≤4096 bytes). Returns hex string.
local hex = MemPatch.ReadBytes(0x7FF000000000, 16)

-- WHGame.dll base address as an integer.
local base = MemPatch.GetWHGameBase()
```

The same safety guarantees apply: `MemPatch.ScanAndWrite` rejects
non-unique patterns and mismatched originals just like the TOML path.

## Installing the engine

Same install as a player does; you'll just be the one updating the
plugin folder repeatedly as you iterate.

1. Grab the latest `kcd2-mempatch-*.zip` from the
   [Releases page](https://github.com/violetanvil/kcd2-mempatch/releases).
2. Extract its contents into your game's `Bin\Win64MasterMasterSteamPGO\`
   folder. The zip already has the right layout: `dinput8.dll` lands
   next to `KingdomCome.exe`, and `plugins/mempatch.asi` lands one level
   deeper. (For typical Steam installs that folder is
   `<SteamLibrary>/steamapps/common/KingdomComeDeliverance2/Bin/Win64MasterMasterSteamPGO/`;
   for other distribution channels the only thing that matters is that
   `dinput8.dll` ends up next to `KingdomCome.exe`.)
3. Drop your plugin folder(s) into `plugins/`, alongside `mempatch.asi`.
   Each plugin is a folder containing one `mempatch.toml`.

Final runtime layout:

```
<game>/Bin/Win64MasterMasterSteamPGO/
├── KingdomCome.exe                   (vanilla)
├── dinput8.dll                       (Ultimate-ASI-Loader, ships in the zip)
└── plugins/
    ├── mempatch.asi                  (the engine, ships in the zip)
    ├── mempatch.log                  (created at runtime)
    └── <your-plugin>/
        └── mempatch.toml             (you write this)
```

Launch the game with `-console` to see live logging in a separate window
alongside the file log. The log is the fastest diagnostic loop while
iterating on a patch — match counts, locator agreement, write outcome
all appear immediately.

## Compatibility

- The `lua_pcall` and `update` AOB signatures are inherited from
  yobson1/kcd2lua's Oct-2025 update (game version 1.3 era), and were
  verified to still match on `release_1_5_1164953_841` (April 2026). If
  a future game update breaks them, the abort message says so explicitly
  and the game launches normally.
- Plays cleanly alongside any pak-based mod, Workshop or otherwise.

## Credits

- [yobson1/kcd2lua](https://github.com/yobson1/kcd2lua) — ASI bootstrap
  scaffold (MIT). Original code by Oren / ecaii.
- [muyuanjin/kcd2-mod-docs](https://github.com/muyuanjin/kcd2-mod-docs) —
  reference disassembly notes for KCD2 reverse engineering.
- [TsudaKageyu/minhook](https://github.com/TsudaKageyu/minhook) — vendored.
- [marzer/tomlplusplus](https://github.com/marzer/tomlplusplus) — vendored.
- Lua 5.1 sources from CryEngine 5.2.3 SDK — same Lua the game ships
  internally.

## License

MIT. See [LICENSE](LICENSE).
