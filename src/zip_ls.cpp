#define LP3_RSRC_LOG_ERROR(...) fmt::print(__VA_ARGS__)

#include <string_view>
#include <vector>
#include <fmt/format.h>
#include <lp3/rsrc.hpp>
#include <lp3/rsrc/zip_utils.hpp>
#include <lp3/sdl.hpp>
#include <zlib.h>

using namespace lp3::rsrc;


int main(int argc, char ** argv) {
    lp3::rsrc::Directory dir("resources/text");
    auto file = dir.load("story.zip");

    auto result = load_directory_info(file);
    if (!result) {
        fmt::print("Error loading zip. :'(\n");
        return 1;
    }
    LoadedDirectoryInfo info = *result;
    fmt::print("* * Zip File Info * *\n");
    fmt::print("{}\n", info.end_info);

    for (auto * dir: info.directories) {
        fmt::print("{}\n\t{}\n", dir->get_name(), *dir);
    }
}
