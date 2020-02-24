#include <lp3/rsrc/ZipFile.hpp>

#include <cstring>
#include <iostream>
#include <lp3/rsrc/zip/stream.hpp>
#include <lp3/rsrc/zip_utils.hpp>
#include <lp3/sdl.hpp>
#include <zlib.h>

namespace lp3::rsrc {

namespace {

    using TrackerList = std::list<std::string>;

    TrackerList::iterator push_back_to_tracker(TrackerList &tracker,
                                               const std::string &file_name) {
        return tracker.insert(tracker.end(), file_name);
    }

    struct OpenFileTracker {
        TrackerList &tracker;
        TrackerList::iterator itr;

        OpenFileTracker(const std::string &file_name, TrackerList &tracker_arg)
            : tracker(tracker_arg),
              itr(push_back_to_tracker(tracker_arg, file_name)) {}

        ~OpenFileTracker() { tracker.erase(itr); }
    };

    struct UncompressedFile {
        std::vector<char> buffer;
        std::int64_t position;
        OpenFileTracker tracker;

        UncompressedFile(const std::string &file_name, TrackerList &tracker_arg,
                         std::size_t size)
            : buffer(size), position(0), tracker(file_name, tracker_arg) {}

        std::int64_t size() const { return this->buffer.size(); }

        std::int64_t seek(const long amount, const int whence) {
            // These two asserts aren't really necessary, except that it would
            // be best to keep the behavior the same as the streaming zip file
            // class below.
            SDL_assert(amount >= 0);
            SDL_assert(whence == RW_SEEK_CUR);
            const std::int64_t new_position
                    = (RW_SEEK_SET == whence
                               ? 0
                               : RW_SEEK_END == whence ? this->buffer.size()
                                                       : this->position)
                      + amount;
            if (new_position < 0 || new_position > this->buffer.size()) {
                LP3_RSRC_LOG_ERROR("Bad seek; new_position would be {}/{}",
                                   new_position, this->buffer.size());
                return -1;
            }
            this->position = new_position;
            return new_position;
        }

        std::size_t read(void *dst, std::size_t object_size,
                         std::size_t object_count) {
            const std::size_t full_size = object_size * object_count;
            const std::int64_t new_position = this->position + full_size;
            if (new_position < 0 || new_position > this->buffer.size()) {
                LP3_RSRC_LOG_ERROR("Bad read; would end up at {}/{}",
                                   new_position, this->buffer.size());
                return 0;
            }
            if (this->position < 0 || this->position >= this->buffer.size()) {
                LP3_RSRC_LOG_ERROR("Bad read; would begin at {}/{}",
                                   this->position, this->buffer.size());
                return 0;
            }
            std::memcpy(dst, this->buffer.data() + this->position, full_size);
            return object_count;
        }
    };

    class CompressedFileReader : public zip::ZipStreamSource {

      private:
        bool _eof;
        OpenFileTracker tracker;
        lp3::sdl::RWops &actual_file;
        std::int64_t size_left;

      public:
        CompressedFileReader(const std::string &file_name,
                             TrackerList &tracker_arg,
                             lp3::sdl::RWops &actual_file_arg,
                             std::int64_t size_left_arg)
            : _eof(false),
              tracker(file_name, tracker_arg),
              actual_file(actual_file_arg),
              size_left(size_left_arg) {}

        bool eof() override { return _eof; }

        std::int64_t read_data(char *dst, std::int64_t max_size) override {
            const auto size_to_read = std::min(max_size, this->size_left);
            if (1 != this->actual_file.read(dst, size_to_read)) {
                LP3_RSRC_LOG_ERROR("Error reading zip stream.");
                throw std::runtime_error("Zip stream read error.");
            }
            this->size_left -= size_to_read;
            _eof = this->size_left <= 0;
            return size_to_read;
        }
    };

    struct CompressedFile {
        CompressedFileReader source;
        zip::ZipStreamInflater inflater;
        std::int64_t compressed_file_size;
        std::int64_t uncompressed_file_size;
        std::int64_t position_in_uncompressed_file;
        zip::ZipStreamInflater::ReadResult last_read;

