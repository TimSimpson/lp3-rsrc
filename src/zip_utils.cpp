#include <lp3/rsrc/zip_utils.hpp>

namespace lp3::rsrc {

std::optional<LoadedDirectoryInfo> load_directory_info(lp3::sdl::RWops & file) {
    LoadedDirectoryInfo info;

    // Zip files have all the files at the start, then all the directory info,
    // then a end structure that has info on how many directories and stuff
    // there are.

    // So first fast forward the stream to the end and read that structure.
    {
        auto tell = file.seek_from_end(-1 * sizeof(EndOfDirectoryStructure));
        if (tell < sizeof(EndOfDirectoryStructure)) {
            LP3_RSRC_LOG_ERROR("File too small to contain zip file.");
            return std::nullopt;
        }
    }

    file.read(info.end_info);
    // TODO: fix lp3-sdl, then do this:
    // if (!file.read(info.end_info)) {
    //     LP3_RSRC_LOG_ERROR("Error reading zip end dir struct.");
    //     return std::nullopt;
    // }

    if (std::string_view(info.end_info.signature, 4) != std::string_view(end_of_directory_signature, 4)) {
        LP3_RSRC_LOG_ERROR("Missing signature on end dir struct.");
        return std::nullopt;
    }

    // Rewind the stream to read all the directory entries at the end.
    file.seek(-1 * (sizeof(EndOfDirectoryStructure) + info.end_info.size_of_central_directory));

    // reserver space in the buffer
    // TODO: this is wrong. Only reserve `info.end_info.size_of_central_directory`
    info.buffer.resize(
        info.end_info.size_of_central_directory + (info.end_info.total_number_of_entries_in_central_dir * sizeof(CentralDirectoryHeader))
    );
    file.read(info.buffer.data(), info.end_info.size_of_central_directory);

    info.directories.reserve(info.end_info.total_number_of_files);

    char * itr = info.buffer.data();
    for (int index = 0; index < info.end_info.total_number_of_files; ++ index) {
        CentralDirectoryHeader * ptr = (CentralDirectoryHeader *)itr;
        info.directories.push_back(ptr);
        if (std::string_view(ptr->signature, 4) != std::string_view(central_directory_signature, 4)) {
            LP3_RSRC_LOG_ERROR("Directory struct missing signature.");
            return std::nullopt;
        }
        itr += ptr->real_size();
    }

    return info;
}

}  // end lp3::rsrc
