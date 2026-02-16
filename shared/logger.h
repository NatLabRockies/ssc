/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#ifndef SAM_SIMULATION_CORE_LOGGER_H
#define SAM_SIMULATION_CORE_LOGGER_H

#include <ostream>
#include <iostream>

class nullstream : public std::ostream {
public:
    nullstream() : std::ostream(nullptr) {}
    nullstream(const nullstream &) : std::ostream(nullptr) {}
};

template <class T>
const nullstream &operator<<(nullstream &&os, const T &value) {
    return os;
}

static nullstream ns;

class logger {
private:
    std::ostream& _out_stream;

public:
    logger(): _out_stream(ns) {}
    explicit logger(std::ostream& stream): _out_stream(stream) {}

    explicit operator std::ostream& () {
        return _out_stream;
    }

    template<typename T>
    logger& operator<< (const T& data)
    {
        _out_stream << data;
        return *this;
    }
};

#endif //SAM_SIMULATION_CORE_LOGGER_H