        CompressedFile(const std::string &file_name, TrackerList &tracker_arg,
                       lp3::sdl::RWops &actual_file, std::int64_t size_left_arg,
                       std::int64_t uncompressed_file_size_arg)
            : source(file_name, tracker_arg, actual_file, size_left_arg),
              inflater(1024 * 1024, 1024 * 1024),
              compressed_file_size(size_left_arg),
              uncompressed_file_size(uncompressed_file_size_arg),
              position_in_uncompressed_file(0),
              last_read{nullptr, 0, false} {}

        std::int64_t size() const { return this->uncompressed_file_size; }

        std::int64_t seek(const long amount, const int whence) {
            // These two asserts aren't really necessary, except that it would
            // be best to keep the behavior the same as the streaming zip file
            // class below.
            if (amount < 0 || whence != RW_SEEK_CUR
                || (whence == RW_SEEK_SET
                    && this->position_in_uncompressed_file == 0)) {
                LP3_RSRC_LOG_ERROR("Seek error; whence={}, amount={}", whence,
                                   amount);
                throw std::runtime_error("Can only seek forward.");
            }

            std::int64_t result = 0;
            std::int64_t amount_left = amount;

            while (amount_left > 0 && !this->last_read.eof) {
                // if (this->last_read.eof) {
                //   if (amount_left == 0) {
                //     return 0;
                //   }
                //   raise std::runtime_error("Cannot seek forward; at EOF
                //   already.");
                // }

                if (this->last_read.count <= 0) {
                    this->last_read = this->inflater.read(this->source);
                }
                std::int64_t read_count
                        = std::min(amount_left, this->last_read.count);
                this->last_read.data += read_count;
                this->last_read.count -= read_count;
                this->position_in_uncompressed_file += read_count;
                result += read_count;
                amount_left -= read_count;
            }
            return result;
        }

        std::size_t read(void *dst, std::size_t object_size,
                         std::size_t object_count) {
            std::size_t result = 0;
            std::int64_t amount_left = object_size * object_count;
            char *write_ptr = (char *)dst;

            while (amount_left > 0 && !this->last_read.eof) {
                if (this->last_read.count <= 0) {
                    this->last_read = this->inflater.read(this->source);
                }
                const std::int64_t read_count
                        = std::min(amount_left, this->last_read.count);

                // TODO: double check if cpp reference says overlapping is OK
                // here.
                std::memmove(write_ptr, this->last_read.data, read_count);
                write_ptr += read_count;
                this->last_read.data += read_count;
                this->last_read.count -= read_count;
                this->position_in_uncompressed_file += read_count;
                result += read_count;
                amount_left -= read_count;
                LP3_RSRC_LOG_ERROR("-read {} ({}/{})\n", read_count,
                                   amount_left, object_size * object_count);
            }

            return result / object_size;
        }
    };

    template <class C> struct SDL_RWopsFuncs {
        static C *get_file(SDL_RWops *rwops) {
            return (C *)rwops->hidden.unknown.data1;
        }

        static std::int64_t size(SDL_RWops *rwops) {
            SDL_assert(rwops != nullptr);
            return get_file(rwops)->size();
        }

        static std::int64_t seek(SDL_RWops *rwops, const long amount,
                                 const int whence) {
            SDL_assert(rwops != nullptr);
            return get_file(rwops)->seek(amount, whence);
        }

        static std::size_t read(SDL_RWops *rwops, void *dst,
                                std::size_t object_size,
                                std::size_t object_count) {
            SDL_assert(rwops != nullptr);
            return get_file(rwops)->read(dst, object_size, object_count);
        }

        static std::size_t write(SDL_RWops *rwops, const void *dst,
                                 std::size_t object_count,
                                 std::size_t object_size) {
            SDL_assert(rwops != nullptr);
            LP3_RSRC_LOG_ERROR("An attempt was made to write to a zip file.");
            return 0;
        }

        static int close(SDL_RWops *rwops) {
            if (rwops != nullptr) {
                if (rwops->hidden.unknown.data1 != nullptr) {
                    delete get_file(rwops);
                    rwops->hidden.unknown.data1 = nullptr;
                }
                delete rwops;
            }
            return 0;
        }

        static lp3::sdl::RWops create_sdlrwops(std::unique_ptr<C> &&file) {
            lp3::sdl::RWops handle(new SDL_RWops);
            SDL_RWops *rwops = handle;
            rwops->type = SDL_RWOPS_UNKNOWN;
            rwops->close = close;
            rwops->read = read;
            rwops->seek = seek;
            rwops->size = size;
            rwops->write = write;
            rwops->hidden.unknown.data1 = file.release();
            return handle;
        }
    };

