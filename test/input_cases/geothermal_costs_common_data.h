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



#ifndef _GEOTHERMAL_COSTS_COMMON_DATA_H_
#define _GEOTHERMAL_COSTS_COMMON_DATA_H_

#include <stdio.h>
#include "code_generator_utilities.h"

void geothermal_costs_default(ssc_data_t &data) {

	ssc_data_set_number(data, "gross_output", 5.435705);
    ssc_data_set_number(data, "gross_cost_output", 5400);
	ssc_data_set_number(data, "design_temp", 200.0);
    ssc_data_set_number(data, "dt_prod_well", 0.0);
	ssc_data_set_number(data, "eff_secondlaw", 0.405130);
	ssc_data_set_number(data, "conversion_type", 0);
	ssc_data_set_number(data, "qRejectTotal", 178754.609375);
	ssc_data_set_number(data, "qCondenser", 165823.921875);
	ssc_data_set_number(data, "v_stage_1", 0);
	ssc_data_set_number(data, "v_stage_2", 0);
	ssc_data_set_number(data, "v_stage_3", 0.035296);
	ssc_data_set_number(data, "GF_flowrate", 665994.875000);
	ssc_data_set_number(data, "qRejectByStage_1", 10269.627930);
	ssc_data_set_number(data, "qRejectByStage_2", 2573.040039);
	ssc_data_set_number(data, "qRejectByStage_3", 88.023651);
	ssc_data_set_number(data, "ncg_condensate_pump", 0.000192);
	ssc_data_set_number(data, "cw_pump_work", 0.024510);
	ssc_data_set_number(data, "pressure_ratio_1", 0.014923);
	ssc_data_set_number(data, "pressure_ratio_2", 0.036414);
	ssc_data_set_number(data, "pressure_ratio_3", 0.088857);
	ssc_data_set_number(data, "condensate_pump_power", 0.009464);
	ssc_data_set_number(data, "cwflow", 6632.956543);
	ssc_data_set_number(data, "cw_pump_head", 88.076920);
	ssc_data_set_number(data, "spec_vol", 6.419918);
	ssc_data_set_number(data, "x_hp", 0.105419);
	ssc_data_set_number(data, "hp_flash_pressure", 67.795647);
	ssc_data_set_number(data, "spec_vol_lp", 25.282135);
	ssc_data_set_number(data, "x_lp", 0.089993);
	ssc_data_set_number(data, "lp_flash_pressure", 15.700000);
	ssc_data_set_number(data, "flash_count", 2);
    ssc_data_set_number(data, "geotherm.cost.pump_casing_cost", 0.0);
    ssc_data_set_number(data, "geotherm.cost.pump_per_foot", 0.0);
    ssc_data_set_number(data, "geotherm.cost.pump_fixed", 0.0);
    ssc_data_set_number(data, "pump_depth_ft", 0.0);
    ssc_data_set_number(data, "num_wells_getem", 5.0);
    ssc_data_set_number(data, "pump_size_hp", 2000.0);
    ssc_data_set_number(data, "inj_pump_hp", 2000.0);
    ssc_data_set_number(data, "stimulation_type", 0);
    ssc_data_set_number(data, "geotherm.cost.prod_cost_curve_welldiam", 0);
    ssc_data_set_number(data, "resource_type", 0);
    ssc_data_set_number(data, "num_wells_getem_prod_drilled", 3);
    ssc_data_set_number(data, "num_wells_getem_inj_drilled", 1.0);
    ssc_data_set_number(data, "num_wells_getem_prod_failed", 1);
	//ssc_data_set_number(data, "baseline_cost", -1);
}

#endif
