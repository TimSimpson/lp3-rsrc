#ifndef FILE_LP3_RSRC_ZIPFILE_HPP
#define FILE_LP3_RSRC_ZIPFILE_HPP
#pragma once

#include "config.hpp"
#include "containers.hpp"
#include <list>
#include <lp3/sdl.hpp>
#include <string>
#include <vector>

namespace lp3::rsrc {

// ------------------------------------------------------------------------
// class ZipFile
// ------------------------------------------------------------------------
//     Represents a zip file.
// ------------------------------------------------------------------------
class LP3_RSRC_API ZipFile : public RContainer {
  public:
    ZipFile(lp3::sdl::RWops && zip_file);
    ~ZipFile();

    /* Opens a resource for reading. */
    sdl::RWops load(const char * file) override;

    sdl::RWops load(const char * file, std::int64_t compression_buffer_size,
                    std::int64_t decompression_buffer_size);

  private:
    struct FileRef {
        std::string file_name;
        std::uint32_t offset_to_file;
    };

    // class OpenFile {
    // public:
    //     virtual ~OpenFile();
    //     std::string get_file_name() const;
    // };

    lp3::sdl::RWops actual_file;
    std::vector<FileRef> file_refs;
    std::list<std::string> open_files;
};
// -/

} // namespace lp3::rsrc
#endif
