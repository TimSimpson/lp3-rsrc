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

int main(int argc, char **argv) {
    lp3::rsrc::Directory dir("resources/text");
    ZipFile zip(dir.load("story.zip"));

    auto print_file = [&](const char *file) {
        auto story = zip.load(file);
        std::string contents(story.size(), ' ');
        if (1 == story.read(contents.data(), story.size())) {
            fmt::print(":: uncompressed {}:\n", file);
            std::cout << contents << "\n";
        } else {
            fmt::print("Something bad happened. :(\n");
        }
    };

    print_file("story.txt");
    print_file("doc.md");
}
