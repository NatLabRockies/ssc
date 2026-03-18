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


#include "core.h"

#include "fluid_properties.h"


static var_info _cm_vtab_csp_dsg_lf_ui[] = {

    /*   VARTYPE   DATATYPE         NAME               LABEL                                                     UNITS     META  GROUP REQUIRED_IF CONSTRAINTS         UI_HINTS*/
    { SSC_INPUT,   SSC_NUMBER,   "P_boil",           "Boiling pressure",                                      "bar",    "", "",  "*",  "", "" },
    { SSC_INPUT,   SSC_NUMBER,   "use_quality_or_subcooled", "0 = 2 phase outlet, 1 = subcooled",             "",       "", "",  "*",  "", "" },
    { SSC_INPUT,   SSC_NUMBER,   "deltaT_subcooled", "Subcooled temperature difference from saturation temp", "C",    "", "",  "*",  "", "" },

    { SSC_OUTPUT,  SSC_NUMBER,   "T_saturation",     "Saturation pressure",                          "C",      "", "",  "*",  "", "" },
    { SSC_OUTPUT,  SSC_NUMBER,   "T_hot_out_target", "Target outlet temperature",                    "C",      "", "",  "*",  "", "" },

    var_info_invalid };

class cm_csp_dsg_lf_ui : public compute_module
{
public:

    cm_csp_dsg_lf_ui()
    {
        add_var_info(_cm_vtab_csp_dsg_lf_ui);
    }

    void exec() override
    {
        fluid_state wp;
        auto water_props = C_fluid_properties::create_fluid_properties(E_fluid_type::WATER);


        double P_boil = as_double("P_boil") * 100.0;    //[kPa]
        bool is_subcooled = as_boolean("use_quality_or_subcooled");
        double deltaT_subcooled = as_double("deltaT_subcooled");

        int wp_code = water_props->PQ(P_boil, 0.0, &wp);

        double T_sat = std::numeric_limits<double>::quiet_NaN();
        if (wp_code != 0) {
            T_sat = -987.6;
        }
        else {
            T_sat = wp.temp - 273.15;   //[C]
        }

        double T_hot_out_target = T_sat;    //[C]
        if (is_subcooled) {
            T_hot_out_target = T_sat - deltaT_subcooled;    //[C]
        }

        assign("T_saturation", (ssc_number_t)T_sat);                //[C]
        assign("T_hot_out_target", (ssc_number_t)T_hot_out_target); //[C]

        return;
    }
};

DEFINE_MODULE_ENTRY(csp_dsg_lf_ui, "Calculates values for all calculated values on UI TES page(s)", 0)

