
#include <iostream>
#include <cstdio>
#include "exf/msf/msf.h"



class stl_io : public exf::io {
public:
    stl_io(const char* path)
        : m_file(fopen(path, "w+b"))
    {}
    ~stl_io() override {
        fflush(m_file);
        fclose(m_file);
    }

    size_t read(void* buffer, const size_t count, const size_t offset) override {
        if (m_flush_required) {
            m_flush_required = false;
            fflush(m_file);
        }
        std::fseek(m_file, offset, SEEK_SET);

        return std::fread(buffer, sizeof(uint8_t), count, m_file);
    }

    size_t write(const void* buffer, const size_t count, const size_t offset) override {
        if (m_flush_required) {
            m_flush_required = false;
            fflush(m_file);
        }
        std::fseek(m_file, offset, SEEK_SET);

        const auto written = std::fwrite(buffer, sizeof(uint8_t), count, m_file);
        if (written > 0) {
            m_flush_required = true;
        }

        return written;
    }

    size_t write_value(const uint8_t value, const size_t count, const size_t offset) override {
        if (m_flush_required) {
            m_flush_required = false;
            fflush(m_file);
        }
        std::fseek(m_file, offset, SEEK_SET);

        for (int i = 0; i < count; i++) {
            std::fwrite(&value, sizeof(uint8_t), 1, m_file);
        }

        if (count > 0) {
            m_flush_required = true;
        }

        return count;
    }

    std::FILE* m_file;
    bool m_flush_required = false;
};

class stl_mem_io : public exf::io {
public:
    stl_mem_io()
        : m_ptr(new uint8_t[40960])
    {}
    ~stl_mem_io() override { delete[] m_ptr;}

    size_t read(void* buffer, const size_t count, const size_t offset) override {
        __builtin_memcpy(buffer, m_ptr + offset, count);
        return count;
    }

    size_t write(const void* buffer, const size_t count, const size_t offset) override {
        __builtin_memcpy(m_ptr + offset, buffer, count);
        return count;
    }

    size_t write_value(const uint8_t value, const size_t count, const size_t offset) override {
        __builtin_memset(m_ptr + offset, value, count);
        return count;
    }

    uint8_t* m_ptr;
};

int main() {
    stl_io io("/home/tomtzook/development/projects/exe-formats/test.msf");
    //stl_io io("/home/tomtzook/development/projects/hype/cmake-build-debug-clang/bin/hype (another copy).pdb");
    //stl_mem_io io;
    exf::io_ref io_ref(&io);
    auto msf = exf::msf::create(io_ref);
    auto stream = msf.new_stream();
    stream.write("hello world", sizeof("hello world"));
    msf.save();

    return 0;
}
