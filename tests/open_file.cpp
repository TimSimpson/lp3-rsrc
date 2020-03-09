#include <fmt/format.h>
#include <iostream>
#include <lp3/rsrc.hpp>

namespace rsrc = lp3::rsrc;

int main(const int argc, const char ** argv) {
    fmt::print("Hi\n");

    rsrc::Directory dir{"."};
    auto file = dir.load("foobar.txt");
    std::string contents(file.size(), ' ');
    if (1 == file.read(contents.data(), file.size())) {
        std::cout << contents << "\n";
    } else {
        fmt::print("Something bad happened. :(\n");
    }

    return 0;
}
