#pragma once

#include "exf/types.h"
#include "exf/msf/layout_controller.h"

namespace exf::msf {

class stream {
public:
    stream(layout_controller& layout_controller, uint32_t index);

    void seek(size_t offset);

    size_t read(void* buffer, size_t count);
    size_t write(const void* buffer, size_t count);

private:
    layout_controller& m_layout_controller;
    uint32_t m_index;
    size_t m_offset;
};

}
