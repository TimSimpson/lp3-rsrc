
#include <lp3/rsrc/zip/stream.hpp>

namespace lp3::rsrc::zip {

constexpr ZipStreamInflater::ReadResult EOF_RESULT = {nullptr, 0, true};

ZipStreamInflater::ZipStreamInflater(std::int64_t compressed_buffer_size,
                                     std::int64_t uncompressed_buffer_size)
    : compressed(compressed_buffer_size),
      uncompressed(uncompressed_buffer_size),
      closed(true),
      z_s(),
      compressed_data_available(0) {
    this->z_s.next_in = 0;
    this->z_s.avail_in = 0;
    this->z_s.total_in = 0;
    this->z_s.next_out = 0;
    this->z_s.avail_out = 0;
    this->z_s.total_out = 0;
    this->z_s.msg = nullptr;
    this->z_s.opaque = 0;
    this->z_s.zalloc = nullptr;
    this->z_s.zfree = nullptr;

    const auto init_result = inflateInit2(&this->z_s, -MAX_WBITS);
    if (Z_OK != init_result) {
        LP3_RSRC_LOG_ERROR("error initializing zlib stream for!");
        throw std::runtime_error("error init'ing zlib stream");
    }

    closed = false;
}

ZipStreamInflater::~ZipStreamInflater() { close(false); }

void ZipStreamInflater::close(bool can_throw) {
    if (this->closed) {
        return;
    }
    this->closed = true;
    if (Z_OK != inflateEnd(&this->z_s)) {
        if (nullptr != this->z_s.msg) {
            LP3_RSRC_LOG_ERROR("Zlib inflateEnd error: {}", this->z_s.msg);
            if (can_throw) {
                throw std::runtime_error("error shutting down zlib stream");
            }
        }
    }
}

void ZipStreamInflater::ensure_compressed_buffer_full(
        ZipStreamSource & source) {
    if (this->compressed_data_available >= this->compressed.size()) {
        // Buffer is full, so just return.
        return;
    }
    if (source.eof()) {
        if (this->compressed_data_available == 0) {
            LP3_RSRC_LOG_ERROR(
                    "EOF on input stream, but compressed data has been "
                    "exhausted!\n");
            throw std::runtime_error("Bad zip stream\n");
        } else {
            return; // source is empty, but we still have data
        }
    }

    char * start_at = this->compressed.data() + this->compressed_data_available;
    auto max = this->compressed.size() - this->compressed_data_available;

    // The first step: completely fill compressed buffer.
    auto result = source.read_data(start_at, max);
    this->compressed_data_available = this->compressed_data_available + result;
}

ZipStreamInflater::ReadResult
ZipStreamInflater::read(ZipStreamSource & source) {
    if (closed) {
        return EOF_RESULT;
    }
    ensure_compressed_buffer_full(source);

    this->z_s.next_in = (Bytef *)this->compressed.data();
    this->z_s.avail_in = compressed_data_available;
    this->z_s.next_out = (Bytef *)this->uncompressed.data();
    this->z_s.avail_out = this->uncompressed.size();

    const int result = inflate(&this->z_s, Z_NO_FLUSH); // Z_FINISH);

    if (Z_OK != result && Z_STREAM_END != result) {
        LP3_RSRC_LOG_ERROR(
                "error reading from zip stream ({}): Message: {} \n", result,
                this->z_s.msg == nullptr ? "nullptr" : this->z_s.msg);
        throw std::runtime_error("error reading from zip stream");
    }
    std::int64_t count = this->uncompressed.size() - this->z_s.avail_out;

    // Move unread compressed data to start for next time.
    // TODO: A circular buffer might be faster here.
    std::memmove(this->compressed.data(), this->z_s.next_in,
                 this->z_s.avail_in);
    this->compressed_data_available = this->z_s.avail_in;
    bool eof = false;
    if (Z_STREAM_END == result) {
        close(true);
        eof = true;
    }
    return {this->uncompressed.data(), count, eof};
}

} // namespace lp3::rsrc::zip
