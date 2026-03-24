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

#ifndef MHK_WAVE_INPUTS_H_
#define MHK_WAVE_INPUTS_H_

#include <stdio.h>
#include "../test/input_cases/code_generator_utilities.h"

void wave_inputs(ssc_data_t &data) {
	char device_matrix[256];
	char resource_matrix[256];
	int nb2 = sprintf(resource_matrix, "%s/test/input_cases/mhk/wave_resource_matrix.csv", SSCDIR);
	int nb3 = sprintf(device_matrix, "%s/test/input_cases/mhk/wave_power_matrix.csv", SSCDIR);

	set_matrix(data, "wave_resource_matrix", resource_matrix, 21, 22);
	set_matrix(data, "wave_power_matrix", device_matrix, 21, 22);
    ssc_data_set_number(data, "wave_resource_model_choice", 0);
	ssc_data_set_number(data, "number_devices", 1);
	ssc_data_set_number(data, "system_capacity", 286);
	ssc_data_set_number(data, "device_rated_power", 286);
	ssc_data_set_number(data, "loss_array_spacing", 0);
	ssc_data_set_number(data, "loss_resource_overprediction", 0);
	ssc_data_set_number(data, "loss_transmission", 2);
	ssc_data_set_number(data, "loss_downtime", 5);
	ssc_data_set_number(data, "loss_additional", 0);

	ssc_data_set_number(data, "capital_cost", 12105156);
	ssc_data_set_number(data, "fixed_operating_cost", 1239344.125);
	ssc_data_set_number(data, "variable_operating_cost", 0);
	ssc_data_set_number(data, "fixed_charge_rate", 0.1080000028014183);

}

#endif
