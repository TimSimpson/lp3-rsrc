#include <lp3/rsrc/env_vars.hpp>

#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

namespace lp3::rsrc {

#if !defined(_WIN32)

std::optional<std::string> get_env_var(const char * name) {
    const char * const env_var_value = getenv(name);
    if (env_var_value) {
        return std::string{env_var_value};
    } else {
        return std::nullopt;
    }
}

#else

std::optional<std::string> get_env_var(const char * name) {
    char * env_var_value;
    std::size_t length;
    auto result = _dupenv_s(&env_var_value, &length, name);
    if (0 != result) {
        return std::nullopt;
    }
    std::unique_ptr<char> delete_env_var(env_var_value);
    if (env_var_value) {
        return std::string{env_var_value};
    } else {
        return std::nullopt;
    }
}

#endif

} // namespace lp3::rsrc
