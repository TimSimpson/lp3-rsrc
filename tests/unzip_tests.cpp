// ---------------------------------------------------------------------------
// The following test shows how to read using the unzippers
// ---------------------------------------------------------------------------
#include <lp3/rsrc.hpp>
#include <string>
#include <vector>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>


TEST_CASE("Read file", "[read_a_file]") {
    lp3::rsrc::Directory dir{"resources"};
    lp3::rsrc::ZipFile zip(dir.load("moby.zip"));

    {
        auto readme = zip.load("info/README.md");
        std::string contents(readme.size(), ' ');
        const auto result = readme.read(contents.data(), readme.size());
        REQUIRE(1 == result);
        REQUIRE(
            "# Moby Wha?\n"
            "\n"
            "The file 2701-0.txt came from Project Gutenberg and is public domain.\n"
            "For info, visit\n"
            "\n"
            "    https://www.gutenberg.org/ebooks/2701\n"
            == contents);
    }
}
// ~end-doc
