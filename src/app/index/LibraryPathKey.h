#pragma once

#include <filesystem>
#include <string>

namespace atlas::index {

// Produces a process-local comparison key for suppressing duplicate paths
// during one scan run. It is not a durable document identity.
[[nodiscard]] std::string libraryPathKey(const std::filesystem::path& path);

} // namespace atlas::index
