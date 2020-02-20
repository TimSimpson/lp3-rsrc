#ifndef FILE_LP3_RSRC_ZIPFILE_HPP
#define FILE_LP3_RSRC_ZIPFILE_HPP
#pragma once

#include "config.hpp"
#include "containers.hpp"
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
    ZipFile(lp3::sdl::RWops &&zip_file);

    /* Opens a resource for reading. */
    sdl::RWops load(const char *file) override;

  private:
    struct FileRef {
        std::string file_name;
        std::uint32_t offset_to_file;
    };

    lp3::sdl::RWops actual_file;
    std::vector<FileRef> file_refs;
};
// -/

} // namespace lp3::rsrc
#endif
