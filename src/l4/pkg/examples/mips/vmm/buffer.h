/*
 * buffer.h - TODO enter description
 * 
 * (c) 2011 Steffen Liebergeld <steffen@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
#pragma once

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <pthread-l4.h>

template < unsigned SIZE >
class Buffer
{
    public:

        char *ptr() const { return _buf; }
        size_t size() const { return SIZE; }
        size_t count() const { return _count; }
        bool valid() { return _buf; }

        Buffer()
            : _pos_in(0), _pos_out(0), _count(0)
        {
            _buf = (typeof(_buf))malloc(SIZE);
            if (_buf)
                memset(_buf, 0, SIZE);
            pthread_mutex_init(&mutex, 0);
        }

        ~Buffer()
        {
            free(_buf);
        }

        bool put(const char c)
        {
            pthread_mutex_lock(&mutex);
            assert(valid());

            if (_count == SIZE) {
                assert(_pos_in == _pos_out);
                _buf[_pos_in] = c;
                _pos_in = (_pos_in + 1) % SIZE;
                _pos_out = _pos_in;
                pthread_mutex_unlock(&mutex);
                return true;
            }

            _buf[_pos_in] = c;
            _pos_in = (_pos_in + 1) % SIZE;
            _count++;
            pthread_mutex_unlock(&mutex);
            return true;
        }

        bool get(char *c)
        {
            if (_count == 0)
                return false;

            pthread_mutex_lock(&mutex);
            *c = _buf[_pos_out];

            _pos_out = (_pos_out + 1) % SIZE;
            _count--;

            pthread_mutex_unlock(&mutex);
            return true;
        }

        void flush()
        {
            pthread_mutex_lock(&mutex);
            _pos_in = 0;
            _pos_out = 0;
            _count = 0;
            pthread_mutex_unlock(&mutex);
        }

    private:
        char  *_buf;
        volatile size_t _pos_in;
        volatile size_t _pos_out;
        size_t _count;
        pthread_mutex_t mutex;
};

