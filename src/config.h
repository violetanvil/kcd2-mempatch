#pragma once
#include <string>

namespace mp::config {

// Walks <pluginsDir>/*/mempatch.toml, parses each file, validates entries,
// appends valid entries to mp::patch::g_patches, sets mp::patch::g_dryRun
// from the first file that supplies a non-default value. Sorts the final
// vector by (priority asc, name asc).
//
// On parse errors or schema-validation errors, the offending entry is skipped
// and a log line is emitted; other entries continue to load.
void LoadAllConfigs(const std::wstring& pluginsDir);

}  // namespace mp::config
