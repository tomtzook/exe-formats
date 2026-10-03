#pragma once

#include "exf/types.h"
#include "exf/io.h"
#include "exf/msf/layout_controller.h"
#include "exf/msf/stream.h"

namespace exf::msf {

class msf {
public:
    explicit msf(io_ref io);
    msf(io_ref io, size_t page_size);
    ~msf();

    stream new_stream();
    stream get_stream(uint32_t index);

    void save();

private:
    layout_controller m_layout_controller;
};

msf create(io_ref io);
msf open(io_ref io);

}
