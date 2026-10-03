#pragma once

#include "exf/io.h"

namespace exf::msf {

struct stream_state {
    static constexpr size_t MAX_PAGES = 0x100;

    uint32_t pages[MAX_PAGES];
    uint32_t page_count;
    size_t current_size;
};

class stream_controller {
public:
    stream_controller();
    ~stream_controller();

    [[nodiscard]] size_t get_stream_count() const;

    stream_state* create_stream();
    stream_state* get_stream_table();
    stream_state* get_stream(uint32_t stream_index);

    bool add_page(stream_state* stream, uint32_t page_index);
    bool remove_page(stream_state* stream, uint32_t page_index);

private:
    stream_state m_stream_table;
    stream_state* m_streams;
    size_t m_stream_amount;
};

class layout_controller {
public:
    explicit layout_controller(const io_ref& io);
    layout_controller(const layout_controller&) = delete;
    layout_controller(layout_controller&&) = default;

    layout_controller& operator=(const layout_controller&) = delete;
    layout_controller& operator=(layout_controller&&) = default;

    uint32_t create_new_stream();
    size_t read(uint32_t stream_index, size_t offset, void* buffer, size_t size);
    size_t write(uint32_t stream_index, size_t offset, const void* buffer, size_t size);
    // size_t remove(uint32_t stream_index, size_t offset, size_t size); // todo: delete data from stream

    void init(size_t page_size, uint8_t free_page_map_index);
    void load();
    void save();

private:
    void write_header();
    void write_stream_table();

    size_t read(const stream_state* stream, size_t offset, void* buffer, size_t size);
    size_t write(stream_state* stream, size_t offset, const void* buffer, size_t size);

    bool acquire_new_pages_for_stream(stream_state* stream, size_t page_count);
    bool mark_page_used(uint32_t page_index);
    bool create_page_at_index(uint32_t page_index);

    io_ref m_io;
    stream_controller m_stream_controller;

    size_t m_page_size;
    size_t m_page_count;
    uint8_t m_free_page_map_index;
    uint32_t m_stream_table_stream_pages_page_index;
};

}
