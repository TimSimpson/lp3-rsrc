#ifndef LP3_RSRC_CONFIG_HPP
#define LP3_RSRC_CONFIG_HPP
#pragma once

#if !defined(LP3_RSRC_LOG_ERROR)
    #define LP3_RSRC_LOG_ERROR
#endif

#if defined(_WIN32) && (defined(BUILD_SHARED_LIBS) || defined(LP3_RSRC_API_DYNAMIC))
# if defined(LP3_RSRC_API_CREATE)
#  define LP3_RSRC_API __declspec(dllexport)
# else
#  define LP3_RSRC_API __declspec(dllimport)
# endif
#else
# define LP3_RSRC_API
#endif

#endif
