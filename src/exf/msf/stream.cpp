
#include "exf/msf/stream.h"

namespace exf::msf {

stream::stream(layout_controller& layout_controller, const uint32_t index)
    : m_layout_controller(layout_controller)
    , m_index(index)
    , m_offset(0)
{}

void stream::seek(const size_t offset) {
    // todo: limit by size of file
    m_offset = offset;
}

size_t stream::read(void* buffer, const size_t count) {
    const auto read = m_layout_controller.read(m_index, m_offset, buffer, count);
    if (read > 0) {
        m_offset += read;
    }

    return read;
}

size_t stream::write(const void* buffer, const size_t count) {
    const auto written = m_layout_controller.write(m_index, m_offset, buffer, count);
    if (written > 0) {
        m_offset += written;
    }

    return written;
}

}
