#include "exf/msf/definitions.h"
#include "exf/meta.h"
#include "exf/msf/layout_controller.h"

namespace exf::msf {

constexpr size_t page_count(const size_t size, const size_t page_size) {
    const auto count = size / page_size;
    if (size % page_size > 0) {
        return count + 1;
    }
    return count;
}

int64_t find_first_free_bit(const void* buffer, const size_t size) {
    auto ptr = static_cast<const uint8_t*>(buffer);
    auto ptr64 = static_cast<const uint64_t*>(buffer);
    const auto qword_count = size / sizeof(uint64_t);

    for (size_t i = 0; i < qword_count; i++) {
        const auto value = ptr64[i];
        if (value == 0) {
            return i * sizeof(uint64_t) * 8;
        }

        if (const auto trailing_index = __builtin_ctzll(~value); trailing_index > 0) {
            return (i * sizeof(uint64_t) * 8) + trailing_index;
        }
    }

    const auto start_offset = qword_count * sizeof(uint64_t);
    for (size_t i = start_offset; i < size; i++) {
        const auto value = ptr[i];
        if (value == 0) {
            return i * 8;
        }

        if (const auto trailing_index = __builtin_ctz(~value); trailing_index > 0) {
            return (i * 8) + trailing_index;
        }
    }

    return -1;
}

bool set_bit(void* buffer, const size_t bit_index) {
    auto ptr = static_cast<uint8_t*>(buffer) + (bit_index / 8);
    const auto new_val = *ptr | (1 << (bit_index % 8));
    if (*ptr == new_val) {
        return false;
    }

    *ptr = new_val;
    return true;
}

stream_controller::stream_controller()
    : m_stream_table({{0}, 0, 0})
    , m_streams(nullptr)
    , m_stream_amount(0)
{}

stream_controller::~stream_controller() {
    if (m_streams != nullptr) {
        delete[] m_streams;
    }
}

size_t stream_controller::get_stream_count() const {
    return m_stream_amount;
}

stream_state* stream_controller::create_stream() {
    if (m_streams == nullptr) {
        m_streams = new stream_state[1];
    } else {
        auto* new_streams = new stream_state[m_stream_amount + 1];
        __builtin_memcpy(new_streams, m_streams, sizeof(stream_state) * m_stream_amount);
        delete[] m_streams;
        m_streams = new_streams;
    }

    auto* stream = &m_streams[m_stream_amount++];
    stream->current_size = 0;
    stream->page_count = 0;
    __builtin_memset(stream->pages, 0, sizeof(stream->pages));

    return stream;
}

stream_state* stream_controller::get_stream_table() {
    return &m_stream_table;
}

stream_state* stream_controller::get_stream(const uint32_t stream_index) {
    if (m_streams == nullptr || stream_index >= m_stream_amount) {
        return nullptr;
    }

    return &m_streams[stream_index];
}

bool stream_controller::add_page(stream_state* stream, const uint32_t page_index) {
    if (stream == nullptr) {
        return false;
    }
    if (stream->page_count + 1 >= stream_state::MAX_PAGES) {
        return false;
    }

    stream->pages[stream->page_count++] = page_index;
    return true;
}

bool stream_controller::remove_page(stream_state* stream, const uint32_t page_index) {
    if (stream == nullptr) {
        return false;
    }

    bool found = false;
    for (uint32_t i = 0; i < stream->page_count; i++) {
        if (stream->pages[i] == page_index) {
            stream->pages[i] = 0;
            found = true;
        }

        if (found) {
            stream->pages[i] = stream->pages[i+1];
        }
    }

    if (!found) {
        return false;
    }

    stream->page_count--;
    return true;
}

layout_controller::layout_controller(const io_ref& io)
    : m_io(io)
    , m_stream_controller()
    , m_page_size()
    , m_page_count()
    , m_free_page_map_index()
{}

uint32_t layout_controller::create_new_stream() {
    m_stream_controller.create_stream();
    return m_stream_controller.get_stream_count() - 1;
}

size_t layout_controller::read(const uint32_t stream_index, const size_t offset, void* buffer, const size_t size) {
    auto* stream = m_stream_controller.get_stream(stream_index);
    if (stream == nullptr) {
        return 0; // todo: return error
    }

    return read(stream, offset, buffer, size);
}

size_t layout_controller::write(const uint32_t stream_index, const size_t offset, const void* buffer, const size_t size) {
    auto* stream = m_stream_controller.get_stream(stream_index);
    if (stream == nullptr) {
        return 0; // todo: return error
    }

    return write(stream, offset, buffer, size);
}

void layout_controller::init(const size_t page_size, const uint8_t free_page_map_index) {
    m_page_size = page_size;
    m_free_page_map_index = free_page_map_index;
    m_stream_table_stream_pages_page_index = 0;

    // write first 3 pages for header and free page maps
    // todo: handle error
    m_io.write_value(0, m_page_size * 3, 0);
    m_page_count = 3;

    // create page for page list of stream table stream
    if (!mark_page_used(3)) {
        return; // todo: error
    }
    if (!create_page_at_index(3)) {
        return; // todo: error
    }

    m_stream_table_stream_pages_page_index = 3;
    m_page_count++;

    write_header();
}

void layout_controller::load() {
    header header{};
    // todo: handle read error
    m_io.read(&header, sizeof(header), 0);

    m_page_size = header.page_size;
    m_page_count = header.amount_of_pages;
    m_free_page_map_index = header.active_free_page_map;
    m_stream_table_stream_pages_page_index = header.page_list_of_stream_table_stream_page_list;

    auto* stream_table = m_stream_controller.get_stream_table();
    stream_table->current_size = header.stream_table_stream_size;
    stream_table->page_count = page_count(header.stream_table_stream_size, m_page_size);

    // todo: handle read error
    m_io.read(stream_table->pages, sizeof(uint32_t) * stream_table->page_count, header.page_list_of_stream_table_stream_page_list * m_page_size);

    // stream table stream
    //  uint32 amount_of_streams
    //  uint32 stream_sizes[amount_of_streams]
    //  uint32 stream_one_pages[]
    //  uint32 stream_two_pages[]
    //  ...

    uint32_t amount_of_streams{};
    // todo: handle read error
    read(stream_table, 0, &amount_of_streams, sizeof(uint32_t));

    if (amount_of_streams >= 0x1000) {
        // todo: error
        return;
    }

    uint32_t stream_sizes[0x1000]{};
    // todo: handle read error
    read(stream_table, sizeof(uint32_t), stream_sizes, sizeof(uint32_t) * amount_of_streams);

    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) * amount_of_streams;
    for (int i = 0; i < amount_of_streams; i++) {
        auto* stream = m_stream_controller.create_stream();
        stream->current_size = stream_sizes[i];
        stream->page_count = page_count(stream_sizes[i], m_page_size);

        const auto size_to_read = stream->page_count * sizeof(uint32_t);
        // todo: handle read error
        read(stream_table, offset, stream->pages, size_to_read);
        offset += size_to_read;
    }
}

