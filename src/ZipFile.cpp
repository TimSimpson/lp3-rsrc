#include <lp3/rsrc/ZipFile.hpp>

#include <cstring>
#include <lp3/sdl.hpp>
#include <lp3/rsrc/zip_utils.hpp>
#include <zlib.h>


#define GET_UNCOMPRESSED_FILE reinterpret_cast<UncompressedFile *>(rwops->hidden)

namespace lp3::rsrc {

    namespace {

        struct UncompressedFile {
            std::vector<char> buffer;
            std::int64_t position;

            UncompressedFile(std::size_t size)
            :   buffer(size), position(0)
            {}
        };

        UncompressedFile * get_file(SDL_RWops * rwops) {
            return (UncompressedFile *)rwops->hidden.unknown.data1;
        }

        std::int64_t size(SDL_RWops * rwops) {
            SDL_assert(rwops != nullptr);
            return get_file(rwops)->buffer.size();
        }

        std::int64_t seek(SDL_RWops * rwops, const int amount, const int whence)
        {
            SDL_assert(rwops != nullptr);
            const std::int64_t new_position =
                (RW_SEEK_SET == whence ? 0
                : RW_SEEK_END == whence ? get_file(rwops)->buffer.size()
                : get_file(rwops)->position) + amount;
            if (new_position < 0 || new_position >= get_file(rwops)->buffer.size()) {
                LP3_RSRC_LOG_ERROR("Bad seek; new_position would be {}/{}", new_position, get_file(rwops)->buffer.size());
                return -1;
            }
            get_file(rwops)->position = new_position;
            return new_position;
        }

        std::size_t read(SDL_RWops * rwops, void * dst, std::size_t object_size, std::size_t object_count) {
            const std::size_t full_size = object_size * object_count;
            const std::int64_t new_position = get_file(rwops)->position + full_size;
            if (new_position < 0 || new_position >=  get_file(rwops)->buffer.size()) {
                LP3_RSRC_LOG_ERROR("Bad read; would end up at {}/{}", new_position, get_file(rwops)->buffer.size());
                return 0;
            }
            std::memcpy(dst, get_file(rwops)->buffer.data() + get_file(rwops)->position, full_size);
            return object_count;
        }

        std::size_t write(SDL_RWops * rwops, const void * dst, std::size_t object_count, std::size_t object_size) {
            LP3_RSRC_LOG_ERROR("An attempt was made to write to a zip file.");
            return 0;
        }

        int close(SDL_RWops * rwops) {
            delete get_file(rwops);
            delete rwops;
            return 0;
        }

        lp3::sdl::RWops create_sdlrwops(std::unique_ptr<UncompressedFile> && file) {
            lp3::sdl::RWops handle(new SDL_RWops);
            SDL_RWops * rwops = handle;
            rwops->type = SDL_RWOPS_UNKNOWN;
            rwops->hidden.unknown.data1 = file.release();
            return handle;
        }
    }

    ZipFile::ZipFile(lp3::sdl::RWops && zip_file)
    :   actual_file(std::move(zip_file)),
        file_refs()
    {
        auto result = load_directory_info(this->actual_file);
        if (!result) {
            LP3_RSRC_LOG_ERROR("Error parsing zip file directory headers.");
            throw std::runtime_error("Bad file.");
        }
        for (auto * dir: result->directories) {
            FileRef ref;
            ref.file_name = dir->get_name();
            ref.offset_to_file = dir->relative_offset_of_local_header_from_start_of_first_disk;
            file_refs.push_back(ref);
        }
    }

    /* Opens a resource for reading. */
    sdl::RWops ZipFile::load(const char * file) {
        std::string search{file};
        auto result = std::find_if(file_refs.begin(), file_refs.end(), [file](const FileRef & ref) { return ref.file_name == file; });
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

        if (std::string_view(header.signature, 4) != std::string_view(file_header_signature, 4)) {
            LP3_RSRC_LOG_ERROR("Missing signature on file struct.");
            throw std::runtime_error("could not find header");
        }

        std::string file_name(header.file_name_length, ' ');
        this->actual_file.read(file_name.data(), header.file_name_length);

        LP3_RSRC_LOG_ERROR("Loading {}", file_name);

        this->actual_file.seek(header.extra_field_length);

        if (header.compression_method == Z_NO_COMPRESSION) {
            auto uf = std::make_unique<UncompressedFile>(header.compressed_file_size);
            this->actual_file.read(uf->buffer.data(), uf->buffer.size());
            return create_sdlrwops(std::move(uf));
        }

        if (header.compression_method != Z_DEFLATED) {
            LP3_RSRC_LOG_ERROR("Unsupported compression method");
            throw std::runtime_error("unsupported compression method");
        }

        LP3_RSRC_LOG_ERROR("TODO more stuff!")

        lp3::sdl::RWops handle(nullptr);
        return handle;
    }

}
