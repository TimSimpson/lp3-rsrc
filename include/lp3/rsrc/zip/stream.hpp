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
// class CompressedDataSource
// ----------------------------------------------------------------------------
//     A source for compressed data. A ZipStreamReader uses this to collect
//     more compressed data from some other source to avoid having to read
//     all of the compressed data in at once.
// ----------------------------------------------------------------------------
class CompressedDataSource {
  public:
    virtual ~CompressedDataSource() {}

    // This is initally false, and turns to true when the data is all out.
    // Calls to `read_data` after that are not allowed.
    virtual bool eof();

    // Writes at most max_size bytes to the array pointed to by dst.
    // Returns amount written.
    virtual std::int64_t read_data(char *dst, std::int64_t max_size) = 0;
};

class ZipStreamReader {
  public:
    ZipStreamReader(std::int64_t compressed_buffer_size,
                    std::int64_t uncompressed_buffer_size);

    ~ZipStreamReader();

    struct ReadResult {
        char *data;         // starting point of data. nullptr if EOF
        std::int64_t count; // size - 0 if EOF
        bool eof;           // true if no more data
    };

    // Reads as much as the stream as it can to fill buffer. Returns a pointer
    // to the first character in buffer and the amount of data written.
    ReadResult read(CompressedDataSource &source);

  private:
    std::vector<char> compressed;
    std::vector<char> uncompressed;
    bool closed;
    z_stream z_s;
    std::int64_t compressed_data_available;

    void close(bool can_throw);
    void ensure_compressed_buffer_full(CompressedDataSource &source);
};

} // namespace lp3::rsrc::zip

#endif