void layout_controller::save() {
    write_stream_table();
    // write header second because writing stream table can change it
    write_header();
}

void layout_controller::write_header() {
    const auto* stream_table = m_stream_controller.get_stream_table();

    header header{};
    __builtin_memcpy(header.signature, signature, min(__builtin_strlen(signature), 32ul));
    header.page_size = m_page_size;
    header.active_free_page_map = m_free_page_map_index;
    header.amount_of_pages = m_page_count;
    header.stream_table_stream_size = stream_table->current_size;
    header.page_list_of_stream_table_stream_page_list = m_stream_table_stream_pages_page_index;

    // todo: handle error
    m_io.write(&header, sizeof(header), 0);

    if (stream_table->page_count > 0) {
        // todo: handle error
        m_io.write(stream_table->pages, sizeof(uint32_t) * stream_table->page_count, m_stream_table_stream_pages_page_index * m_page_size);
    }
}

void layout_controller::write_stream_table() {
    auto* stream_table = m_stream_controller.get_stream_table();
    const auto stream_count = m_stream_controller.get_stream_count();
    // todo: handle write error
    write(stream_table, 0, &stream_count, sizeof(uint32_t));

    size_t stream_table_offset = sizeof(uint32_t);
    for (int i = 0; i < stream_count; i++) {
        auto* stream = m_stream_controller.get_stream(i);
        // todo: handle write error
        write(stream_table, stream_table_offset, &stream->current_size, sizeof(uint32_t));
        stream_table_offset += sizeof(uint32_t);
    }

    for (int i = 0; i < stream_count; i++) {
        auto* stream = m_stream_controller.get_stream(i);
        if (stream == nullptr) {
            return; // todo: return error
        }

        if (stream->page_count > 0) {
            // todo: handle write error
            write(stream_table, stream_table_offset, &stream->pages, sizeof(uint32_t) * stream->page_count);
            stream_table_offset += sizeof(uint32_t) * stream->page_count;
        }
    }
}

