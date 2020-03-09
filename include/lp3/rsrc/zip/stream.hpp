#ifndef FILE_LP3_RSRC_ZIP_STREAM_HPP
#define FILE_LP3_RSRC_ZIP_STREAM_HPP
#pragma once

#include <fmt/format.h>
#include <lp3/rsrc.hpp>
#include <lp3/sdl.hpp>
#include <string_view>
#include <vector>
#include <zlib.h>

namespace lp3::rsrc::zip {

// ----------------------------------------------------------------------------
// class ZipStreamSource
// ----------------------------------------------------------------------------
//     A source for compressed data. A ZipStreamInflater uses this to collect
//     more compressed data from some other source to avoid having to read
//     all of the compressed data in at once.
// ----------------------------------------------------------------------------
class ZipStreamSource {
  public:
    virtual inline ~ZipStreamSource() {}

    // This is initally false, and turns to true when the data is all out.
    // Calls to `read_data` after that are not allowed.
    virtual bool eof() = 0;

    // Writes at most max_size bytes to the array pointed to by dst.
    // Returns amount written.
    virtual std::int64_t read_data(char * dst, std::int64_t max_size) = 0;
};

// ----------------------------------------------------------------------------
// class ZipStreamInflater
// ----------------------------------------------------------------------------
//     Reads uncompresses zipped data from some source and returns it in
//     in chunks determined by "uncompressed_buffer_size".
//
//     Internally it stores two buffers, one for compressed data and one for
//     uncompressed data, along with a z_stream structure and other stuff.
// ----------------------------------------------------------------------------
class ZipStreamInflater {
  public:
    ZipStreamInflater(std::int64_t compressed_buffer_size,
                      std::int64_t uncompressed_buffer_size);
    ZipStreamInflater(ZipStreamInflater && rhs) = default;
    ZipStreamInflater(const ZipStreamInflater & rhs) = delete;
    ZipStreamInflater & operator=(const ZipStreamInflater && rhs) = delete;
    ~ZipStreamInflater();

    struct ReadResult {
        char * data;        // starting point of data. nullptr if EOF
        std::int64_t count; // size - 0 if EOF
        bool eof;           // true if no more data
    };

    // Reads as much as the stream as it can to fill buffer. Returns a pointer
    // to the first character in buffer and the amount of data written.
    // The source should be the same each time to avoid errors.
    ReadResult read(ZipStreamSource & source);

  private:
    std::vector<char> compressed;
    std::vector<char> uncompressed;
    bool closed;
    z_stream z_s;
    std::int64_t compressed_data_available;

    void close(bool can_throw);
    void ensure_compressed_buffer_full(ZipStreamSource & source);
};

} // namespace lp3::rsrc::zip

#endif
