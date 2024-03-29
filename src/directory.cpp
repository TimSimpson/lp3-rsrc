#include <lp3/rsrc/directory.hpp>

#include <fmt/format.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace lp3::rsrc {

namespace {
#if defined(__EMSCRIPTEN__)
    struct EnsureNodeInitialized {
        EnsureNodeInitialized() {
#ifndef NODERAWFS
            // mount the current folder as a NODEFS instance
            // inside of emscripten
            // clang-format off
            EM_ASM(
                try{
                    // TODO: solve this cleaner -
                    // See if an "exists" method is in there.
                    // https://emscripten.org/docs/api_reference/Filesystem-API.html#FS.mount
                    FS.mkdir('/cwd');
                    FS.mount(NODEFS, {root : '.'}, '/cwd');
                    FS.mkdir('/root');
                    FS.mount(NODEFS, {root : '/'}, '/root');
                } catch(error) {
                    // Ignore errors that might happen from somehow calling
                    // this twice.
                }
            );
            // clang-format on
#endif
        }

        std::string parse_path(const std::string & path) {
#ifndef NODERAWFS
            // clang-format off
            const bool is_node = EM_ASM_INT(
                return (
                    (typeof process === 'object'
                        && typeof process.versions === 'object'
                        && typeof process.versions.node !== 'undefined')
                    ? 1 : 0
                );
            ) == 1;
            // clang-format on
            if (is_node) {
                if (path.length() > 0 && path[0] != '/') {
                    return "/cwd/" + path;
                } else {
                    return "/root/" + path;
                }
            }
#endif
            return path;
        }

    } initialize_on_load;

    std::string get_full_path(const std::string & base_directory,
                              const std::string & file) {
        const std::string result = fmt::format("{}/{}", base_directory, file);
        return initialize_on_load.parse_path(result);
    }

#else
    std::string get_full_path(const std::string & base_directory,
                              const std::string & file) {
        return fmt::format("{}/{}", base_directory, file);
    }

#endif
} // namespace

LP3_RSRC_API
Directory::Directory(const std::string & _base_directory)
    : base_directory(_base_directory) {}

LP3_RSRC_API
sdl::RWops Directory::load(const char * file) {
    const auto full_path = get_full_path(base_directory, file);

    SDL_RWops * ptr = SDL_RWFromFile(full_path.c_str(), "rb");
    if (!ptr) {
        const auto error_msg = fmt::format("Error opening file {}: {}",
                                           full_path.c_str(), SDL_GetError());
        LP3_RSRC_LOG_ERROR(error_msg.c_str());
        throw std::runtime_error(error_msg);
    }
    return sdl::RWops{ptr};
}

LP3_RSRC_API
sdl::RWops Directory::save(const char * file) {
    const auto full_path = get_full_path(base_directory, file);
    SDL_RWops * ptr = SDL_RWFromFile(full_path.c_str(), "wb");
    if (!ptr) {
        const auto error_msg = fmt::format("Error opening writable file {}: {}",
                                           full_path.c_str(), SDL_GetError());
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

} // namespace lp3::rsrc
