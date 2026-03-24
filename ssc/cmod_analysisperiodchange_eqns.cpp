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


#include <exception>
#include <algorithm>
#include <math.h>

#include "vartab.h"
#include "../shared/lib_util.h"
#include "../shared/lib_time.h"

#include "cmod_analysisperiodchange_eqns.h"
//#pragma warning(disable: 4297)  // ignore warning: 'function assumed not to throw an exception but does'

bool analysisperiodchange(ssc_data_t data)
{
	std::string error = "";
    std::string warning = "";
    bool analysisperiodchange_success = false;
	auto vt = static_cast<var_table*>(data);
    if (!vt) {
        return false;
    }
	try {

		ssc_number_t analysis_period, analysis_period_old;
        util::matrix_t<ssc_number_t> mp_energy_market_revenue, mp_ancserv1_revenue, mp_ancserv2_revenue, mp_ancserv3_revenue, mp_ancserv4_revenue;
        util::matrix_t<ssc_number_t> mp_energy_market_revenue_single, mp_ancserv1_revenue_single, mp_ancserv2_revenue_single, mp_ancserv3_revenue_single, mp_ancserv4_revenue_single;

        vt_get_number(vt, "analysis_period", &analysis_period);
        vt_get_number(vt, "analysis_period_old", &analysis_period_old);

        vt_get_matrix(vt, "mp_energy_market_revenue_single", mp_energy_market_revenue_single);
        vt_get_matrix(vt, "mp_ancserv1_revenue_single", mp_ancserv1_revenue_single);
        vt_get_matrix(vt, "mp_ancserv2_revenue_single", mp_ancserv2_revenue_single);
        vt_get_matrix(vt, "mp_ancserv3_revenue_single", mp_ancserv3_revenue_single);
        vt_get_matrix(vt, "mp_ancserv4_revenue_single", mp_ancserv4_revenue_single);

        vt_get_matrix(vt, "mp_energy_market_revenue", mp_energy_market_revenue);
        vt_get_matrix(vt, "mp_ancserv1_revenue", mp_ancserv1_revenue);
        vt_get_matrix(vt, "mp_ancserv2_revenue", mp_ancserv2_revenue);
        vt_get_matrix(vt, "mp_ancserv3_revenue", mp_ancserv3_revenue);
        vt_get_matrix(vt, "mp_ancserv4_revenue", mp_ancserv4_revenue);

        // resize - truncate or fill with zeros if analysis period has changed - SAM issue 994
        if (analysis_period != analysis_period_old) {
            // for each revenue stream - determine current mode (subhourly, hourly, daily, weekly, monthly, annual or single value), resize to new analysis period
            for (util::matrix_t<ssc_number_t>* pmat : { &mp_energy_market_revenue_single, &mp_ancserv1_revenue_single, &mp_ancserv2_revenue_single, &mp_ancserv3_revenue_single, &mp_ancserv4_revenue_single,
                &mp_energy_market_revenue, &mp_ancserv1_revenue, &mp_ancserv2_revenue, &mp_ancserv3_revenue, &mp_ancserv4_revenue }) {
                size_t  oldSize = pmat->nrows();
                size_t newSize = 1;
                if (oldSize == 1) {
                    newSize = 1;
                }
                else if (oldSize == analysis_period_old) {
                    newSize = (size_t)analysis_period;
                }
                else if (oldSize == (analysis_period_old * 12)) {
                    newSize = (size_t)analysis_period * 12;
                }
                else if (oldSize == (analysis_period_old * 52)) {
                    newSize = (size_t)analysis_period * 52;
                }
                else if (oldSize == (analysis_period_old * 365)) {
                    newSize = (size_t)analysis_period * 365;
                }
                else if (oldSize == (analysis_period_old * 8760)) {
                    newSize = (size_t)analysis_period * 8760;
                }
                else {
                    size_t steps_per_hour = oldSize / (size_t)analysis_period_old / 8760;
                    newSize = steps_per_hour * 8760 * (size_t)analysis_period;
                }
                pmat->resize_preserve(newSize, pmat->ncols(), 0.0);
            }

            vt->assign("mp_energy_market_revenue", var_data(mp_energy_market_revenue));
            vt->assign("mp_ancserv1_revenue", var_data(mp_ancserv1_revenue));
            vt->assign("mp_ancserv2_revenue", var_data(mp_ancserv2_revenue));
            vt->assign("mp_ancserv3_revenue", var_data(mp_ancserv3_revenue));
            vt->assign("mp_ancserv4_revenue", var_data(mp_ancserv4_revenue));

            vt->assign("mp_energy_market_revenue_single", var_data(mp_energy_market_revenue_single));
            vt->assign("mp_ancserv1_revenue_single", var_data(mp_ancserv1_revenue_single));
            vt->assign("mp_ancserv2_revenue_single", var_data(mp_ancserv2_revenue_single));
            vt->assign("mp_ancserv3_revenue_single", var_data(mp_ancserv3_revenue_single));
            vt->assign("mp_ancserv4_revenue_single", var_data(mp_ancserv4_revenue_single));



        }
    }
    catch (std::exception& e)
    {
	    error = std::string(e.what());
	    return false;
    }
    analysisperiodchange_success = (error == "");
    vt->assign("analysisperiodchange_check", var_data(analysisperiodchange_success));
    vt->assign("analysisperiodchange_error", var_data(error));
    vt->assign("analysisperiodchange_warning", var_data(warning));
    return true;
}



