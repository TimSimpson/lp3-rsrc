// ---------------------------------------------------------------------------
// The following test shows how to read using the unzippers
// ---------------------------------------------------------------------------
#include <fmt/format.h>
#include <iostream>
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
        REQUIRE("# Moby Wha?\n"
                "\n"
                "The file 2701-0.txt came from Project Gutenberg and is public "
                "domain.\n"
                "For info, visit\n"
                "\n"
                "    https://www.gutenberg.org/ebooks/2701\n"
                == contents);
    }

    {
        auto moby = zip.load("2701-0.txt");
        std::string contents(moby.size(), '?');
        const auto result = moby.read(contents.data(), moby.size());

        constexpr auto size = 1'276'201;
        REQUIRE(moby.size() == size);

        const std::string expected_eof("This Web site includes information "
                                       "about Project Gutenberg-tm,\r\n"
                                       "including how to make donations to the "
                                       "Project Gutenberg Literary\r\n"
                                       "Archive Foundation, how to help "
                                       "produce our new eBooks, and how to\r\n"
                                       "subscribe to our email newsletter to "
                                       "hear about new eBooks.\r\n"
                                       "\r\n"
                                       "\r\n"
                                       "\r\n"
                                       "\r\n"
                                       "\r\n"
                                       "\r\n");
        const std::string_view actual_eof(contents.data() + size
                                                  - expected_eof.size(),
                                          expected_eof.size());

        REQUIRE(expected_eof == actual_eof);
    }
}
// ~end-doc
