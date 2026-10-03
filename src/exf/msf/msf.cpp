
#include "exf/meta.h"
#include "exf/msf/msf.h"

namespace exf::msf {

msf::msf(io_ref io)
    : m_layout_controller(move(io)) {
    m_layout_controller.load();
}

msf::msf(io_ref io, const size_t page_size)
    : m_layout_controller(move(io)) {
    m_layout_controller.init(page_size, 1);
}

msf::~msf() {
    // m_layout_controller.save(); // todo: save?
}

stream msf::new_stream() {
    const auto stream_index = m_layout_controller.create_new_stream();
    return stream{m_layout_controller, stream_index};
}

stream msf::get_stream(const uint32_t index) {
    return stream{m_layout_controller, index};
}

void msf::save() {
    m_layout_controller.save();
}

msf create(io_ref io) {
    return msf{move(io), 0x1000};
}

msf open(io_ref io) {
    return msf{move(io)};
}

}
