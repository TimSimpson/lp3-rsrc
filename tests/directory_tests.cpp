// ---------------------------------------------------------------------------
// The following test shows how to read from the Directory class.
// ---------------------------------------------------------------------------
#include <lp3/rsrc.hpp>
#include <string>
#include <vector>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

TEST_CASE("Read file", "[read_a_file]") {
    lp3::rsrc::Directory dir{"resources"};
    lp3::rsrc::Directory dir2 = dir.sub_directory("text");
    auto story = dir2.load("story.txt");
    char content[16];
    std::size_t read_size = story.read(content, 1, 16);

    REQUIRE(std::size_t{16} == read_size);

    std::string contents(content, read_size);
    REQUIRE(contents == "Romulus and Remu");
}
// ~end-doc