    // struct CompressedFileArgs {
    //     const std::string * file_name;
    //     TrackerList * tracker_arg tracker;
    //     lp3::sdl::RWops * zip_file;
    //     std::int64_t zip_file_starting_position;
    //     std::int64_t zip_compressed_max_size;
    // }

    // struct CompressedFile {
    //     constexpr int compressed_buffer_size = 2048;
    //     lp3::sdl::RWops & actual_file;
    //     std::int64_t zf_starting_position;
    //     std::int64_t zip_compressed_max_size;
    //     std::vector<char> compressed_buffer;
    //     std::int64_t zf_position;
    //     std::int64_t ub_position;
    //     OpenFileTracker tracker;
    //     z_stream stream;
    //     bool closed;

    //     CompressedFile(CompressedFileArgs args)
    //         : actual_file(*args.actual_file),
    //         zf_starting_position(args.zf_starting_position),
    //         zip_compressed_max_size(args.zip_compressed_max_size),
    //         compressed_buffer(args.compressed_buffer),
    //         zf_position(0),
    //         ub_position(0),
    //         tracker(*args.file_name, *args.tracker),
    //         closed(false)
    //     {
    //         read_amount = std::min(this->zip_compressed_max_size,
    //         compressed_buffer_size); if (1
    //           != this->actual_file.read(compressed_buffer.data(),
    //           read_amount)) { LP3_RSRC_LOG_ERROR("Error reading in compressed
    //           data for {}", file); throw std::runtime_error("error reading
    //           compressed file data");
    //       }

    //       stream.next_in = 0;
    //       stream.avail_in = 0;
    //       stream.next_out = 0;
    //       stream.avail_out = 0;
    //       stream.opaque = 0;
    //       stream.zalloc = nullptr;
    //       stream.zfree = nullptr;

    //       const int result = inflateInit(MY_Z_STREAM);
    //       if (Z_OK != result) {
    //           LP3_RSRC_LOG_ERROR("error initializing zlib stream for {}",
    //           *args.file_name); throw std::runtime_error("error init'ing zlib
    //           stream");
    //       }
    //     }

    //     ~CompressedFile() {
    //       end();
    //     }

    //     void end(bool can_throw) {
    //         if (closed) { return; }
    //         closed = true;
    //         if (Z_OK != inflateEnd(&stream)) {
    //             if (nullptr != stream.msg) {
    //               LP3_RSRC_LOG_ERROR("Zlib inflateEnd error: {}",
    //               stream.msg); if (can_throw) {
    //                   throw std::runtime_error("error shutting down zlib
    //                   stream");
    //               }
    //             }
    //         }
    //     }

    //     std::size_t read(void *dst, std::size_t object_size,
    //                      std::size_t object_count) {
    //       object_size
    //       stream.next_in = (Bytef *)this->compressed_buffer.data();
    //       stream.avail_in = this->zip_compressed_max_size -
    //       this->zf_position; stream.next_out = (Bytef *)dst; stream.avail_out
    //       = uf->buffer.size(); stream.zalloc = nullptr; stream.zfree =
    //       nullptr;

