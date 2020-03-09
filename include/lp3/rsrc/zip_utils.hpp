#ifndef FILE_LP3_RSRC_ZIP_UTILS_HPP
#define FILE_LP3_RSRC_ZIP_UTILS_HPP
#pragma once

#include <fmt/format.h>
#include <lp3/rsrc.hpp>
#include <lp3/sdl.hpp>
#include <string_view>
#include <vector>
#include <zlib.h>

namespace lp3::rsrc {

enum class CompressionMethods {
    NONE = 0,
    LZW = 1, // 1 - Shrunk / LZW, 8K buffer, 9-13 bits with partial clearing
    REDUCED_1 = 2, // - Reduced-1 / Probalistic compression, lower 7 bits
    REDUCED_2 = 3, // - Reduced-2 / Probalistic compression, lower 6 bits
    REDUCDE_3 = 4, // - Reduced-3 / Probalistic compression, lower 5 bits
    REDUCED_4 = 5, // 5 - Reduced-4 / Probalistic compression, lower 4 bits
    IMPLODED = 6   // - Imploded / 2/3 Shanno-Fano
};

#pragma pack(1)

// See https://www.fileformat.info/format/zip/corion.htm

struct FileHeader {
    char signature[4];
    std::uint16_t version;
    std::uint16_t flags;
    std::uint16_t compression_method;
    std::uint32_t dos_date_time;
    std::uint32_t crc_of_file;
    std::uint32_t compressed_file_size;
    std::uint32_t uncompressed_file_size;
    std::uint16_t file_name_length;
    std::uint16_t extra_field_length;

    std::ptrdiff_t real_size() const {
        return (sizeof(*this))
               + (this->file_name_length + this->extra_field_length);
    }
};

constexpr char file_header_signature[] = {'P', 'K', (char)03, (char)04};

struct CentralDirectoryHeader {
    char signature[4];
    std::uint8_t version_made_by;
    std::uint8_t host_os;
    std::uint8_t minimum_version_needed_to_extract;
    std::uint8_t target_os;
    std::uint16_t flags;
    std::uint16_t compression_method;
    std::uint32_t dos_date_time_of_file;
    std::uint32_t crc_of_file;
    std::uint32_t compressed_file_size;
    std::uint32_t uncompressed_file_size;
    std::uint16_t file_name_length;
    std::uint16_t extra_field_length;
    std::uint16_t file_comment_length;
    std::uint16_t disk_number;
    std::uint16_t internal_attributes;
    std::uint32_t external_file_attributes;
    std::uint32_t relative_offset_of_local_header_from_start_of_first_disk;

    std::string_view get_name() const {
        char * ptr = (char *)(this + 1);
        return std::string_view(ptr, this->file_name_length);
    }

    std::string_view get_extra_field() const {
        const auto name = this->get_name();
        return std::string_view(name.data() + name.size(),
                                this->extra_field_length);
    }

    std::string_view get_file_comment() const {
        const auto ef = this->get_extra_field();
        return std::string_view(ef.data() + ef.size(),
                                this->file_comment_length);
    }

    std::ptrdiff_t real_size() const {
        return (sizeof(*this))
               + (this->file_name_length + this->extra_field_length
                  + this->file_comment_length);
    }
};

constexpr char central_directory_signature[] = {'P', 'K', (char)01, (char)02};

struct EndOfDirectoryStructure {
    char signature[4];
    std::uint16_t number_of_this_disk;
    std::uint16_t number_of_disk_with_start_of_central;
    std::uint16_t total_number_of_files;
    std::uint16_t total_number_of_entries_in_central_dir;
    std::uint32_t size_of_central_directory;
    std::uint32_t offset_of_start_of_central_directory;
    std::uint16_t archive_comment_length;
};

constexpr char end_of_directory_signature[] = {'P', 'K', (char)05, (char)06};

#pragma pack()

struct LoadedDirectoryInfo {
    EndOfDirectoryStructure end_info;
    std::vector<char> buffer;
    // TODO: Change to std::ref
    std::vector<CentralDirectoryHeader *> directories;
};

std::optional<LoadedDirectoryInfo> load_directory_info(lp3::sdl::RWops & file);

} // namespace lp3::rsrc

