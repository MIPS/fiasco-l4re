/*
 * l4_exceptions.h - TODO enter description
 * 
 * (c) 2011 Steffen Liebergeld <steffen@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
#pragma once

#include <stdio.h>

class L4_exception
{
public:
    L4_exception(const char *name, const char *func, const char* file, int line)
    : _name(name), _func(func), _file(file), _line(line)
    {}
    void print()
    {
        printf("Error %s occurred in function %s in file %s, line %d\n",
            _name, _func, _file, _line);
    }
private:
    const char *_name, *_func, *_file;
    int _line;
};

#define L4_EXCEPTION(name)\
    L4_exception(name, __func__, __FILE__, __LINE__)
#define L4_MALLOC_EXCEPTION L4_EXCEPTION("Malloc")
#define L4_CAPABILITY_EXCEPTION L4_EXCEPTION("Capability")
#define L4_PTHREAD_EXCEPTION L4_EXCEPTION("pthread")
#define L4_PANIC_EXCEPTION L4_EXCEPTION("panic")
