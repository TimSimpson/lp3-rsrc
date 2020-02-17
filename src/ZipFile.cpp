#include <lp3/rsrc/ZipFile.hpp>
#include <lp3/rsrc/zip_utils.hpp>

namespace lp3::rsrc {

    ZipFile::ZipFile(lp3::sdl::RWops && zip_file)
    :   actual_file(std::move(zip_file)),
        file_refs()
    {
        auto result = load_directory_info(this->actual_file);
        if (!result) {
            LP3_RSRC_LOG_ERROR("Error parsing zip file directory headers.");
            throw std::runtime_error("Bad file.");
        }
        for (auto * dir: result->directories) {
            FileRef ref;
            ref.file_name = dir->get_name();
            ref.offset_to_file = dir->relative_offset_of_local_header_from_start_of_first_disk;
            file_refs.push_back(ref);
        }
    }

    /* Opens a resource for reading. */
    sdl::RWops ZipFile::load(const char * file) {
        // FileHeader
    }

}
