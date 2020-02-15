#include <lp3/rsrc/directory.hpp>

#include <fmt/format.h>


namespace lp3::rsrc {

LP3_RSRC_API
Directory::Directory(const std::string & _base_directory)
:   base_directory(_base_directory)
{}

LP3_RSRC_API
sdl::RWops Directory::load(const char * file) {
    const auto full_path = fmt::format("{}/{}", base_directory, file);
    SDL_RWops * ptr = SDL_RWFromFile(full_path.c_str(), "rb");
    if (!ptr) {
        const auto error_msg = fmt::format(
            "Error opening file {}: {}", full_path.c_str(), SDL_GetError()
        );
        LP3_RSRC_LOG_ERROR(error_msg.c_str());
        throw std::runtime_error(error_msg);
    }
    return sdl::RWops{ptr};
}

LP3_RSRC_API
sdl::RWops Directory::save(const char * file) {
    const auto full_path = fmt::format("{}/{}", base_directory, file);
    SDL_RWops * ptr = SDL_RWFromFile(full_path.c_str(), "wb");
    if (!ptr) {
        const auto error_msg = fmt::format(
            "Error opening writable file {}: {}", full_path.c_str(), SDL_GetError()
        );
        throw std::runtime_error(error_msg);
    }
    return sdl::RWops{ptr};
}

LP3_RSRC_API
Directory Directory::sub_directory(const char * sub_d) {
    const auto full_path = fmt::format("{}/{}", base_directory, sub_d);
    Directory subm(full_path);
    return subm;
}

}   // end namespace
