/*
 * singleton.hpp - TODO enter description
 * 
 * (c) 2011 Janis Danisevskis <janis@sec.t-labs.tu-berlin.de>
 *
 * This file is part of the Karma VMM and distributed under the terms of the
 * GNU General Public License, version 2.
 *
 * Please see the file COPYING-GPL-2 for details.
 */
/*
 * singleton.hpp
 *
 *  Created on: 22.10.2010
 *      Author: janis
 */

#ifndef SINGLETON_HPP_
#define SINGLETON_HPP_

namespace util{

template <unsigned int N>
class Tnumber{
public:
    enum {value = N};
};

// default Singleton_traits
template <typename T>
class Singleton_traits{
public:
    typedef  T type;
    static type & create() {
        static type object;
        return object;
    }
};

template <typename T>
class Singleton{
private:
    typedef Singleton_traits<T> traits;
public:
    static typename traits::type & get(){
        static typename traits::type & object = traits::create();
        return object;
    }
};

}

#endif /* SINGLETON_HPP_ */
