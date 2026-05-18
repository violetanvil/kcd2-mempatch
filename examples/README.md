# examples

Reference plugin configurations for kcd2-mempatch plugin authors.

**These files are NOT included in `kcd2-mempatch` release zips.** They
live in the source tree as references — material you copy from, study,
and adapt when writing your own plugin.

## Folders

- [**outfit-swap-in-combat/**](outfit-swap-in-combat/) — a real, working
  `mempatch.toml` that removes the "You can't switch outfits in combat"
  popup. Demonstrates Tier-1 (pattern) + Tier-2 (context) locators with
  verified-unique bytes. The companion of the worked example in
  [`docs/finding-patch-sites.md`](../docs/finding-patch-sites.md).
- [**template-fully-commented/**](template-fully-commented/) — a
  heavily-annotated schema reference covering every field, including
  Tier-3 anchors. Uses intentionally non-matching bytes so it's safe to
  drop into a game folder unedited (the engine logs "pattern not found"
  and applies nothing).
- [**conflict-test-incidental/**](conflict-test-incidental/) —
  synthetic plugin demonstrating that mempatch tolerates *incidental*
  byte overlaps: when this plugin and `outfit-swap-in-combat/` are
  installed together, both apply successfully because they read
  overlapping bytes but don't write to the same place. Headline test
  of the multi-plugin-coexistence design.
- [**conflict-test-on-original/**](conflict-test-on-original/) —
  synthetic plugin demonstrating mempatch's write-on-original conflict
  diagnostic: when installed alongside `outfit-swap-in-combat/`, both
  plugins target the same 3-byte site; the later one aborts cleanly
  with a plain-English log line naming the upstream mod.

## Writing your own plugin

1. **Find the bytes you need to patch.** Read
   [`../docs/finding-patch-sites.md`](../docs/finding-patch-sites.md) for
   the investigation workflow (Ghidra + x64dbg + the trap that costs
   most people half a day).
2. Copy `template-fully-commented/` to a new folder named for your plugin.
3. Edit `mempatch.toml` — rename, replace patterns / bytes with the real
   targets you reverse-engineered.
4. Read [`../docs/writing-safe-patches.md`](../docs/writing-safe-patches.md)
   for safety methodology (locator tiers, when to add context, when to
   use anchors).
5. Read [`../docs/config-schema.md`](../docs/config-schema.md) for the
   formal field reference.
6. Test with `[mempatch] dry_run = true` first.
