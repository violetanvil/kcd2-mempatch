# outfit-swap-in-combat (worked example)

A complete, verified `mempatch.toml` that removes the "You can't switch
outfits in combat" restriction in Kingdom Come: Deliverance II.
Demonstrates how Tier-1 (pattern) and Tier-2 (context) locators work
together to make a patch safe across minor game updates.

This is the plugin built up step-by-step in
[`docs/finding-patch-sites.md`](../../docs/finding-patch-sites.md) — the
investigation methodology guide for plugin authors. Reading the guide
end-to-end shows how the bytes in this TOML were derived.

## What the patch does

Patches a single 3-byte instruction inside `WHGame.dll` — specifically the
function that registers the `next_outfit` action binding for the
`apse_change_outfit` action map (Ghidra naming: `FUN_1805616e8+0x11`).
Forces the `IsInCombat()` result to 0 for this one binding so the action
is always enabled and the popup never fires.

| | Bytes | Instruction |
|---|---|---|
| Original | `44 8A F0` | `mov r14b, al` — store `IsInCombat()` result |
| Patched | `45 31 F6` | `xor r14d, r14d` — force result to 0 |

Verified live on KCD2 build `release_1_5_1164953_841` (2026-05-17).

## What this example demonstrates

- A real 16-byte Tier-1 `pattern` confirmed to produce exactly 1 match in
  current `WHGame.dll`.
- A 23-byte Tier-2 `context` that wraps the pattern with the preceding
  `mov rcx, [rax+0x90]` instruction, adding a second independent
  uniqueness constraint.
- `original` bytes verification (`44 8A F0`) so the engine refuses to
  write if a future game update changes the instruction.
- `idempotent = true` so relaunches without a Steam restart log
  "already applied" instead of erroring.

Tier-3 is not declared here because the `cant_change_outfit_in_combat`
localization key has zero static xrefs in the binary (the engine resolves
it indirectly at startup via the localization hash table). For patches
whose surrounding function uses a unique `.rdata` string literal,
`anchor_string` provides the strongest guarantee. See
[`docs/writing-safe-patches.md`](../../docs/writing-safe-patches.md).

## Interaction with other plugins

This patch writes 3 bytes at one specific address. If another plugin
is installed that also touches bytes in or near this region, mempatch's
pre-flight pass will categorize the overlap and log accordingly — see
[`../conflict-test-incidental/`](../conflict-test-incidental/) for what
"incidental coexistence" looks like in the log, and
[`../conflict-test-on-original/`](../conflict-test-on-original/) for
what a write-on-original conflict looks like.

## Releases

This example lives in the engine's source tree for reference; it is
**not** bundled into `kcd2-mempatch` release zips. To install the patch
as a player, drop this folder (or just its `mempatch.toml`) into your
game's `plugins/` folder alongside `mempatch.asi`.
