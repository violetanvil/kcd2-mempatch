# conflict-test-incidental (engine developer reference)

**This is not a real patch.** It exercises mempatch's tolerance for
incidental byte overlaps between plugins.

## Scenario

When installed alongside [`../outfit-swap-in-combat/`](../outfit-swap-in-combat/):

1. Both plugins use the **same 16-byte pattern**, but write to
   different offsets within it (outfit-swap at offset 13, this at
   offset 0).
2. Pre-flight resolves both against the pristine DLL and notes that
   outfit-swap's write region overlaps this plugin's *pattern* read
   range. Under the **incidental** classification, this is allowed —
   no `[WARN]` is logged.
3. Outfit-swap applies first, writing `45 31 F6` at offset 13.
4. This plugin runs next. Because pre-flight resolved its `patchAddr`
   against the pristine DLL, the engine doesn't need to re-scan — it
   knows exactly where to apply (offset 0). The verify check at offset
   0 passes (that byte wasn't modified). The no-op write completes.
5. Both plugins log `applied successfully`.

This is the headline test of mempatch's pre-flight design: **plugins
that legitimately reference overlapping bytes can coexist** as long as
they don't write to the same place.

If pre-flight ever logs a `[WARN]` for this configuration, or if this
plugin aborts when running alongside outfit-swap, that's a regression.

This synthetic patch's "write" is a no-op (writes the same byte back
that was already there). Safe to drop into a real game folder.

## Releases

Not shipped in `kcd2-mempatch` release zips.
