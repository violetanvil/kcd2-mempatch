# template-fully-commented (schema reference)

A heavily-commented `mempatch.toml` that demonstrates every field the engine
understands, including the optional Tier-2 `context` locator and Tier-3
anchor locators.

**This template uses fake patterns that intentionally do not match anything
in `WHGame.dll`.** If you copy this file unchanged into your game's
`plugins/` folder, the engine logs "pattern not found (0 matches)" and
applies nothing. Safe to drop and inspect.

## How to use it

1. **Reverse-engineer your patch first.** Use
   [`docs/finding-patch-sites.md`](../../docs/finding-patch-sites.md)
   for the investigation methodology — finding the right anchor string,
   reading the decompile, identifying the gate, verifying in x64dbg.
   You should know the AOB pattern, offset, original bytes, and
   replacement bytes before touching this template.
2. Copy the `template-fully-commented/` folder to a new name like
   `<your-plugin-name>/`.
3. Edit `mempatch.toml`: rename the `[[patch]]` `name`, replace the
   placeholder pattern / original / replacement with your real bytes.
4. Decide whether you need `context` (Tier 2) and / or `anchor_string`
   (Tier 3). See [`docs/writing-safe-patches.md`](../../docs/writing-safe-patches.md)
   for guidance.
5. Test with `[mempatch] dry_run = true` first, watch `mempatch.log` to
   confirm match counts are sane, then go live.

## What you'll find in the TOML

- `[mempatch] dry_run` — global flag
- `[[patch]]` block with every field documented inline:
  - identification: `name`, `description`, `priority`
  - target: `module`
  - Tier 1 (required): `pattern`, `offset`, `original`, `replacement`, `idempotent`
  - Tier 2 (recommended): `context`
  - Tier 3 (optional, strongest): `anchor_string` / `anchor_function_by_export` / `anchor_symbol`, `max_anchor_distance`

## Multi-plugin coexistence

`priority` (default 100, lower applies first) matters when several
plugins are installed at once. The engine runs a pre-flight pass that
detects three kinds of byte overlap between plugins:

- **Incidental** (your pattern includes bytes another plugin writes
  somewhere you don't write) — silent and safe. Both plugins apply.
- **Write-on-original** (another plugin writes the bytes your
  `original` expects) — logged as `[WARN]`, your plugin aborts cleanly.
- **Write-on-write** (you and another plugin target the same bytes) —
  logged; both writes happen in priority order, later one wins.

Full reference in
[`../../docs/writing-safe-patches.md`](../../docs/writing-safe-patches.md).
See [`../conflict-test-incidental/`](../conflict-test-incidental/) and
[`../conflict-test-on-original/`](../conflict-test-on-original/) for
plugins that exercise each diagnostic.

## Releases

Not shipped in `kcd2-mempatch` release zips. Lives in the source tree as a
reference for plugin authors.
