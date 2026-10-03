#pragma once

#include "exf/types.h"

namespace exf {

class io {
public:
    virtual ~io() = default;

    virtual size_t read(void* buffer, size_t count, size_t offset) = 0;
    virtual size_t write(const void* buffer, size_t count, size_t offset) = 0;
    virtual size_t write_value(uint8_t value, size_t count, size_t offset) = 0;
};

class io_ref {
public:
    io_ref(io* io) : m_io(io) {}
    io_ref(const io_ref&) = default;
    io_ref(io_ref&&) = default;

    io_ref& operator=(const io_ref&) = default;
    io_ref& operator=(io_ref&&) = default;

    size_t read(void* buffer, const size_t count, const size_t offset) { return m_io->read(buffer, count, offset); }
    size_t write(const void* buffer, const size_t count, const size_t offset) { return m_io->write(buffer, count, offset); }
    size_t write_value(const uint8_t value, const size_t count, const size_t offset) { return m_io->write_value(value, count, offset); }

private:
    io* m_io;
};

}
