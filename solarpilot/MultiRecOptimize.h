/*
BSD 3-Clause License

Copyright (c) Alliance for Sustainable Energy, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE
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

#ifndef _MULTI_REC_
#define _MULTI_REC_

#ifdef _WINDOWS
	#define LPWINAPP
#endif 
#include <vector>
#include <string>
#include <sstream>

class SolarField;
class simulation_info;

struct multi_rec_opt_helper
{
    enum RESULT_STATUS { RS_NOSIM=-1, RS_OPTIMAL=0, RS_SUBOPTIMAL, RS_INFEASIBLE, RS_TIMED_OUT, RS_UNKNOWN_ERROR};

    bool is_abort_flag;
    bool is_performance;
    bool is_field_assigned;
    int result_status;
    double objective;
    Hvector included_heliostats;
    double timeout_sec;
    simulation_info *sim_info;
    int solver_status;
    double last_report_time;
    double sim_report_step;
    std::string problem_name;

    multi_rec_opt_helper()
    {
        is_abort_flag = false;
        is_performance = false;
        is_field_assigned = false;
        result_status = RESULT_STATUS::RS_NOSIM;
        timeout_sec = 10000;
        objective = 0.;
        sim_info = 0;
        solver_status = std::numeric_limits<int>::quiet_NaN();
        last_report_time = -1.;
        sim_report_step = 1.;
        problem_name = "Optimization problem";
    };

    int run(SolarField *SF);
};



#endif
