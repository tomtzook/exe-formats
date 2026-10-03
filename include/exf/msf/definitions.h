#pragma once

#include "exf/types.h"

namespace exf::msf {

static constexpr const char* signature = "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0\0";

#pragma pack(push, 1)

struct header {
    uint8_t signature[32];
    uint32_t page_size;
    uint32_t active_free_page_map;
    uint32_t amount_of_pages;
    uint32_t stream_table_stream_size;
    uint32_t unused;
    uint32_t page_list_of_stream_table_stream_page_list;
};

#pragma pack(pop)

}
