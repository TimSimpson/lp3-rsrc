// ----------------------------------------------------------------------------
// Lp3-Rsrc Containers
// ============================================================================
//      Contains simple containers which can load files in the form of
//      RWops.
//
// ---------------------------------------------------------------------------/
// ~end-doc summary

#ifndef FILE_LP3_RSRC_CONTAINERS_HPP
#define FILE_LP3_RSRC_CONTAINERS_HPP
#pragma once

#include "config.hpp"
#include <lp3/sdl.hpp>

namespace lp3::rsrc {

// ----------------------------------------------------------------------------
// class RContainer
// ----------------------------------------------------------------------------
//     A container from which you can load readable RWops.
// ----------------------------------------------------------------------------
class LP3_RSRC_API RContainer {
  public:
    virtual ~RContainer() {}

    /* Opens a resource for reading. */
    virtual sdl::RWops load(const char *file) = 0;
};
// -/

// ----------------------------------------------------------------------------
// class WContainer
// ----------------------------------------------------------------------------
//     A container you can write to.
// ----------------------------------------------------------------------------
class LP3_RSRC_API WContainer {
  public:
    virtual ~WContainer() {}

    /* Opens a resource for writing. */
    virtual sdl::RWops save(const char *file) = 0;
};
// -/

} // namespace lp3::rsrc

#endif
