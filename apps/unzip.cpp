#define LP3_RSRC_LOG_ERROR(...) fmt::print(__VA_ARGS__)

#include <fmt/format.h>
#include <iostream>
#include <lp3/rsrc.hpp>
#include <lp3/rsrc/ZipFile.hpp>
#include <lp3/sdl.hpp>
#include <string_view>
#include <vector>
#include <zlib.h>

using namespace lp3::rsrc;

int main(int argc, char ** argv) {
    if (argc != 3) {
        fmt::print("Usage: {} <zipfile> <interior file to unzip>\n",
                   argc > 0 ? argv[0] : "unzip");
        return 1;
    }
    lp3::rsrc::Directory dir(".");
    ZipFile zip(dir.load(argv[1]));

    auto story = zip.load(argv[2]);
    std::string contents(story.size(), ' ');
    if (1 == story.read(contents.data(), story.size())) {
        std::cout << contents << "\n";
        return 0;
    } else {
        std::cerr << "Something bad happened. :(\n";
        return 1;
    }
}
