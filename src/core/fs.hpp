#pragma once

#include <optional>
#include <string>
#include <string_view>

/// @brief Whole-file read/write built on SDL I/O.
///
/// This is the only place in the game that touches the filesystem
/// (ADR-0005). Console ports swap this file: asset reads come from the
/// platform's read-only image and user data goes through its save-data
/// API, while every caller keeps passing plain paths.
namespace raven::fs {

/// @brief Read an entire file into a string.
/// @param path Path to the file (absolute, or as resolved by paths::asset).
/// @return The file contents, or std::nullopt if it could not be opened or read.
[[nodiscard]] std::optional<std::string> read_text(const std::string& path);

/// @brief Replace a file's contents atomically.
///
/// Writes to a temporary sibling first and renames it over the target,
/// so a crash or power loss mid-write never leaves a truncated file.
/// @param path Destination path. An empty path fails without touching disk.
/// @param data Bytes to write.
/// @return True if the file was written and moved into place.
[[nodiscard]] bool write_text(const std::string& path, std::string_view data);

} // namespace raven::fs
