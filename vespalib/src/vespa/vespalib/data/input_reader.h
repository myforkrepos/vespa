// Copyright 2017 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "memory.h"
#include <vespa/vespalib/stllike/string.h>
#include <vector>

namespace vespalib {

class Input;

/**
 * A utility wrapper for the Input interface that supplies us with an
 * inlined API for efficient buffer handling. Note that reading past
 * the end of the data is considered an error and will tag the reader
 * as failed with buffer underflow.
 **/
class InputReader
{
private:
    Input            &_input;
    Memory            _data;
    size_t            _pos;
    size_t            _bytes_evicted;
    vespalib::string  _error;
    std::vector<char> _space;

    const char *data() const { return (_data.data + _pos); }
    size_t size() const { return (_data.size - _pos); }

    size_t obtain_slow();
    Memory read_slow(size_t bytes);

public:
    explicit InputReader(Input &input)
        : _input(input), _data(), _pos(0), _bytes_evicted(0), _error(), _space() {}
    ~InputReader();

    bool failed() const { return !_error.empty(); }
    const vespalib::string &get_error_message() const { return _error; }
    size_t get_offset() const { return (_bytes_evicted + _pos); }

    void fail(const vespalib::string &msg);

    /**
     * Make sure we have more input data available.
     *
     * @return number of bytes available without requesting more from
     *         the underlying Input. Returns 0 if and only is there is
     *         no more input data available.
     **/
    size_t obtain() {
        if (__builtin_expect(_pos < _data.size, true)) {                    
            return size();
        }
        return obtain_slow();
    }

    /**
     * Read a single byte. Reading past the end of the input will
     * result in the reader failing with input underflow.
     *
     * @return the next input byte. Returns 0 if the reader has
     *         failed.
     **/
    char read() {
        if (__builtin_expect(obtain() > 0, true)) {
            return _data.data[_pos++];
        }
        return 0;
    }

    /**
     * Read a continous sequence of bytes. Bytes within an input chunk
     * will be referenced directly. Reads crossing chunk boundries
     * will result in a gathering copy into a temporary buffer owned
     * by the reader itself. Reading past the end of the input will
     * result in the reader failing with input underflow.
     *
     * @param bytes the number of bytes we want to read
     * @return Memory referencing the read bytes. Returns an empty
     *         Memory if the reader has failed.
     **/
    Memory read(size_t bytes) {
        if (__builtin_expect(obtain() >= bytes, true)) {
            Memory ret(data(), bytes);
            _pos += bytes;
            return ret;
        }
        return read_slow(bytes);
    }
};

} // namespace vespalib