template <> struct fmt::formatter<lp3::rsrc::FileHeader> {
    constexpr auto parse(format_parse_context & ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const lp3::rsrc::FileHeader & file_header,
                FormatContext & ctx) {
        format_to(ctx.out(), "{{ signature={0}{1}{2}{3}, ",
                  file_header.signature[0], file_header.signature[1],
                  file_header.signature[2], file_header.signature[3]);
        format_to(ctx.out(), "version={0}, ", file_header.version);
        format_to(ctx.out(), "flags={0}, ", file_header.flags);
        format_to(ctx.out(), "compression_method={0}, ",
                  file_header.compression_method);
        format_to(ctx.out(), "dos_date_time={0}, ", file_header.dos_date_time);
        format_to(ctx.out(), "crc_of_file={0}, ", file_header.crc_of_file);
        format_to(ctx.out(), "uncompressed_file_size={0}, ",
                  file_header.uncompressed_file_size);
        format_to(ctx.out(), "file_name_length={0}, ",
                  file_header.file_name_length);
        format_to(ctx.out(), "extra_field_length={0} }}",
                  file_header.extra_field_length);
        return ctx.out();
    }
};

template <> struct fmt::formatter<lp3::rsrc::CentralDirectoryHeader> {
    constexpr auto parse(format_parse_context & ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const lp3::rsrc::CentralDirectoryHeader & s,
                FormatContext & ctx) {
        format_to(ctx.out(), "{{ signature={0}{1}{2}{3}, ", s.signature[0],
                  s.signature[1], s.signature[2], s.signature[3]);
        format_to(ctx.out(), "version_made_by={}, ", s.version_made_by);
        format_to(ctx.out(), "host_os={}, ", s.host_os);
        format_to(ctx.out(), "minimum_version_needed_to_extract={}, ",
                  s.minimum_version_needed_to_extract);
        format_to(ctx.out(), "target_os={}, ", s.target_os);
        format_to(ctx.out(), "flags={}, ", s.flags);
        format_to(ctx.out(), "compression_method={}, ", s.compression_method);
        format_to(ctx.out(), "dos_date_time_of_file={}, ",
                  s.dos_date_time_of_file);
        format_to(ctx.out(), "crc_of_file={}, ", s.crc_of_file);
        format_to(ctx.out(), "compressed_file_size={}, ",
                  s.compressed_file_size);
        format_to(ctx.out(), "uncompressed_file_size={}, ",
                  s.uncompressed_file_size);
        format_to(ctx.out(), "file_name_length={}, ", s.file_name_length);
        format_to(ctx.out(), "extra_field_length={}, ", s.extra_field_length);
        format_to(ctx.out(), "file_comment_length={}, ", s.file_comment_length);
        format_to(ctx.out(), "disk_number={}, ", s.disk_number);
        format_to(ctx.out(), "internal_attributes={}, ", s.internal_attributes);
        format_to(ctx.out(), "external_file_attributes={}, ",
                  s.external_file_attributes);
        format_to(ctx.out(),
                  "relative_offset_of_local_header_from_start_of_first_disk={}",
                  s.relative_offset_of_local_header_from_start_of_first_disk);
        format_to(ctx.out(), "}}");
        return ctx.out();
    }
};

template <> struct fmt::formatter<lp3::rsrc::EndOfDirectoryStructure> {
    constexpr auto parse(format_parse_context & ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const lp3::rsrc::EndOfDirectoryStructure & s,
                FormatContext & ctx) {
        format_to(ctx.out(), "{{ signature={0}{1}{2}{3}, ", s.signature[0],
                  s.signature[1], s.signature[2], s.signature[3]);
        format_to(ctx.out(), "number_of_this_disk={}, ", s.number_of_this_disk);
        format_to(ctx.out(), "number_of_disk_with_start_of_central={}, ",
                  s.number_of_disk_with_start_of_central);
        format_to(ctx.out(), "total_number_of_files={}, ",
                  s.total_number_of_files);
        format_to(ctx.out(), "total_number_of_entries_in_central_dir={}, ",
                  s.total_number_of_entries_in_central_dir);
        format_to(ctx.out(), "size_of_central_directory={}, ",
                  s.size_of_central_directory);
        format_to(ctx.out(), "offset_of_start_of_central_directory={}, ",
                  s.offset_of_start_of_central_directory);
        format_to(ctx.out(), "archive_comment_length={}",
                  s.archive_comment_length);
        format_to(ctx.out(), "}}");
        return ctx.out();
    }
};

#endif
