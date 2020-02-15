#ifndef FILE_LP3_RSRC_DIRECTORY_HPP
#define FILE_LP3_RSRC_DIRECTORY_HPP
#pragma once

#include <string>
#include <lp3/sdl.hpp>
#include "containers.hpp"
#include "config.hpp"

namespace lp3::rsrc {

// ----------------------------------------------------------------------------
// class Directory
// ----------------------------------------------------------------------------
//     Represents a directory. Pretty simple, right?
// ----------------------------------------------------------------------------
class
LP3_RSRC_API
Directory : public RContainer, public WContainer
{
public:
    Directory(const std::string & _base_directory);

    /* Opens a resource for reading. */
    sdl::RWops load(const char * file) override;

    /* Opens a resource for writing. */
    sdl::RWops save(const char * file) override;

    /* Opens a new Media Manager in a sub directory. */
    Directory sub_directory(const char * sub_directory);

private:
    const std::string base_directory;
};
// -/

}   // end lp3::rsrc namespace

#endif