    //       const auto init_result = inflateInit2(&stream, -MAX_WBITS);
    //       if (Z_OK != init_result) {
    //           LP3_RSRC_LOG_ERROR("error initializing zlib stream for {}",
    //           file); throw std::runtime_error("error init'ing zlib stream");
    //       }
    //       const auto inflate_result = inflate(&stream, Z_FINISH);
    //       inflateEnd(&stream);
    //       if (inflate_result != Z_STREAM_END) {
    //           LP3_RSRC_LOG_ERROR("error ending zlib stream for {}", file);
    //           throw std::runtime_error("error ending zlib stream");
    //       }
    //     }
    // };
} // namespace

ZipFile::ZipFile(lp3::sdl::RWops &&zip_file)
    : actual_file(std::move(zip_file)), file_refs() {
    auto result = load_directory_info(this->actual_file);
    if (!result) {
        LP3_RSRC_LOG_ERROR("Error parsing zip file directory headers.");
        throw std::runtime_error("Bad file.");
    }
    for (auto *dir : result->directories) {
        FileRef ref;
        ref.file_name = dir->get_name();
        ref.offset_to_file
                = dir->relative_offset_of_local_header_from_start_of_first_disk;
        file_refs.push_back(ref);
    }
}

ZipFile::~ZipFile() {
    if (open_files.size() != 0) {
        LP3_RSRC_LOG_ERROR("ERROR! ZipFile is being closed but has still has "
                           "unclosed RWops.");
        for (const auto f : open_files) {
            LP3_RSRC_LOG_ERROR("\tFile: ", f);
        }
    }
}

/* Opens a resource for reading. */
sdl::RWops ZipFile::load(const char *file) {
    if (open_files.size() != 0) {
        LP3_RSRC_LOG_ERROR("ERROR! A new ZipFile is being loaded, but the file "
                           "is currently being used to read another zipfile.");
        for (const auto f : open_files) {
            LP3_RSRC_LOG_ERROR("\tFile: ", f);
        }
        throw std::runtime_error("Concurrent zip file read error!");
    }

    std::string search{file};
    auto result = std::find_if(
            file_refs.begin(), file_refs.end(),
            [file](const FileRef &ref) { return ref.file_name == file; });
    if (result == file_refs.end()) {
        LP3_RSRC_LOG_ERROR("{} not found in zipfile", file);
        throw std::runtime_error("file not found");
    }

    auto tell = this->actual_file.seek_from_beginning(result->offset_to_file);
    if (tell != result->offset_to_file) {
        LP3_RSRC_LOG_ERROR("{} offset not found in zipfile", file);
        throw std::runtime_error("could not find zipfile offset");
    }

    // Read into zip file header
    FileHeader header;
    if (!this->actual_file.read(header)) {
        LP3_RSRC_LOG_ERROR("Could not find header for {}", file);
        throw std::runtime_error("could not find header");
    }

    if (std::string_view(header.signature, 4)
        != std::string_view(file_header_signature, 4)) {
        LP3_RSRC_LOG_ERROR("Missing signature on file struct.");
        throw std::runtime_error("could not find header");
    }

    std::string file_name(header.file_name_length, ' ');
    this->actual_file.read(file_name.data(), header.file_name_length);

    LP3_RSRC_LOG_ERROR("Loading {}", file_name);

    this->actual_file.seek(header.extra_field_length);

    if (header.compression_method == Z_NO_COMPRESSION) {
        auto uf = std::make_unique<UncompressedFile>(
                file, this->open_files, header.compressed_file_size);
        if (1 != this->actual_file.read(uf->buffer.data(), uf->buffer.size())) {
            LP3_RSRC_LOG_ERROR("Error reading in (un)compressed data for {}",
                               file);
            throw std::runtime_error(
                    "Error reading in (un)compressed file data");
        }
        return SDL_RWopsFuncs<UncompressedFile>::create_sdlrwops(std::move(uf));
    } else if (header.compression_method != Z_DEFLATED) {
        LP3_RSRC_LOG_ERROR("Unsupported compression method");
        throw std::runtime_error("unsupported compression method");
    } else {
        auto cf = std::make_unique<CompressedFile>(
                file, this->open_files, this->actual_file,
                header.compressed_file_size, header.uncompressed_file_size);

        return SDL_RWopsFuncs<CompressedFile>::create_sdlrwops(std::move(cf));
    }
}

} // namespace lp3::rsrc
