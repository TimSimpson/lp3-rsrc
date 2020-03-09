#ifndef FILE_LP3_RSRC_ENV_VARS_HPP
#define FILE_LP3_RSRC_ENV_VARS_HPP
#pragma once

#include "config.hpp"
#include <optional>
#include <string>

namespace lp3::rsrc {

std::optional<std::string> get_env_var(const char * name);

}

#endif
