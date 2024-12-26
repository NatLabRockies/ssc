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

#include "core.h"
#include <stdio.h>
#include <math.h>
#include "lib_geothermal.h"
#include "common.h"


static var_info _cm_vtab_geothermal_costs[] = {
	/*   VARTYPE			DATATYPE         NAME                              LABEL                                                       UNITS		META                      GROUP                   REQUIRED_IF                 CONSTRAINTS                      UI_HINTS*/

		{ SSC_INPUT,        SSC_NUMBER,     "conversion_type",					"Conversion Type",											"",			"",						"GeoHourly",			    "*",                        "INTEGER",					    "" },
        { SSC_INPUT,        SSC_NUMBER,     "ppi_base_year",					"PPI Base Year",									"",		"",						"GeoHourly",				"?=19",						"",								"" },

        // Binary Plant Type Inputs:		
		{ SSC_INPUT,		SSC_NUMBER,     "gross_output",						"Gross output from GETEM",									"kW",		"",						"GeoHourly",				"*",						"",								"" },
        { SSC_INPUT,		SSC_NUMBER,     "gross_cost_output",						"Gross output from GETEM for cost calculations",									"MW",		"",						"GeoHourly",				"*",						"",								"" },

        { SSC_INPUT,		SSC_NUMBER,		"design_temp",						"Power block design temperature",							"C",        "",						"GeoHourly",				"*",						"",								"" },
        { SSC_INPUT,        SSC_NUMBER,      "dt_prod_well",                   "Temperature loss in production well",                  "C",              "",             "GeoHourly",        "*",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,     "eff_secondlaw",					"Second Law Efficiency",									"%",		"",						"GeoHourly",				"*",						"",								"" },
		// Flash Plant Type Inputs:
		{ SSC_INPUT,		SSC_NUMBER,		"qRejectTotal",						"Total Rejected Heat",										"btu/h",			"",				"GeoHourly",				"conversion_type=1",		"",								""},
		{ SSC_INPUT,		SSC_NUMBER,		"qCondenser",						"Condenser Heat Rejected",									"btu/h",	"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"v_stage_1",						"Vacumm Pump Stage 1",										"kW",		"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"v_stage_2",						"Vacumm Pump Stage 2",										"kW",		"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"v_stage_3",						"Vacumm Pump Stage 3",										"kW",		"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"GF_flowrate",						"GF Flow Rate",												"lb/h",		"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"qRejectByStage_1",					"Heat Rejected by NCG Condenser Stage 1",					"BTU/hr",	"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"qRejectByStage_2",					"Heat Rejected by NCG Condenser Stage 2",					"BTU/hr",	"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"qRejectByStage_3",					"Heat Rejected by NCG Condenser Stage 3",					"BTU/hr",	"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"ncg_condensate_pump",				"Condensate Pump Work",										"kW",		"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"cw_pump_work",						"CW Pump Work",												"kW",		"",						"GeoHourly",				"conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"pressure_ratio_1",					"Suction Steam Ratio 1",									"",			"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"pressure_ratio_2",					"Suction Steam Ratio 2",									"",			"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"pressure_ratio_3",					"Suction Steam Ratio 3",									"",			"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"condensate_pump_power",			"hp",														"",			"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"cwflow",							"Cooling Water Flow",										"lb/h",		"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"cw_pump_head",						"Cooling Water Pump Head",									"lb/h",		"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"spec_vol",							"Specific Volume",											"cft/lb",	"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"spec_vol_lp",						"LP Specific Volume",										"cft/lb",	"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"x_hp",								"HP Mass Fraction",											"%",		"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"x_lp",								"LP Mass Fraction",											"%",		"",						"GeoHourly",				 "conversion_type=1",		 "",							"" },
		{ SSC_INPUT,		SSC_NUMBER,		"hp_flash_pressure",				"HP Flash Pressure",										"psia",		"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"lp_flash_pressure",				"LP Flash Pressure",										"psia",		"",						"GeoHourly",				 "conversion_type=1",		"",								"" },
		{ SSC_INPUT,		SSC_NUMBER,		"flash_count",						"Flash Count",												"(1 -2)",	"",						"GeoHourly",				 "conversion_type=1",		"",								"" },

        { SSC_INPUT,        SSC_NUMBER,      "calc_drill_costs",                      "Calculate drill costs",                      "0/1",             "0=LargerDiameter,1=SmallerDiameter",             "GeoHourly",        "?=0",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.inj_cost_curve_welltype",                      "Injection well type",                      "0/1",             "",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.prod_cost_curve_welltype",                      "Production well type",                      "0/1",             "",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.inj_cost_curve_welldiam",                      "Injection well diameter type",                      "0/1",             "0=LargerDiameter,1=SmallerDiameter",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.prod_cost_curve_welldiam",                      "Production well diameter type",                      "0/1",             "0=LargerDiameter,1=SmallerDiameter",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.inj_cost_curve",                      "Injection well diameter type",                      "0/1",             "0=LargerDiameter,1=SmallerDiameter",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.prod_cost_curve",                      "Production well diameter type",                      "0/1",             "0=LargerDiameter,1=SmallerDiameter",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "resource_depth",                     "Resource Depth",                               "m",              "",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.prod_wells_drilled",                      "Number of drilled production wells",                      "0/1",             "0=LargerDiameter,1=SmallerDiameter",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },
        { SSC_INPUT,        SSC_NUMBER,      "geotherm.cost.inj_wells_drilled",                      "Number of drilled injection wells",                      "0/1",             "0=LargerDiameter,1=SmallerDiameter",             "GeoHourly",        "calc_drill_costs=1",                        "",                "" },


        // Outputs	

		{ SSC_OUTPUT,       SSC_NUMBER,     "baseline_cost",					"Baseline Cost",											"$/kW",		"",                     "GeoHourly",				"?",                         "",                            "" },
        { SSC_OUTPUT,       SSC_NUMBER,     "inj_total_cost",					"Total Injection well cost",											"$/kW",		"",                     "GeoHourly",				"?",                         "",                            "" },
        { SSC_OUTPUT,       SSC_NUMBER,     "prod_total_cost",					"Total Production well cost",											"$/kW",		"",                     "GeoHourly",				"?",                         "",                            "" },


        var_info_invalid };



class cm_geothermal_costs : public compute_module
{
private:

	//Inputs for Binary Type Plant (Note: Some variables might be common to both plant types - Binary and Flash)
	std::vector<double> hx_ppi{ 0.89055794,0.919504053,0.938721984,0.956747735,0.963614688,0.972293753,0.983166428,1,0.998426323,1.066285169,1.226514068,1.332856462,1.377682403,1.438149738,1.414735336,1.423366714,1.463996185,1.512970911,1.534763948,1.554792561,1.604464797,1.643961076,1.657698912,1.742987979,1.797,1.831855031,1.997587983,2.272575844};		//HX Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
	std::vector<double> steel_ppi{ 1.129319793,1.103090524,1.1087163,1.074084898,0.999853876,1.022283919,0.961569372,1,1.06517133,1.423905896,1.500474903,1.63534741,1.76276759,2.160444217,1.613209615,1.959815884,2.220208957,2.110396727,1.984949222,2.03492365,1.714285714,1.638913234,1.858019281,2.084136722,1.947,1.825966245,3.12752612,3.334864154};	//Steel Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
	std::vector<double> process_equip_ppi{ 0.884018929,0.907470403,0.926181264,0.942518082,0.956851458,0.967823396,0.985395348,1,1.014829443,1.077774928,1.155295495,1.222766901,1.304818202,1.382893406,1.40355926,1.411450935,1.455548144,1.509649784,1.533757048,1.639031617,1.656479161,1.65317208,1.679672296,1.740780754,1.794,1.832151402,1.899909387,2.184450409}; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
	std::vector<double> engineering_ppi{ 0.77985529,0.810695609,0.859015689,0.888566516,0.913317573,0.954043986,0.975857869,1,1.048105165,1.081631922,1.102335411,1.135826349,1.209434773,1.274983881,1.329751415,1.39193352,1.362346873,1.364746758,1.388351601,1.433483774,1.486242476,1.503869304,1.558039553,1.602,1.611,1.646034816,1.700479977,1.780739467}; // Engineering Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
	std::vector<double> pump_ppi{ 0.853374525,0.872338403,0.899382129,0.92404943,0.936264259,0.9503327,0.975903042,1,1.010646388,1.039876426,1.093203422,1.14168251,1.212975285,1.277851711,1.31411597,1.324192015,1.324572243,1.34871673,1.339163498,1.366539924,1.391899601,1.411294923,1.438106104,1.489446663,1.553,1.553,1.553,1.553}; //Pump Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
	std::vector<double> turbine_ppi{ 0.882749597,0.895883655,0.917874396,0.93458132,0.960396538,0.969303543,0.980072464,1,1.013285024,1.018820451,1.017562399,1.050221417,1.10668277,1.24511876,1.350392512,1.3403281,1.359752415,1.349889291,1.376811594,1.411835749,1.399154589,1.403046162,1.346947738,1.332125604,1.408,1.452294686,1.49086252,1.540966184}; //Turbine-Generator Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
	std::vector<double> construction_ppi{ 0.788589981,0.814564007,0.840955473,0.870640074,0.907606679,0.932560297,0.956864564,1,1.038450835,1.066280148,1.086827458,1.1283859,1.1682282,1.222402597,1.278710575,1.319851577,1.357374768,1.362152134,1.376994434,1.429313544,1.475,1.528333333,1.594444444,1.653,1.676,1.709230056,1.752272727,1.858333333};
    std::vector<double> drilling_ppi{ 0.319286373,0.352814519,0.44078745,0.474007998,0.408182098,0.425099969,0.540449093,0.471239619,0.470624423,0.517379268,0.807751461,1.179637035,1.120885881,1.141802522,1.014764688,1,1.11319594,1.207320824,1.343586589,1.402952938,1.151953245,0.959704706,0.988926484,1.016302676,1.03,0.959092657,0.976696655,1.154758536};
    std::vector<double> legal_services_ppi{ 0.0 ,0.547045952,0.560722101,0.580415755,0.59463895,0.615426696,0.644967177,0.665754923,0.687089716,0.721006565,0.757658643,0.794310722,0.840262582,0.884026258,0.909190372,0.940371991,0.973194748,1,1.027899344,1.061269147,1.091356674,1.113238512,1.154266958,1.182713348,1.225,1.282294469,1.330897816,1.396991247}; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> og_support_ppi{ 0.62254627,0.6068424,0.632080763,0.637128435,0.633202468,0.691531127,0.798653954,0.770611329,0.760515984,0.795288839,0.904655076,1.052159282,1.089736399,1.104318564,1.01794728,1,1.067302299,1.102075154,1.117779024,1.117218172,1.103757712,1.081323612,1.088614694,1.103196859,1.108,1.041828294,1.050913679,1.157150869}; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> labor_ppi{ 0.807138965,0.833514986,0.858855586,0.879455041,0.905667575,0.93640327,0.965395095,1,1.029373297,1.055640327,1.082561308,1.099237057,1.128828338,1.160817439,1.192752044,1.216730245,1.237765668,1.247629428,1.261743869,1.279291553,1.302158273,1.336821452,1.366252453,1.409,1.449,1.490190736,1.556294278,1.639633748}; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;
    std::vector<double> chemical_ppi{ 0.567132867,0.559624586,0.558446816,0.536290026,0.525174825,0.570040486,0.567316894,0.562237762,0.625947736,0.718917924,0.832719912,0.937909459,1,1.212661023,1.033860876,1.188847994,1.434155318,1.355612808,1.330364372,1.276223776,1.069787986,1.007508834,1.121908127,1.21024735,1.116,0.999484726,1.366118881,1.56194788}; //Process Equipment Cost Index Normalized to 2001, 2002, 2007, 2010 and 2012; Beginning Year = 1995; Final Year = 2016;

    double user_adjust = 1; //User Adjustment (Constant)
	double size_ratio;
	//double scaling_factor ;	//for the GF HX
	double ref_plant_size = 10000;	//kW
	double hx_cost_adjust = 1;
	double hx_cost;
	double sf_hx;
	double hx_gf_c1;
	double hx_gf_c2;
	double hx_gf_c;	//ref gf hx cost
	double sales_tax = 0.05; //Always 5% ? 
	double freight = 0.05;	//Also always 5% ?
	//freight hydrothermal_binary = 0.05;
	//freight hydrothermal_flash = 
	double total_material_cost_multiplier = 1.7;
	double steel = 0.22;
	double current_cost_ref_hx;

	double sf_condenser;
	double condenser_cost;
	double wf_pump_cost;
	double turbine_cost;

	double corrected_equip_cost;

	//Defining variables used in Direct Construction Cost Multiplier:
	double dc_cost_multiplier;	//Direct Construction Cost Multiplier  [Defined]
	double corrected_total_material_mult;
	double corrected_construct_malts;
	double plant_size_adjustment;
	double direct_installation_multiplier;
	double escalation_equip_cost;	//Escalation in Equipment Cost
	double const_matls_rentals = 0.25;
	double multiplier_input_year;
	double corrected_labor;
	double labor_cost_multiplier = 0.27;
	double labor_fringe_benefits = 0.45;

	//Coefficients for GF HX cost calculation:
	double sf_0 = 1.01216;
	double sf_1 = -0.000760473;
	double sf_2 = 2.0145e-6;
	double sf_3 = 0;
	double hx_c10 = 5.95;
	double hx_c11 = 2163827753;
	double hx_c12 = -3.8105414;
	double hx_c20 = -22.09917;
	double hx_c21 = 0.4275955;
	double hx_c22 = -0.002356472;
	double hx_c23 = 4.24462e-06;

	//Coefficients for Air Cooled Condenser (ACC) Cost Calculations:
	double acc_c0 = 47;
	double acc_c1 = 11568490;
	double acc_c2 = -2.350919;
	double acc_c10 = 15.52712;
	double acc_c11 = 0.005950211;
	double acc_c12 = -0.001200635;
	double acc_c13 = 0.000005657483;
	double acc_c20 = 3.582461;
    double acc_c21 = -0.05107826;
    double acc_c22 = 0.000277465;
    double acc_c23 = -2.549939E-7;
	double acc_0;
	double acc_1;
	double acc_2;
	double accc_0;
	double accc_1;
	double accc_2;
	double acc_c;	//ref acc cost 
	double current_cost_ref_acc;

	//Coefficients for WF Pump Cost Calculation:
	double sf_wf;
	double wf_sf_c0 = -0.777900000001;
	double wf_sf_c1 = 0.029802;
	double wf_sf_c2 = -0.00019008;
	double wf_sf_c3 = 3.872e-07;
	double wf_c10 = 32.066071;
	double wf_c11 = -0.25379;
	double wf_c12 = 0.000671;
	double wf_c20 = -0.33297;
	double wf_c21 = 0.055929;
	double wf_c22 = -0.000198;
	double pcc_1;
	double pcc_2;
	double pcc_c;	//ref wf pump cost
	double current_cost_ref_pcc;

	//Coefficients for Turbine Cost Calculation: 
	double turbine_sf_c0 = 0.6642;
	double turbine_sf_c1 = 0.0003589091;
	double turbine_sf_c2 = -2.0218e-06;
	double sf_turbine;
	double turbine_c0 = 0.796647619;
	double turbine_c1 = -0.009773661;
	double turbine_c2 = 0.0000424483;
	double turbine_c3 = -5.32e-08;
	double turbine_c10 = -24.62889286;
	double turbine_c11 = 0.497681317;
	double turbine_c12 = -0.002962545;
	double turbine_c13 = 5.52551e-06;
	double ppc_0;
	double ppc_1;
	double turbine_c;	//Reference Turbine Cost
	double generator_c; //Reference Generator Cost
	double max_turbine_size = 15000 * 0.7457; //From GETEM
	double parasitic;
	double tg_size;
	double tg_sets;
	double ref_turbine_cost;
	double tg_cost;
	double current_cost_ref_tg;

	double plant_equip_cost, baseline_cost;
    double indirect_plant_cost;



	//Inputs for Flash Plant Type:
	int tg_sets_num = 1; //Number of T-G Sets
	double cooling_tower_cost;
	double condenser_cost_flash;
	double lmtd;
	double condenser_pinch_pt = 7.50; //As seen in GETEM, this value is always 7.50
	double dtCooling_water = 25.0;
	double condenser_u = 350.00; //As seen in GETEM, this value is always 350.00
	double area;
	//double qCondenser;
	double hp_total_cost;
	//double hp_flash_pressure;
	double a_cross_section;
	//double flash_vessels_cost = 552981.85;
	double vacuum_pump_1;	//stage 1 cost of vacuum pump
	double vacuum_pump_2;	//stage 2 cost of vacuum pump
	double vacuum_pump_3;	//stage 3 cost of vacuum pump
	double vacuum_pump;		//vacuum pump cost (sum of all 3 stages)
	int U = 350;				//Heat Transfer Coefficient
	double cond_area_1;
	double cond_area_2;
	double cond_area_3;
	double condenser_ncg;
	double ncg_pump_work;
	double ncg_water_pump;
	double pump_ncg;
	double ejector_ncg;
	double cw_pump_power;
	double condensate;
	double condensate_pump;
	double cooling_water;
	double pump_cost;
	double h2s_level = 20; //ppm
	double h2s_flow;
	double h2s_cost;
	double flash_vessel_cost;
	double equip_cost_flash;
	double ncg_cost;
	double current_tg_cost;
	double current_vessel_cost;
	double current_tower_cost;
	double current_condenser_cost;
	double current_pump_cost;
	double current_ncg_cost;
	double current_h2s_cost;

	//HP Flash Vessel Cost Inputs:
	double current_cost_flash;
	double m_stm, m_stm_lp;
	double hp_steam_flow;
	double max_drop_size = 200; //(microns)
	double v_terminal;
	double num_vessels; //number of hp flash vessels
	double area_xsection_hp;
	double hp_flash_volume;
	double A, D, H, A_lp, D_lp, H_lp;
	double hp_flash_cost;

	//LP Flash Vessel Cost Calculation:
	double num_vessels_lp, a_xsection_lp, v_terminal_lp, lp_steam_flow, lp_flash_volume, lp_flash_cost,
		direct_multiplier_2002, tax, labor_multiplier, construction_multiplier, freight_flash, material_multiplier, escalation_ppi,
		direct_plant_cost, condenser_heat_rejected, vStage_3;



public:
	//double hx_cost();

	cm_geothermal_costs() {

		add_var_info(_cm_vtab_geothermal_costs);
	}


	void exec() override
	{
		SGeothermal_Inputs geo_inputs;

        int calc_drill_costs = as_integer("calc_drill_costs");
        if (calc_drill_costs == 1) {
            int inj_cost_curve_welldiam = as_integer("geotherm.cost.inj_cost_curve_welldiam");
            int inj_cost_curve_welltype = as_integer("geotherm.cost.inj_cost_curve_welltype");
            int inj_cost_curve = as_integer("geotherm.cost.inj_cost_curve");
            int prod_cost_curve_welldiam = as_integer("geotherm.cost.prod_cost_curve_welldiam");
            int prod_cost_curve_welltype = as_integer("geotherm.cost.prod_cost_curve_welltype");
            int prod_cost_curve = as_integer("geotherm.cost.prod_cost_curve");

            //Drilling cost calculations
            double resource_depth = as_double("resource_depth");
            double inj_well_cost = 0;
            if (inj_cost_curve_welldiam == 0) {
                if (inj_cost_curve_welltype == 0) {
                    switch (inj_cost_curve) {
                    case 0:
                        inj_well_cost = 0.281801107 * pow(resource_depth, 2) + 1275.521301 * resource_depth + 632315.1264;
                        break;
                    case 1:
                        inj_well_cost = 0.189267288 * pow(resource_depth, 2) + 293.4517365 * resource_depth + 1326526.313;
                        break;
                    case 2:
                        inj_well_cost = 0.003145418 * pow(resource_depth, 2) + 782.70 * resource_depth + 983620.25;
                        break;
                    case 3:
                        inj_well_cost = -0.002397497 * pow(resource_depth, 2) + 752.94 * resource_depth + 524337.65;
                        break;
                    }
                }
                else {
                    switch (inj_cost_curve) {
                    case 0:
                        inj_well_cost = 0.2528 * pow(resource_depth, 2) + 1716.72 * resource_depth + 500866.89;
                        break;
                    case 1:
                        inj_well_cost = 0.19950 * pow(resource_depth, 2) + 296.13 * resource_depth + 1697867.71;
                        break;
                    case 2:
                        inj_well_cost = 0.0038019 * pow(resource_depth, 2) + 838.90 * resource_depth + 1181947.04;
                        break;
                    case 3:
                        inj_well_cost = 0.0037570 * pow(resource_depth, 2) + 762.53 * resource_depth + 765103.08;
                        break;
                    }

                }
            }
            else {
                if (inj_cost_curve_welltype == 0) {
                    switch (inj_cost_curve) {
                    case 0:
                        inj_well_cost = 0.30212 * pow(resource_depth, 2) + 584.91 * resource_depth + 751368.47;
                        break;
                    case 1:
                        inj_well_cost = 0.13710 * pow(resource_depth, 2) + 129.61 * resource_depth + 1205587.57;
                        break;
                    case 2:
                        inj_well_cost = 0.0080395 * pow(resource_depth, 2) + 455.61 * resource_depth + 921007.69;
                        break;
                    case 3:
                        inj_well_cost = 0.0025212 * pow(resource_depth, 2) + 439.45 * resource_depth + 590611.90;
                        break;
                    }

                }
                else {
                    switch (inj_cost_curve) {
                    case 0:
                        inj_well_cost = 0.28977 * pow(resource_depth, 2) + 882.15 * resource_depth + 680562.50;
                        break;
                    case 1:
                        inj_well_cost = 0.15340 * pow(resource_depth, 2) + 120.32 * resource_depth + 1431801.54;
                        break;
                    case 2:
                        inj_well_cost = 0.0085389 * pow(resource_depth, 2) + 506.08 * resource_depth + 1057330.39;
                        break;
                    case 3:
                        inj_well_cost = 0.0071869 * pow(resource_depth, 2) + 455.85 * resource_depth + 753377.73;
                        break;
                    }

                }
            }
            double inj_wells_drilled = as_double("geotherm.cost.inj_wells_drilled");
            double inj_total_cost = inj_wells_drilled * inj_well_cost;
            assign("inj_total_cost", inj_total_cost);

            double prod_well_cost = 0;
            if (prod_cost_curve_welldiam == 0) {
                if (prod_cost_curve_welltype == 0) {
                    switch (prod_cost_curve) {
                    case 0:
                        prod_well_cost = 0.281801107 * pow(resource_depth, 2) + 1275.521301 * resource_depth + 632315.1264;
                        break;
                    case 1:
                        prod_well_cost = 0.189267288 * pow(resource_depth, 2) + 293.4517365 * resource_depth + 1326526.313;
                        break;
                    case 2:
                        prod_well_cost = 0.003145418 * pow(resource_depth, 2) + 782.70 * resource_depth + 983620.25;
                        break;
                    case 3:
                        prod_well_cost = -0.002397497 * pow(resource_depth, 2) + 752.94 * resource_depth + 524337.65;
                        break;
                    }
                }
                else {
                    switch (prod_cost_curve) {
                    case 0:
                        prod_well_cost = 0.2528 * pow(resource_depth, 2) + 1716.72 * resource_depth + 500866.89;
                        break;
                    case 1:
                        prod_well_cost = 0.19950 * pow(resource_depth, 2) + 296.13 * resource_depth + 1697867.71;
                        break;
                    case 2:
                        prod_well_cost = 0.0038019 * pow(resource_depth, 2) + 838.90 * resource_depth + 1181947.04;
                        break;
                    case 3:
                        prod_well_cost = 0.0037570 * pow(resource_depth, 2) + 762.53 * resource_depth + 765103.08;
                        break;
                    }

                }
            }
            else {
                if (prod_cost_curve_welltype == 0) {
                    switch (prod_cost_curve) {
                    case 0:
                        prod_well_cost = 0.30212 * pow(resource_depth, 2) + 584.91 * resource_depth + 751368.47;
                        break;
                    case 1:
                        prod_well_cost = 0.13710 * pow(resource_depth, 2) + 129.61 * resource_depth + 1205587.57;
                        break;
                    case 2:
                        prod_well_cost = 0.0080395 * pow(resource_depth, 2) + 455.61 * resource_depth + 921007.69;
                        break;
                    case 3:
                        prod_well_cost = 0.0025212 * pow(resource_depth, 2) + 439.45 * resource_depth + 590611.90;
                        break;
                    }

                }
                else {
                    switch (prod_cost_curve) {
                    case 0:
                        prod_well_cost = 0.28977 * pow(resource_depth, 2) + 882.15 * resource_depth + 680562.50;
                        break;
                    case 1:
                        prod_well_cost = 0.15340 * pow(resource_depth, 2) + 120.32 * resource_depth + 1431801.54;
                        break;
                    case 2:
                        prod_well_cost = 0.0085389 * pow(resource_depth, 2) + 506.08 * resource_depth + 1057330.39;
                        break;
                    case 3:
                        prod_well_cost = 0.0071869 * pow(resource_depth, 2) + 455.85 * resource_depth + 753377.73;
                        break;
                    }

                }
            }

            double prod_wells_drilled = as_double("geotherm.cost.prod_wells_drilled");
            double prod_total_cost = prod_wells_drilled * prod_well_cost;
            assign("prod_total_cost", prod_total_cost);

            if (!is_assigned("conversion_type")) {
                return;
            }
            else {
                //keep going
            }
        }
		int conversion_type = as_integer("conversion_type");

        int ppi_base_year = as_integer("ppi_base_year");

        //int resource_type = as_integer("resource_type");

        

        //Exploration costs
        /*
        double prod_well_cost = as_double("prod_well_cost"); //todo provide input
        double num_expl_wells = as_double("num_expl_wells"); //todo provide input
        double expl_cost_multiplier = as_double("expl_cost_multiplier"); //todo provide input
        double total_drilling_cost_nodev = prod_well_cost * expl_cost_multiplier * num_expl_wells;

        double expl_permitting_cost = 250000 * legal_services_ppi[ppi_base_year];

        double num_wells_stimulated = as_double("num_wells_stimulated"); //todo provide input
        double expl_stimulation_cost = num_wells_stimulated * 1250000 * drilling_ppi[ppi_base_year];

        double percent_ind_cost = 0.04;
        if (resource_type == 1) percent_ind_cost = 0.05;
        double expl_indirect_cost = (prod_well_cost * num_expl_wells) * (1 / (1 - percent_ind_cost) - 1); //num_wells different here

        double total_predrilling_expl_cost = (resource_type == 0) ? 300000 : 250000 * og_support_ppi[ppi_base_year];
        double total_predrilling_permitting_cost = 60000 * legal_services_ppi[ppi_base_year];
        double total_predrilling_cost = total_predrilling_expl_cost + total_predrilling_permitting_cost;

        double lease_cost = as_double("lease_cost"); //todo add lease cost input
        double total_leasing_cost = num_expl_wells * 2600 * lease_cost; //number of wells different here again (# of full sized expl wells drilled + totals wells drilled in drilling phase)
        //2600 acres per well always

        double total_expl_cost = total_drilling_cost_nodev + expl_permitting_cost + expl_stimulation_cost + expl_indirect_cost + total_predrilling_cost + total_leasing_cost;
        */

		if (conversion_type == 0) {
			//geo_inputs.me_ct = BINARY;

			// for inputs, to get use as_integer, as_double, etc, e.g.
			//double unit_plant = as_double("nameplate");		//Gross plant size
			double design_temp = as_double("design_temp") - as_double("dt_prod_well");
			double eff = as_double("eff_secondlaw");	// w-h/lb
			double unit_plant = as_double("gross_output");

			//Geofluid Heat Exchangers Equipment Cost Calculations:				
			size_ratio = unit_plant / ref_plant_size;
			sf_hx = (sf_3 * pow(design_temp, 3)) + (sf_2 * pow(design_temp, 2)) + (sf_1 * design_temp) + sf_0;
			hx_gf_c1 = hx_c10 + (hx_c11 * pow(design_temp, hx_c12));
			hx_gf_c2 = hx_c20 + (hx_c21 * design_temp) + (hx_c22 * pow(design_temp, 2)) + (hx_c23 * pow(design_temp, 3));
            //hx_gf_c2 = hx_c20 + pow(hx_c21, pow(design_temp, 2)) + pow(hx_c23, pow(design_temp, 3)); //Took out hx_c22 per Parangat report -MP 6/14/21

			hx_gf_c = hx_gf_c1 * exp(hx_gf_c2 * eff);
			current_cost_ref_hx = hx_gf_c * hx_ppi[ppi_base_year];

			hx_cost = user_adjust * pow(size_ratio, sf_hx)*((ref_plant_size*hx_gf_c*hx_ppi[ppi_base_year]) / unit_plant);


			//Air Cooled Condenser Cost Calculations:
			sf_condenser = 1;
			accc_0 = (acc_c1 * pow(design_temp, acc_c2)) + acc_c0;
			accc_1 = exp((acc_c13*pow(design_temp, 3)) + (acc_c12*pow(design_temp, 2)) + (acc_c11*design_temp) + acc_c10);
			accc_2 = exp((acc_c23*pow(design_temp, 3)) + (acc_c22*pow(design_temp, 2)) + (acc_c21*design_temp) + acc_c20);
			acc_c = accc_1 * pow(eff, accc_2) + accc_0;
			//acc_c = 183.65;
			current_cost_ref_acc = acc_c * hx_ppi[ppi_base_year];
			condenser_cost = user_adjust * pow(size_ratio, sf_condenser)*((ref_plant_size * acc_c * hx_ppi[ppi_base_year]) / unit_plant);



			//Working Fluid Pumps Cost Calculation:
			sf_wf = (wf_sf_c3*pow(design_temp, 3)) + (wf_sf_c2*pow(design_temp, 2)) + (wf_sf_c1*design_temp) + wf_sf_c0;
			pcc_1 = (wf_c12 * pow(design_temp, 2)) + (wf_c11 * design_temp) + wf_c10;
			pcc_2 = (wf_c22 * pow(design_temp, 2)) + (wf_c21 * design_temp) + wf_c20;
			pcc_c = pcc_1 * exp(pcc_2*eff);
			current_cost_ref_pcc = pcc_c * pump_ppi[ppi_base_year];
			wf_pump_cost = user_adjust * pow(size_ratio, sf_wf)*((ref_plant_size * pcc_c * pump_ppi[ppi_base_year]) / unit_plant);


			//Turbine-Generator (TG) Cost Calculation:
			if (unit_plant < ref_plant_size)
				sf_turbine = (turbine_sf_c2*pow(design_temp, 2)) + (turbine_sf_c1*design_temp) + turbine_sf_c0;
			else
				sf_turbine = 1;

			ppc_0 = turbine_c0 + (turbine_c1*design_temp) + (turbine_c2*pow(design_temp, 2)) + (turbine_c3*pow(design_temp, 3));
			ppc_1 = turbine_c10 + (turbine_c11*design_temp) + (turbine_c12*pow(design_temp, 2)) + (turbine_c13*pow(design_temp, 3));
			parasitic = (ppc_0 * exp(ppc_1 * eff))*ref_plant_size;
			tg_size = ref_plant_size + parasitic;
			tg_sets = tg_size / max_turbine_size;

			if (tg_sets > 1)
				turbine_c = 7400 * pow(max_turbine_size, 0.6);		//Total $ of reference turbine
			else
				turbine_c = 7400 * pow(tg_size, 0.6);				//Total $ of reference turbine

			if (tg_sets < 1)
				ref_turbine_cost = turbine_c / ref_plant_size;		//Cost of turbine per kW ($/kW)
			else
				ref_turbine_cost = (turbine_c * tg_sets) / ref_plant_size;

			generator_c = ((1800 * pow(tg_size, 0.67))) / ref_plant_size;
			tg_cost = ref_turbine_cost + generator_c;
			current_cost_ref_tg = tg_cost * turbine_ppi[ppi_base_year];
			turbine_cost = user_adjust * pow(size_ratio, sf_turbine)*((ref_plant_size * tg_cost * turbine_ppi[ppi_base_year]) / unit_plant);


			//Calculating Direct Construction Cost Multiplier:
			escalation_equip_cost = (current_cost_ref_acc + current_cost_ref_hx + current_cost_ref_pcc + current_cost_ref_tg) / (hx_gf_c + acc_c + pcc_c + tg_cost);
			corrected_labor = ((labor_cost_multiplier*engineering_ppi[ppi_base_year]) / escalation_equip_cost)*(1 + labor_fringe_benefits);
			corrected_construct_malts = (const_matls_rentals * process_equip_ppi[ppi_base_year]) / escalation_equip_cost;
			corrected_total_material_mult = ((steel*steel_ppi[ppi_base_year]) + ((total_material_cost_multiplier - 1 - steel)*process_equip_ppi[ppi_base_year]))*(1 / escalation_equip_cost) + 1;
			multiplier_input_year = corrected_total_material_mult + corrected_labor + corrected_construct_malts;
			plant_size_adjustment = 1.02875*pow((unit_plant / 1000), -0.01226);
			direct_installation_multiplier = plant_size_adjustment * multiplier_input_year;
			dc_cost_multiplier = (sales_tax + freight)*((corrected_total_material_mult + corrected_construct_malts)*plant_size_adjustment) + direct_installation_multiplier;


			//Total Plant Cost: 
			plant_equip_cost = hx_cost + condenser_cost + wf_pump_cost + turbine_cost;
			corrected_equip_cost = dc_cost_multiplier * plant_equip_cost;

            //Indirect Plant Costs
            indirect_plant_cost = corrected_equip_cost * 0.12;

            corrected_equip_cost += indirect_plant_cost;


			// for outputs, to assign, use:
			//assign("dc_cost_multiplier", var_data(static_cast<ssc_number_t>(dc_cost_multiplier)));
			assign("baseline_cost", var_data(static_cast<ssc_number_t>(corrected_equip_cost)));
		}

		else if (conversion_type == 1) {
			//geo_inputs.me_ct = FLASH;
			double unit_plant = as_double("gross_output");
            double gross_cost = as_double("gross_cost_output");
            double GF_flowrate = as_double("GF_flowrate");
			double qRejectTotal = (as_double("qRejectTotal")*GF_flowrate / 1000) / 1000000;		// Converting from btu/h to MMBTU/h
			double q_Condenser = as_double("qCondenser") / 1000000;			// Converting from btu/h to MMBTU/h




			//double hp_flash_pressure = as_double("hp_flash_pressure");
			double v_stage_1 = as_double("v_stage_1");
			double v_stage_2 = as_double("v_stage_2");
			double v_stage_3 = as_double("v_stage_3");
			double qRejectByStage_1 = as_double("qRejectByStage_1");
			double qRejectByStage_2 = as_double("qRejectByStage_2");
			double qRejectByStage_3 = as_double("qRejectByStage_3");
			double ncg_condensate_pump = as_double("ncg_condensate_pump");
			double cw_pump_work = as_double("cw_pump_work");
			double pressure_ratio_1 = 1 / as_double("pressure_ratio_1");
			double pressure_ratio_2 = 1 / as_double("pressure_ratio_2");
			//double pressure_ratio_3 = 1 / as_double("pressure_ratio_3");
			int ncg_level = 2000;	//units: ppm
			double ncg_flow = GF_flowrate * ncg_level / 1000000; //units: lb/h
			double cwflow = as_double("cwflow");
			//double total_head = as_double("total_head");
			double condensate_pump_power = as_double("condensate_pump_power");
			double cw_pump_head = as_double("cw_pump_head");
			double spec_vol = as_double("spec_vol");
			double spec_vol_lp = as_double("spec_vol_lp");
			double x_hp = as_double("x_hp");		// %
			double x_lp = as_double("x_lp");	// %
			double hp_flash_pressure = as_double("hp_flash_pressure");
			double lp_flash_pressure = as_double("lp_flash_pressure");
			double flash_count = as_double("flash_count");
			double design_temp = as_double("design_temp");

			//T-G Cost:
			tg_cost = (tg_sets_num * (2830 * (pow((gross_cost / tg_sets_num), 0.745)))) + (3685 * (pow((gross_cost / tg_sets_num), 0.617)));	//Reference Equipment Cost
			current_tg_cost = tg_cost * turbine_ppi[ppi_base_year];


			//Cooling Tower Cost:
			condenser_heat_rejected = qRejectTotal;
			cooling_tower_cost = 7800 * (pow(condenser_heat_rejected, 0.8));		//Reference Equipment Cost
			current_tower_cost = cooling_tower_cost * process_equip_ppi[ppi_base_year];

			//Condenser Cost: 
			lmtd = (condenser_pinch_pt - (condenser_pinch_pt + dtCooling_water)) / (std::log(condenser_pinch_pt / (condenser_pinch_pt + dtCooling_water)));
			area = (q_Condenser*GF_flowrate / 1000) * 1000000 / (lmtd*condenser_u);
			condenser_cost_flash = 102 * pow(area, 0.85);		//Reference Equipment Cost
			current_condenser_cost = condenser_cost_flash * hx_ppi[ppi_base_year];

			//Flash Vessel Calculation:
			//HP Flash Cost Calculation:
			m_stm = x_hp * 1000; // (lb / h)
			hp_steam_flow = ((GF_flowrate / 1000)* m_stm) * spec_vol / 60;	// units: cfm
			v_terminal = (-0.0009414 * pow(max_drop_size, 2) * std::log(hp_flash_pressure)) + (0.01096 * pow(max_drop_size, 2));
			area_xsection_hp = hp_steam_flow / v_terminal;
			num_vessels = ceil(area_xsection_hp / 300);
			A = area_xsection_hp / num_vessels;
			D = pow((A * 4 / M_PI), 0.5);
			H = D * 3;
			hp_flash_volume = A * H * 7.4805;
			hp_flash_cost = num_vessels * ((hp_flash_pressure < 75) ? 166.5 * pow(hp_flash_volume, 0.625) : 110 * pow(hp_flash_volume, 0.68));

			//LP Flash Cost Calculation: 
			m_stm_lp = (flash_count == 2) ? (x_lp * 1000 * (1 - x_hp)) : 0;
			lp_steam_flow = ((GF_flowrate / 1000)*m_stm_lp) * spec_vol_lp / 60;	// (lb/h)
			v_terminal_lp = (flash_count == 1) ? 0 : ((-0.0009414 * pow(max_drop_size, 2) * std::log(lp_flash_pressure)) + (0.01096 * pow(max_drop_size, 2)));
			a_xsection_lp = (flash_count == 1) ? 0 : (lp_steam_flow / v_terminal_lp);
			num_vessels_lp = ceil(a_xsection_lp / 300);
			A_lp = (flash_count == 1) ? 0 : (a_xsection_lp / num_vessels_lp);
			D_lp = pow((A_lp * 4 / M_PI), 0.5);
			H_lp = D_lp * 3;
			lp_flash_volume = A_lp * H_lp * 7.4805;
			lp_flash_cost = (flash_count == 1) ? 0 : (num_vessels_lp * ((lp_flash_pressure < 75) ? (166.5 * pow(lp_flash_volume, 0.625)) : (110 * pow(lp_flash_volume, 0.68))));

			//Total Flash Vessel Cost:
			flash_vessel_cost = hp_flash_cost + lp_flash_cost;		//Reference Equipment Cost
			current_vessel_cost = flash_vessel_cost * process_equip_ppi[ppi_base_year];


			//NCG Removal System Cost: //Reference Equipment Cost
			//Vacuum Pump Cost breakdown:
			vacuum_pump_1 = (v_stage_1 < 5000) ? 70000 * pow(v_stage_1, 0.34) : 7400 * pow(v_stage_1, 0.6);
			vacuum_pump_2 = (v_stage_2 < 5000) ? 70000 * pow(v_stage_2, 0.34) : 7400 * pow(v_stage_2, 0.6);
			vStage_3 = v_stage_3 * (GF_flowrate / 1000);
			vacuum_pump_3 = (vStage_3 < 5000) ? 70000 * pow(vStage_3, 0.34) : 7400 * pow(vStage_3, 0.6);
			vacuum_pump = vacuum_pump_1 + vacuum_pump_2 + vacuum_pump_3;

			//(NCG) Condensers Cost Breakdown:	//Reference Equipment Cost
			cond_area_1 = (GF_flowrate / 1000) * (qRejectByStage_1 / (lmtd * 0.9 * U));
			cond_area_2 = (GF_flowrate / 1000) * (qRejectByStage_2 / (lmtd * 0.9 * U));
			cond_area_3 = (GF_flowrate / 1000) * (qRejectByStage_3 / (lmtd * 0.9 * U));
			condenser_ncg = 322 * (pow(cond_area_1, 0.72) + pow(cond_area_2, 0.72) + pow(cond_area_3, 0.72));

			//(NCG) Pumps Cost Calculation:	//Reference Equipment Cost
			ncg_pump_work = (ncg_condensate_pump * GF_flowrate / 1000) / 0.7457;
			ncg_water_pump = (cw_pump_work * GF_flowrate / 1000) / 0.7457;
			pump_ncg = 2.35 * 1185 * (pow(ncg_pump_work, 0.767) + pow(ncg_water_pump, 0.767));

			//(NCG) Ejector Cost Calculation: (Note: According to lib_geothermal.cpp, ncg removal type is alwasy JET)	//Reference Equipment Cost
			ejector_ncg = (76 * pow(pressure_ratio_1, (-0.45)) + 43 * pow(pressure_ratio_2, (-0.63))) * ncg_flow;

			//NCG total cost:	
			ncg_cost = vacuum_pump + condenser_ncg + pump_ncg + ejector_ncg;		//Reference Equipment Cost
			current_ncg_cost = ncg_cost * process_equip_ppi[ppi_base_year];

			//Pump Cost Calculation:
			condensate_pump = (GF_flowrate / 1000) * (condensate_pump_power * 1.34102);		//condensate_pump_power * 1.34102 is conversion from kW to hp
			condensate = 2.35 * 1185 * pow(condensate_pump, 0.767);
			cw_pump_power = (GF_flowrate / 1000) * ((((cwflow / 60)*(cw_pump_head)) / 33000) / 0.7);
			cooling_water = 2.35 * 1185 * pow(cw_pump_power, 0.767);
			pump_cost = condensate + cooling_water;		//Reference Equipment Cost
			current_pump_cost = pump_cost * pump_ppi[ppi_base_year];


			//H2S Removal System Cost Calculation:
			h2s_flow = h2s_level * GF_flowrate / 1000000;
			h2s_cost = 115000 * pow(h2s_flow, 0.58);		//Reference Equipment Cost
			current_h2s_cost = h2s_cost * process_equip_ppi[ppi_base_year];

			//Total Equipment Cost: 
			equip_cost_flash = tg_cost + cooling_tower_cost + condenser_cost_flash + flash_vessel_cost + ncg_cost + pump_cost + h2s_cost;
			current_cost_flash = current_tg_cost + current_tower_cost + current_condenser_cost + current_vessel_cost + current_ncg_cost + current_pump_cost + current_h2s_cost;
			escalation_ppi = current_cost_flash / equip_cost_flash;

			//Calculating Direct Construction Cost Multiplier:
			material_multiplier = 1 + ((8.65* pow(design_temp, -0.297)) - 1) * (process_equip_ppi[ppi_base_year] / escalation_ppi);
			labor_multiplier = ((42.65 * pow(design_temp, -0.923)) * 1.45) * construction_ppi[ppi_base_year] / escalation_ppi;
			construction_multiplier = (16.177*pow(design_temp, -0.827)) * process_equip_ppi[ppi_base_year] / escalation_ppi;
			direct_multiplier_2002 = material_multiplier + labor_multiplier + construction_multiplier;
			tax = (material_multiplier + construction_multiplier) * sales_tax;
			freight_flash = (material_multiplier + construction_multiplier) * freight;
			dc_cost_multiplier = direct_multiplier_2002 + tax + freight_flash;


			//Direct Plant Construction Cost: 
			direct_plant_cost = current_cost_flash * dc_cost_multiplier;
			baseline_cost = direct_plant_cost / unit_plant;		// ($/kW)

            //Indirect Plant Costs
            indirect_plant_cost = baseline_cost * 0.12;

            baseline_cost += indirect_plant_cost;

			assign("baseline_cost", var_data(static_cast<ssc_number_t>(baseline_cost)));

		}

       





        //OM Cost calculations
        /*
        double unit_plant = as_double("gross_output");
        double cooling_water_flow_rate = as_double("cwflow"); //todo provide input
        double cooling_water_cost = 300; //$/acre-ft //todo provide input
        double water_treatment_cost = (conversion_type == 0) ? 0 : cooling_water_flow_rate * cooling_water_cost;
        double non_labor_om_cost = 0.018 * baseline_cost * unit_plant + water_treatment_cost;

        //Plant OM
        double total_operator_cost = 0.25 * pow(unit_plant, 0.525) * 8760 * 20 * labor_ppi[ppi_base_year] * 1.8;
        double total_mechanic_cost = (conversion_type == 0) ? 0.15 : 0.13 * pow(unit_plant, 0.65) * 2000 * 24 * labor_ppi[ppi_base_year] * 1.8;
        double total_electrician_cost = (conversion_type == 0) ? 0.15 : 0.13 * pow(unit_plant, 0.65) * 2000 * 24 * labor_ppi[ppi_base_year] * 1.8;
        double total_general_maintenance_cost = (conversion_type == 0) ? 0.15 : 0.13 * pow(unit_plant, 0.65) * 2000 * 17.50 * labor_ppi[ppi_base_year] * 1.8;
        double total_facility_manager_cost = 0.075 * pow(unit_plant, 0.65) * 2000 * 40 * labor_ppi[ppi_base_year] * 1.8;
        double total_operations_manager_cost = 0.075 * pow(unit_plant, 0.65) * 2000 * 30 * labor_ppi[ppi_base_year] * 1.8;
        double total_clerical_cost = 0.075 * pow(unit_plant, 0.65) * 2000 * 12 * labor_ppi[ppi_base_year] * 1.8;
        double field_labor_om = total_operator_cost * 0.25;
        double labor_om_cost = total_operator_cost + total_mechanic_cost + total_electrician_cost + total_general_maintenance_cost
            + total_facility_manager_cost + total_operations_manager_cost + total_clerical_cost - field_labor_om;
        double plant_om = non_labor_om_cost + labor_om_cost;

        //Field OM
        double percent_drilling_cost_om = 1.5 / 100.0; //% always
        double well_om = percent_drilling_cost_om * as_double("geotherm.cost.prod_inj_total");
        double surface_equip_om = 0.015 * as_double("field_gathering_system_cost"); //todo provide input
        double GF_flowrate = as_double("GF_flowrate");
        double chemical_cost = GF_flowrate * as_double("num_wells") * 3600 * 24 * 365 * 0.95 * (conversion_type == 0) ? 0 : 22.5 * chemical_ppi[ppi_base_year]; //todo provide input
        double water_cost = 300;
        double water_loss = as_double("water_loss"); //todo where does this come from
        double makeup_water_cost = water_loss * water_cost;
        double total_annual_pump_cost = 0; //todo define
        double oil_downhole_pump_cost = 0; //todo define
        double pump_om_cost = total_annual_pump_cost + oil_downhole_pump_cost;
        double field_om = well_om + surface_equip_om + chemical_cost + makeup_water_cost + pump_om_cost + field_labor_om;

        
        //Annual Tax and Insurance
        double total_capital_cost = as_double("total_installed_cost"); //todo move OM to separate cmod, pass in total installed cost
        double tax_insurance_rate = 0.0075;
        double annual_tax_insurance_cost = total_capital_cost * tax_insurance_rate;
        */
        
	};

};

DEFINE_MODULE_ENTRY(geothermal_costs, "Geothermal monthly and hourly models using general power block code from TRNSYS Type 224 code by M.Wagner, and some GETEM model code.", 3);
