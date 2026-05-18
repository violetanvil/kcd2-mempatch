# mempatch.toml schema

One file per plugin folder: `plugins/<plugin_name>/mempatch.toml`.
A file may declare a top-level `[mempatch]` table plus one or more
`[[patch]]` entries.

This doc is the formal field reference. If you're still figuring out
what bytes to patch, start with
[`finding-patch-sites.md`](finding-patch-sites.md) (the investigation
methodology). If you have the bytes and want to know how to ship them
safely, see [`writing-safe-patches.md`](writing-safe-patches.md).

## Top-level options

```toml
[mempatch]
dry_run = false   # bool, optional, default false.
                  # If any loaded config file sets this true, NO writes happen
                  # for the entire session. Locator resolution still runs and
                  # is logged, so you can verify the patches would work against
                  # a new game build before committing.
```

## Patch entries

```toml
[[patch]]
# --- required ---
name         = "string"            # log tag; should be unique across all loaded patches
pattern      = "48 8B 01 ?? ..."   # AOB; hex bytes space-separated, ?? = wildcard
offset       = 0                   # int, offset from pattern start to first byte to write
original     = "44 8A F0"          # expected current bytes at (match + offset)
replacement  = "45 31 F6"          # bytes to write; must be same length as original

# --- optional ---
priority     = 100                 # int, lower applies first. Default 100.
module       = "WHGame.dll"        # PE module name. Default "WHGame.dll".
idempotent   = true                # bool, default true.
                                   # If true and bytes already equal replacement, log
                                   # "patch already applied; skipping" without erroring.
description  = "..."               # free-form notes; not used by the engine.

# --- Tier 2: context (recommended for any non-trivial patch) ---
context      = "longer hex bytes"  # Must contain `pattern`. Must itself produce
                                   # exactly 1 match in the executable sections.
                                   # The pattern match address must fall inside
                                   # the context match window.

# --- Tier 3: anchor (mutually exclusive — declare at most one) ---
anchor_string             = "..."  # A null-terminated string literal in WHGame.dll's
                                   # .rdata. Must be unique. The engine finds the LEA
                                   # xref to it (also must be unique) and requires the
                                   # patch address to fall within the same function
                                   # (function bounds determined via .pdata).
anchor_function_by_export = "..."  # An exported function name in the module. Patch
                                   # must fall within that function's bounds.
anchor_symbol             = "..."  # A CryEngine internal symbol (not yet implemented).

max_anchor_distance = 4096         # int, optional, default 4096.
                                   # Maximum byte distance from anchor to patch address.
```

## Locator agreement

Each declared locator (`pattern` always, `context` if present, anchor if present)
must resolve independently. The engine refuses to write unless they all agree:

1. **`pattern`** must match exactly 1 location in the module's executable sections.
2. **`context`** (if present) must also match exactly 1 location; the `pattern`
   match must be inside the `context` match window.
3. **anchor** (if present) must resolve to a unique function; the patch
   address (`pattern_match + offset`) must fall inside that function and within
   `max_anchor_distance` bytes of the anchor.

If any check fails, the entry is aborted with a log message and the next entry
is attempted. The game always continues running; a failed patch never crashes
the game.

## Original-bytes verification

After locator agreement, the engine reads `original.length` bytes at the patch
address and compares against `original`:

- Match → write `replacement`.
- Does **not** match, but equals `replacement` and `idempotent = true` → log
  "patch already applied; skipping" and continue.
- Anything else → abort with a log message describing the actual bytes found.

## Validation at load time

- `original.length == replacement.length`
- At most one of `anchor_string` / `anchor_function_by_export` / `anchor_symbol`
- `pattern` syntactically valid (hex pairs + optional `??` wildcards)
- `name` is non-empty

Entries failing validation are logged and skipped; other entries still load.

## Load order

All `plugins/*/mempatch.toml` files are discovered, parsed, and merged. Entries
are sorted by `(priority asc, name asc)` and applied in that order. Each entry
is independent; one failure does not abort the others.

## Conflict handling between plugins

Before any patch is applied, the engine runs a **pre-flight pass** that
resolves every patch's locators against the pristine DLL and classifies
each pair of plugins by how their byte footprints overlap:

| Overlap | Engine log | Outcome |
|---|---|---|
| Incidental (writer's bytes inside reader's `pattern` or `context` only) | silent | Both apply. Reader uses its pre-flight resolution; the writer's modifications don't move the reader's target. |
| Write-on-original (writer's bytes overlap reader's verify target) | `[WARN]` | Reader's verify fails at apply time. Reader aborts; log line names the writer as the upstream cause. |
| Write-on-write full overlap (identical write ranges) | `[INFO]` | Both writes happen in priority order. Later plugin's bytes win the contested region. |
| Write-on-write partial overlap (overlapping but not identical write ranges) | `[WARN]` | Both writes happen; the result is a mix of both plugins' bytes which may be an invalid instruction. |

The `priority` field controls who runs first. **Lower priority numbers
apply first.** For write-on-write conflicts, whichever runs *last* wins
the contested bytes.

See [`writing-safe-patches.md`](writing-safe-patches.md) for guidance
on avoiding conflicts and what log lines to expect when they happen.