size_t layout_controller::read(const stream_state* stream, const size_t offset, void* buffer, const size_t size) {
    if (offset + size >= stream->current_size) {
        return 0; // todo: return error
    }

    auto* buffer_ptr = static_cast<uint8_t*>(buffer);
    size_t current_offset = offset;
    const auto max_offset = offset + size;
    while (current_offset < max_offset) {
        const auto current_page_index = current_offset / m_page_size;
        const auto current_page_offset = current_offset % m_page_size;
        const auto size_to_read = min(max_offset - current_offset, m_page_size - current_page_offset);
        const auto read_offset = stream->pages[current_page_index] * m_page_size + current_page_offset;

        if (const auto read = m_io.read(buffer_ptr, size_to_read, read_offset);
            read < size_to_read) {
            return 0; // todo: handle error
        }

        current_offset += size_to_read;
        buffer_ptr += size_to_read;
    }

    return current_offset - offset;
}

size_t layout_controller::write(stream_state* stream, const size_t offset, const void* buffer, const size_t size) {
    if (const auto end_page_index = (offset + size) / m_page_size;
        end_page_index >= stream->page_count) {
        if (!acquire_new_pages_for_stream(stream, stream->page_count - end_page_index + 1)) {
            return 0; // todo: error
        }
    }

    auto* buffer_ptr = static_cast<const uint8_t*>(buffer);
    size_t current_offset = offset;
    const auto max_offset = offset + size;
    while (current_offset < max_offset) {
        const auto current_page_index = current_offset / m_page_size;
        const auto current_page_offset = current_offset % m_page_size;
        const auto size_to_write = min(max_offset - current_offset, m_page_size - current_page_offset);
        const auto write_offset = stream->pages[current_page_index] * m_page_size + current_page_offset;

        if (const auto written = m_io.write(buffer_ptr, size_to_write, write_offset);
            written < 1) {
            return 0; // todo: handle error
        }

        current_offset += size_to_write;
        buffer_ptr += size_to_write;
    }

    if (current_offset > stream->current_size) {
        stream->current_size = current_offset;
    }

    return current_offset - offset;
}

bool layout_controller::acquire_new_pages_for_stream(stream_state* stream, size_t page_count) {
    char page_buffer[0x1000]; // todo: assert with m_page_size

    size_t current_page_map_index = m_free_page_map_index;
    while (page_count > 0) {
        if (const auto read = m_io.read(page_buffer, m_page_size, current_page_map_index * m_page_size);
            read < 1) {
            return false; // todo: handle error
        }

        const auto bit = find_first_free_bit(page_buffer, m_page_size);
        if (bit < 0) {
            current_page_map_index += (m_page_size * 8); // jump to next free page map
            if (current_page_map_index >= m_page_count) {
                // we need to add more pages
                const auto page_index = current_page_map_index + 3 - m_free_page_map_index;
                if (!create_page_at_index(page_index)) {
                    return false; // todo: error
                }

                // we know about the free page because we just added one
                if (!m_stream_controller.add_page(stream, page_index)) {
                    return false; // todo: error
                }
            } else {
                continue;
            }
        } else {
            set_bit(page_buffer, bit);

            if (const auto written = m_io.write(page_buffer, 1, (current_page_map_index * m_page_size) + bit / 8);
                written < 1) {
                return false; // todo: handle error
            };

            const auto page_index = current_page_map_index + 3 + bit - m_free_page_map_index;
            if (!m_stream_controller.add_page(stream, page_index)) {
                return false; // todo: error
            }

            if (page_index >= m_page_count) {
                // need to add this new page
                if (!create_page_at_index(page_index)) {
                    return false; // todo: error
                }
            }
        }

        page_count--;
    }

    return true;
}

bool layout_controller::mark_page_used(const uint32_t page_index) {
    const auto correct_page_index = page_index - 3;
    const auto free_page_map_index = m_free_page_map_index + correct_page_index / (m_page_size * 8);
    const auto page_index_in_map = correct_page_index % (m_page_size * 8);
    const auto byte_offset_in_map = (free_page_map_index * m_page_size) + page_index_in_map / 8;

    char page_buffer[1]; // todo: assert with m_page_size
    if (const auto read = m_io.read(page_buffer, 1, byte_offset_in_map);
        read < 1) {
        return false; // todo: handle error
    }
    if (!set_bit(page_buffer, page_index_in_map % 8)) {
        return false; // taken
    }
    if (const auto written = m_io.write(page_buffer, 1, byte_offset_in_map);
        written < 1) {
        return false; // todo: handle error
    };

    return true;
}

bool layout_controller::create_page_at_index(const uint32_t page_index) {
    const auto missing_pages = m_page_count - page_index + 1;
    if (const auto written = m_io.write_value(0, m_page_size * missing_pages, m_page_size * m_page_count);
        written < 1) {
        return false; // todo: handle error
    }

    m_page_count += missing_pages;
    return true;
}

}
