# conflict-test-on-original (engine developer reference)

**This is not a real patch.** It exercises mempatch's write-on-original
conflict diagnostic.

## Scenario

When installed alongside [`../outfit-swap-in-combat/`](../outfit-swap-in-combat/):

1. Both plugins target the exact same 3-byte site
   (`mov r14b, al` at `FUN_1805616e8+0x11`).
2. Pre-flight detects that outfit-swap's write footprint overlaps this
   plugin's `original` read range — i.e., this plugin's verify check
   depends on bytes that outfit-swap will modify.
3. Pre-flight logs a `[WARN]` line naming both plugins:

   ```
   [WARN] Plugin 'outfit_swap_in_combat' modified bytes that plugin
          'conflict_test_on_original' needs to verify before patching
          (overlap at 0x...). The earlier mod stopped the later one
          from applying. Try removing or reordering one of them.
   ```

4. Outfit-swap applies first, writing `45 31 F6`.
5. This plugin runs next. Verify check finds `45 31 F6` at the site
   but expected `44 8A F0`; aborts cleanly. The abort log line carries
   the same conflict explanation again (so a user reporting a bug sees
   the cause at the point of failure, not just at startup).

This synthetic patch never writes bytes (it always aborts). Safe to
drop into a real game folder; remove the folder when done.

## Releases

Not shipped in `kcd2-mempatch` release zips.
