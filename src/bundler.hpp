#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kestral {

struct EmbeddedBundle {
    std::string entry_name;
    std::string entry_source;
    std::vector<std::pair<std::string, std::string>> files;
};

struct BuildBinaryResult {
    std::filesystem::path output_path;
    std::size_t size_bytes = 0;
    std::size_t bundled_file_count = 0;
};

std::filesystem::path get_self_executable_path();

std::string encode_bundle_trailer(const EmbeddedBundle& bundle);
std::optional<EmbeddedBundle> decode_bundle_trailer(const std::string& bytes);
std::size_t stripped_host_size(const std::string& bytes);

std::optional<EmbeddedBundle> read_embedded_bundle_from_file(const std::filesystem::path& exe_path);
std::optional<EmbeddedBundle> read_embedded_bundle();

std::filesystem::path default_binary_output_path(const std::filesystem::path& source_path);

BuildBinaryResult build_standalone_binary_from_source(
    const std::string& entry_name,
    const std::string& source,
    const std::filesystem::path& base_dir,
    const std::filesystem::path& output_path,
    const std::filesystem::path& host_binary_override = {});

BuildBinaryResult build_standalone_binary(
    const std::filesystem::path& source_path,
    const std::filesystem::path& output_path = {},
    const std::filesystem::path& host_binary_override = {});

} // namespace kestral
