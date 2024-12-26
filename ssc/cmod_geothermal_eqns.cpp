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


#include <cmath>

#include "vartab.h"

#include "cmod_geothermal_eqns.h"
#pragma warning(disable: 4297)  // ignore warning: 'function assumed not to throw an exception but does'

bool getem_om_cost_calc(ssc_data_t data)
{
    auto vt = static_cast<var_table*>(data);
    if (!vt){
        return false;
    }

    //Inputs for Binary Type Plant (Note: Some variables might be common to both plant types - Binary and Flash)
    std::vector<double> hx_ppi{ 0.89055794,0.919504053,0.938721984,0.956747735,0.963614688,0.972293753,0.983166428,1,0.998426323,1.066285169,1.226514068,1.332856462,1.377682403,1.438149738,1.414735336,1.423366714,1.463996185,1.512970911,1.534763948,1.554792561,1.604464797,1.643961076,1.657698912,1.742987979,1.797,1.831855031,1.997587983,2.272575844 };		//HX Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> steel_ppi{ 1.129319793,1.103090524,1.1087163,1.074084898,0.999853876,1.022283919,0.961569372,1,1.06517133,1.423905896,1.500474903,1.63534741,1.76276759,2.160444217,1.613209615,1.959815884,2.220208957,2.110396727,1.984949222,2.03492365,1.714285714,1.638913234,1.858019281,2.084136722,1.947,1.825966245,3.12752612,3.334864154 };	//Steel Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> process_equip_ppi{ 0.884018929,0.907470403,0.926181264,0.942518082,0.956851458,0.967823396,0.985395348,1,1.014829443,1.077774928,1.155295495,1.222766901,1.304818202,1.382893406,1.40355926,1.411450935,1.455548144,1.509649784,1.533757048,1.639031617,1.656479161,1.65317208,1.679672296,1.740780754,1.794,1.832151402,1.899909387,2.184450409 }; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> engineering_ppi{ 0.77985529,0.810695609,0.859015689,0.888566516,0.913317573,0.954043986,0.975857869,1,1.048105165,1.081631922,1.102335411,1.135826349,1.209434773,1.274983881,1.329751415,1.39193352,1.362346873,1.364746758,1.388351601,1.433483774,1.486242476,1.503869304,1.558039553,1.602,1.611,1.646034816,1.700479977,1.780739467 }; // Engineering Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> pump_ppi{ 0.853374525,0.872338403,0.899382129,0.92404943,0.936264259,0.9503327,0.975903042,1,1.010646388,1.039876426,1.093203422,1.14168251,1.212975285,1.277851711,1.31411597,1.324192015,1.324572243,1.34871673,1.339163498,1.366539924,1.391899601,1.411294923,1.438106104,1.489446663,1.553,1.553,1.553,1.553 }; //Pump Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> turbine_ppi{ 0.882749597,0.895883655,0.917874396,0.93458132,0.960396538,0.969303543,0.980072464,1,1.013285024,1.018820451,1.017562399,1.050221417,1.10668277,1.24511876,1.350392512,1.3403281,1.359752415,1.349889291,1.376811594,1.411835749,1.399154589,1.403046162,1.346947738,1.332125604,1.408,1.452294686,1.49086252,1.540966184 }; //Turbine-Generator Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> construction_ppi{ 0.788589981,0.814564007,0.840955473,0.870640074,0.907606679,0.932560297,0.956864564,1,1.038450835,1.066280148,1.086827458,1.1283859,1.1682282,1.222402597,1.278710575,1.319851577,1.357374768,1.362152134,1.376994434,1.429313544,1.475,1.528333333,1.594444444,1.653,1.676,1.709230056,1.752272727,1.858333333 };
    std::vector<double> drilling_ppi{ 0.319286373,0.352814519,0.44078745,0.474007998,0.408182098,0.425099969,0.540449093,0.471239619,0.470624423,0.517379268,0.807751461,1.179637035,1.120885881,1.141802522,1.014764688,1,1.11319594,1.207320824,1.343586589,1.402952938,1.151953245,0.959704706,0.988926484,1.016302676,1.03,0.959092657,0.976696655,1.154758536 };
    std::vector<double> legal_services_ppi{ 0.0 ,0.547045952,0.560722101,0.580415755,0.59463895,0.615426696,0.644967177,0.665754923,0.687089716,0.721006565,0.757658643,0.794310722,0.840262582,0.884026258,0.909190372,0.940371991,0.973194748,1,1.027899344,1.061269147,1.091356674,1.113238512,1.154266958,1.182713348,1.225,1.282294469,1.330897816,1.396991247 }; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> og_support_ppi{ 0.62254627,0.6068424,0.632080763,0.637128435,0.633202468,0.691531127,0.798653954,0.770611329,0.760515984,0.795288839,0.904655076,1.052159282,1.089736399,1.104318564,1.01794728,1,1.067302299,1.102075154,1.117779024,1.117218172,1.103757712,1.081323612,1.088614694,1.103196859,1.108,1.041828294,1.050913679,1.157150869 }; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> labor_ppi{ 0.807138965,0.833514986,0.858855586,0.879455041,0.905667575,0.93640327,0.965395095,1,1.029373297,1.055640327,1.082561308,1.099237057,1.128828338,1.160817439,1.192752044,1.216730245,1.237765668,1.247629428,1.261743869,1.279291553,1.302158273,1.336821452,1.366252453,1.409,1.449,1.490190736,1.556294278,1.639633748 }; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> chemical_ppi{ 0.567132867,0.559624586,0.558446816,0.536290026,0.525174825,0.570040486,0.567316894,0.562237762,0.625947736,0.718917924,0.832719912,0.937909459,1,1.212661023,1.033860876,1.188847994,1.434155318,1.355612808,1.330364372,1.276223776,1.069787986,1.007508834,1.121908127,1.21024735,1.116,0.999484726,1.366118881,1.56194788 }; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> petroleum_ppi{ 1.010,1.258,1.051,0.713,1.245,1.547,1.000,1.372,1.568,2.041,2.907,2.959,4.034,2.196,3.441,4.149,4.850,4.785,4.743,3.448,2.063,1.686,2.045,2.504,2.278,1.689,2.823,4.262 };

    double unit_plant, cooling_water_flow_rate, drilling_cost, field_cost, flow_rate, num_wells, water_loss, total_capital_cost = 0;
    double conversion_type, ppi_base_year, baseline_cost = 0;
    double pump_cost_install, pump_only_cost, pump_type, pump_depth = 0;

    vt_get_number(vt, "gross_output", &unit_plant);
    vt_get_number(vt, "conversion_type", &conversion_type);
    vt_get_number(vt, "baseline_cost", &baseline_cost);
    vt_get_number(vt, "ppi_base_year", &ppi_base_year);
    vt_get_number(vt, "cwflow", &cooling_water_flow_rate);
    vt_get_number(vt, "drilling_cost", &drilling_cost);
    vt_get_number(vt, "field_gathering_system_cost", &field_cost);
    vt_get_number(vt, "GF_flowrate", &flow_rate);
    vt_get_number(vt, "num_wells", &num_wells);
    vt_get_number(vt, "water_loss", &water_loss);
    vt_get_number(vt, "total_installed_cost", &total_capital_cost);
    vt_get_number(vt, "pump_cost_install", &pump_cost_install);
    vt_get_number(vt, "pump_only_cost", &pump_only_cost);
    vt_get_number(vt, "pump_type", &pump_type);
    vt_get_number(vt, "pump_depth", &pump_depth);
   


    //OM Cost calculations

    double cooling_water_cost = 1.7 * chemical_ppi[ppi_base_year]; //$/acre-ft //todo provide input
    double water_treatment_cost = (conversion_type == 0) ? 0 : cooling_water_flow_rate * cooling_water_cost;
    double non_labor_om_cost = 0.018 * baseline_cost * unit_plant * 1000.0 + water_treatment_cost;

    //Plant OM
    double total_operator_cost = 0.25 * pow(unit_plant, 0.525) * 8760 * 20 * labor_ppi[ppi_base_year] * 1.8;
    double total_mechanic_cost = ((conversion_type == 0) ? 0.15 : 0.13) * pow(unit_plant, 0.65) * 2000 * 24 * labor_ppi[ppi_base_year] * 1.8;
    double total_electrician_cost = ((conversion_type == 0) ? 0.15 : 0.13) * pow(unit_plant, 0.65) * 2000 * 24 * labor_ppi[ppi_base_year] * 1.8;
    double total_general_maintenance_cost = ((conversion_type == 0) ? 0.15 : 0.13) * pow(unit_plant, 0.65) * 2000 * 17.50 * labor_ppi[ppi_base_year] * 1.8;
    double total_facility_manager_cost = 0.075 * pow(unit_plant, 0.65) * 2000 * 40 * labor_ppi[ppi_base_year] * 1.8;
    double total_operations_manager_cost = 0.075 * pow(unit_plant, 0.65) * 2000 * 30 * labor_ppi[ppi_base_year] * 1.8;
    double total_clerical_cost = 0.075 * pow(unit_plant, 0.65) * 2000 * 12 * labor_ppi[ppi_base_year] * 1.8;
    double field_labor_om = total_operator_cost * 0.25;
    double labor_om_cost = total_operator_cost + total_mechanic_cost + total_electrician_cost + total_general_maintenance_cost
        + total_facility_manager_cost + total_operations_manager_cost + total_clerical_cost - field_labor_om;
    double plant_om = non_labor_om_cost + labor_om_cost;

    //Field OM
    double percent_drilling_cost_om = 1.5 / 100.0; //% always
    double well_om = percent_drilling_cost_om * drilling_cost;
    double surface_equip_om = 0.015 * field_cost; //todo provide input
    double GF_flowrate = flow_rate;
    double chemical_cost = GF_flowrate * num_wells * 3600 * 24 * 365 * 0.95 * (conversion_type == 0) ? 0 : 22.5 * chemical_ppi[ppi_base_year]; //todo provide input
    double water_cost = 300;
    double makeup_water_cost = water_loss * water_cost;
    double rework_cost_per_pump = (conversion_type == 0) ? pump_only_cost+ pump_cost_install * 2: 0.0;
    double num_downhole_pumps = num_wells;
    double operating_life_pump = (pump_type == 0) ? 3 : 2;
    double pump_total_annual_rework_cost = rework_cost_per_pump * num_downhole_pumps / operating_life_pump;
    double cost_pump_per_year = 0;
    double ref_depth = 500; //feet
    double oil_cost = 4300.0 * petroleum_ppi[ppi_base_year];
    if (conversion_type == 0) {
        if (pump_depth != 0) {
            if (pump_depth < 250.0) cost_pump_per_year = 0.5 * oil_cost;
            else cost_pump_per_year = pump_depth / ref_depth * oil_cost;
        }
       
    }
    double pump_lineshaft_oil_cost = num_downhole_pumps * cost_pump_per_year;
    double total_annual_pump_cost = 0; //todo define
    double oil_downhole_pump_cost = 0; //todo define
    double pump_om_cost = pump_total_annual_rework_cost + pump_lineshaft_oil_cost;
    double field_om = well_om + surface_equip_om + chemical_cost + makeup_water_cost + pump_om_cost + field_labor_om;

    //Annual Tax and Insurance
    double tax_insurance_rate = 0.0075;
    double annual_tax_insurance_cost = total_capital_cost * tax_insurance_rate;

    double total_om = plant_om + field_om + annual_tax_insurance_cost;
    vt->assign("total_getem_om_cost", total_om);
    return true;
}





