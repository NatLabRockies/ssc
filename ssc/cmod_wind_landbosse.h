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



#ifndef SYSTEM_ADVISOR_MODEL_CMOD_WIND_LANDBOSSE_H
#define SYSTEM_ADVISOR_MODEL_CMOD_WIND_LANDBOSSE_H


#include <future>
#include <chrono>
#include <stdio.h>
#include <iostream>
#include <string>

#include "sscapi.h"
#include "vartab.h"
#include "core.h"

#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif

class cm_wind_landbosse : public compute_module {
private:
    std::string python_module_name;
    std::string python_exec_path;
    std::string python_run_cmd;

    void load_config();

public:
    cm_wind_landbosse();

    std::string call_python_module(const std::string& input_json);

#ifdef __WINDOWS__
	std::string call_python_module_windows(const std::string& input_json);
#endif

	void cleanOutputString(std::string& output_json);

    void exec() override;
};

#endif //SYSTEM_ADVISOR_MODEL_CMOD_WIND_LANDBOSSE_H
