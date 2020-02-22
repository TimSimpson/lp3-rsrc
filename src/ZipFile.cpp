#include <lp3/rsrc/ZipFile.hpp>

#include <cstring>
#include <iostream>
#include <lp3/rsrc/zip_utils.hpp>
#include <lp3/sdl.hpp>
#include <zlib.h>

#define GET_UNCOMPRESSED_FILE                                                  \
    reinterpret_cast<UncompressedFile *>(rwops->hidden)

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

    UncompressedFile *get_file(SDL_RWops *rwops) {
        return (UncompressedFile *)rwops->hidden.unknown.data1;
    }

    std::int64_t size(SDL_RWops *rwops) {
        SDL_assert(rwops != nullptr);
        return get_file(rwops)->size();
    }

    std::int64_t seek(SDL_RWops *rwops, const long amount, const int whence) {
        SDL_assert(rwops != nullptr);
        return get_file(rwops)->seek(amount, whence);
    }

    std::size_t read(SDL_RWops *rwops, void *dst, std::size_t object_size,
                     std::size_t object_count) {
        SDL_assert(rwops != nullptr);
        return get_file(rwops)->read(dst, object_size, object_count);
    }

    std::size_t write(SDL_RWops *rwops, const void *dst,
                      std::size_t object_count, std::size_t object_size) {
        SDL_assert(rwops != nullptr);
        LP3_RSRC_LOG_ERROR("An attempt was made to write to a zip file.");
        return 0;
    }

    int close(SDL_RWops *rwops) {
        if (rwops != nullptr) {
            if (rwops->hidden.unknown.data1 != nullptr) {
                delete get_file(rwops);
                rwops->hidden.unknown.data1 = nullptr;
            }
            delete rwops;
        }
        return 0;
    }

    lp3::sdl::RWops create_sdlrwops(std::unique_ptr<UncompressedFile> &&file) {
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

/* Opens a resource for reading. */
sdl::RWops ZipFile::load(const char *file) {
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
        return create_sdlrwops(std::move(uf));
    } else if (header.compression_method != Z_DEFLATED) {
        LP3_RSRC_LOG_ERROR("Unsupported compression method");
        throw std::runtime_error("unsupported compression method");
    } else {
        auto uf = std::make_unique<UncompressedFile>(
            file, this->open_files, header.uncompressed_file_size);

        std::vector<char> cb(header.compressed_file_size);
        if (1
            != this->actual_file.read(cb.data(), header.compressed_file_size)) {
            LP3_RSRC_LOG_ERROR("Error reading in compressed data for {}", file);
            throw std::runtime_error("error reading compressed file data");
        }

        z_stream stream;
        stream.next_in = (Bytef *)cb.data();
        stream.avail_in = header.compressed_file_size;
        stream.next_out = (Bytef *)uf->buffer.data();
        stream.avail_out = uf->buffer.size();
        stream.zalloc = nullptr;
        stream.zfree = nullptr;

        const auto init_result = inflateInit2(&stream, -MAX_WBITS);
        if (Z_OK != init_result) {
            LP3_RSRC_LOG_ERROR("error initializing zlib stream for {}", file);
            throw std::runtime_error("error init'ing zlib stream");
        }
        const auto inflate_result = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);
        if (inflate_result != Z_STREAM_END) {
            LP3_RSRC_LOG_ERROR("error ending zlib stream for {}", file);
            throw std::runtime_error("error ending zlib stream");
        }

        return create_sdlrwops(std::move(uf));
    }
}

} // namespace lp3::rsrc
