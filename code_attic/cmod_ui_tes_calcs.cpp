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
#include "htf_props.h"
#include "sam_csp_util.h"
#include "csp_solver_two_tank_tes.h"

static var_info _cm_vtab_ui_tes_calcs[] = {

	/*   VARTYPE   DATATYPE         NAME               LABEL                                            UNITS     META  GROUP REQUIRED_IF CONSTRAINTS         UI_HINTS*/
	{ SSC_INPUT,   SSC_NUMBER,   "P_ref",                    "Power cycle output at design",                 "MWe",   "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "design_eff",               "Power cycle thermal efficiency",               "",      "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "tshours",                  "Hours of TES relative to q_dot_pb_des",        "hr",    "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "T_htf_hot_des",            "Hot design HTF temp from field",               "C",     "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "T_htf_cold_des",           "Cold design HTF temp into field",              "C",     "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "store_fluid",              "TES storage fluid code",                       "",      "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_MATRIX,   "store_fl_props",           "User defined tes storage fluid prop data",     "",      "7 columns (T,Cp,dens,visc,kvisc,cond,h), at least 3 rows", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "h_tank_min",               "Min. allowable HTF height in storage tank",    "m",     "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "h_tank",                   "Total height of tank (HTF when tank is full",  "m",     "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "tank_pairs",               "Number of equivalent tank pairs",              "",      "", "",  "*",  "", "" },
	{ SSC_INPUT,   SSC_NUMBER,   "u_tank",                   "Loss coefficient from the tank",               "W/m2-K","", "",  "*",  "", "" },
    { SSC_INPUT,   SSC_NUMBER,   "field_fluid",              "Field fluid code",                             "",      "", "",  "*",  "", "" },
    { SSC_INPUT,   SSC_MATRIX,   "field_fl_props",           "User defined field fluid prop data",           "",      "7 columns (T,Cp,dens,visc,kvisc,cond,h), at least 3 rows", "",  "*",  "", "" },
    { SSC_INPUT,   SSC_NUMBER,   "dt_hot",                   "Heat exchanger approach temperature",          "",      "", "",  "*",  "", "" },

	{ SSC_OUTPUT,  SSC_NUMBER,   "q_tes",                    "TES thermal capacity at design",               "MWt-hr","", "",  "*",  "", "" },
	{ SSC_OUTPUT,  SSC_NUMBER,   "tes_avail_vol",            "Available single temp storage volume",         "m^3",   "", "",  "*",  "", "" },
	{ SSC_OUTPUT,  SSC_NUMBER,   "vol_tank",                 "Total single temp storage volume",             "m^3",   "", "",  "*",  "", "" },
	{ SSC_OUTPUT,  SSC_NUMBER,   "csp_pt_tes_tank_diameter", "Single tank diameter",                         "m",     "", "",  "*",  "", "" },
	{ SSC_OUTPUT,  SSC_NUMBER,   "q_dot_tes_est",            "Estimated tank heat loss to env.",             "MWt",   "", "",  "*",  "", "" },
	{ SSC_OUTPUT,  SSC_NUMBER,   "csp_pt_tes_htf_density",   "HTF dens",                                     "kg/m^3","", "",  "*",  "", "" },
    { SSC_OUTPUT,  SSC_NUMBER,   "are_htfs_equal",           "0: no, 1: yes",                                "",      "", "",  "*",  "", "" },

	var_info_invalid};

class cm_ui_tes_calcs : public compute_module
{
public:

	cm_ui_tes_calcs()
	{
		add_var_info(_cm_vtab_ui_tes_calcs);
	}

	void exec() override
	{
		// Set TES fluid number and copy over fluid matrix if it makes sense
		HTFProperties tes_htf_props;
		int tes_fl = (int) as_double("store_fluid");
		util::matrix_t<double> tes_fl_props = as_matrix("store_fl_props");
		if( tes_fl != HTFProperties::User_defined && tes_fl < HTFProperties::End_Library_Fluids ) {
			if( !tes_htf_props.SetFluid(tes_fl) ) {
				throw exec_error("ui_tes_calcs", util::format("The user-defined HTF did not read correctly"));
			}
		}
		else if( tes_fl == HTFProperties::User_defined ) {
			size_t n_rows = tes_fl_props.nrows();
			size_t n_cols = tes_fl_props.ncols();
			if( n_rows > 2 && n_cols == 7 )	{
				if( !tes_htf_props.SetUserDefinedFluid(tes_fl_props) ) {
					std::string error_msg = util::format(tes_htf_props.UserFluidErrMessage(), n_rows, n_cols);
					throw exec_error("ui_tes_calcs", error_msg);
				}
			}
			else {
				std::string error_msg = util::format("The user defined storage HTF table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)", n_rows, n_cols);
				throw exec_error("ui_tes_calcs", error_msg);
			}
		}
		else {
			throw exec_error("ui_tes_calcs", "Storage HTF code is not recognized");
		}

        // Set fluid number and copy over fluid matrix if it makes sense
        HTFProperties field_htf_props;
        int field_fl = (int)as_double("field_fluid");
        util::matrix_t<double> field_fl_props = as_matrix("field_fl_props");
        if (field_fl != HTFProperties::User_defined && field_fl < HTFProperties::End_Library_Fluids) {
            if (!field_htf_props.SetFluid(field_fl)) {
                throw exec_error("ui_tes_calcs", util::format("The user-defined field HTF did not read correctly"));
            }
        }
        else if (field_fl == HTFProperties::User_defined) {
            size_t n_rows = field_fl_props.nrows();
            size_t n_cols = field_fl_props.ncols();
            if (n_rows > 2 && n_cols == 7) {
                if (!field_htf_props.SetUserDefinedFluid(field_fl_props)) {
                    std::string error_msg = util::format(field_htf_props.UserFluidErrMessage(), n_rows, n_cols);
                    throw exec_error("ui_tes_calcs", error_msg);
                }
            }
            else {
                std::string error_msg = util::format("The user defined field HTF table must contain at least 3 rows and exactly 7 columns. The current table contains %d row(s) and %d column(s)", n_rows, n_cols);
                assign("are_htfs_equal", std::numeric_limits<ssc_number_t>::quiet_NaN());
                assign("q_tes", std::numeric_limits<ssc_number_t>::quiet_NaN());
                assign("tes_avail_vol", std::numeric_limits<ssc_number_t>::quiet_NaN());
                assign("vol_tank", std::numeric_limits<ssc_number_t>::quiet_NaN());
                assign("q_dot_tes_est", std::numeric_limits<ssc_number_t>::quiet_NaN());
                assign("csp_pt_tes_tank_diameter", std::numeric_limits<ssc_number_t>::quiet_NaN());
                assign("csp.pt.tes.tank_diameter", std::numeric_limits<ssc_number_t>::quiet_NaN());
                assign("csp_pt_tes_htf_density", std::numeric_limits<ssc_number_t>::quiet_NaN());
                assign("csp.pt.tes.htf_density", std::numeric_limits<ssc_number_t>::quiet_NaN());
                return;
            }
        }
        else {
            throw exec_error("ui_tes_calcs", "Field HTF code is not recognized");
        }

        // Determine if field and tes fluids are same
        bool are_htfs_same = false;
        if (tes_fl != field_fl) {
            are_htfs_same = false;
        }
        else if (field_fl != HTFProperties::User_defined) {
            are_htfs_same = true;
        }
        else {
            are_htfs_same = tes_htf_props.equals(&field_htf_props);
        }

        double P_ref = as_double("P_ref");		                    // [MWe] Power cycle output at design
        double design_eff = as_double("design_eff");                // [-] Power cycle efficiency at design 
        double q_dot_pb_des = P_ref / design_eff;		            // [MWt] Power cycle thermal power at design
        double tshours = as_double("tshours");                      // [hrs] Hours of TES relative to q_dot_pb_des
        double Q_tes_des = q_dot_pb_des * tshours;                  // [MWt-hr] TES thermal capacity at design

		double T_htf_hot_des = as_double("T_htf_hot_des");			// [C] Hot HTF temp
		double T_htf_cold_des = as_double("T_htf_cold_des");		// [C] Cold HTF temp
		double T_HTF_ave = 0.5*(T_htf_hot_des + T_htf_cold_des);	// [C] Ave HTF temp at design
		double h_min = as_double("h_tank_min");			            // [m]
		double h_tank = as_double("h_tank");			            // [m]
		double tank_pairs = as_double("tank_pairs");	            // [-]
		double u_tank = as_double("u_tank");			            // [W/m^2-K]
        double dT_approach = as_double("dt_hot");                   // [K]
        double T_tes_hot, T_tes_cold;                               // [K]
        if (are_htfs_same) {
            T_tes_hot = T_htf_hot_des + 273.15;
            T_tes_cold = T_htf_cold_des + 273.15;
        }
        else {
            T_tes_hot = T_htf_hot_des - dT_approach + 273.15;
            T_tes_cold = T_htf_cold_des + dT_approach + 273.15;
        }
		double tes_avail_vol, vol_tank, csp_pt_tes_tank_diameter, q_dot_loss_des;
		tes_avail_vol = vol_tank = csp_pt_tes_tank_diameter = q_dot_loss_des = std::numeric_limits<double>::quiet_NaN();
		two_tank_tes_sizing(tes_htf_props, Q_tes_des, T_tes_hot, T_tes_cold,
			h_min, h_tank, (int)tank_pairs, u_tank,
			tes_avail_vol, vol_tank, csp_pt_tes_tank_diameter, q_dot_loss_des);

        assign("are_htfs_equal", are_htfs_same);
        assign("q_tes", (ssc_number_t)Q_tes_des);
		assign("tes_avail_vol", (ssc_number_t)tes_avail_vol);
		assign("vol_tank", (ssc_number_t)vol_tank);
		assign("q_dot_tes_est", (ssc_number_t)q_dot_loss_des);
		assign("csp_pt_tes_tank_diameter", (ssc_number_t)csp_pt_tes_tank_diameter);
        assign("csp.pt.tes.tank_diameter", (ssc_number_t)csp_pt_tes_tank_diameter);
		assign("csp_pt_tes_htf_density", (ssc_number_t)tes_htf_props.dens(T_HTF_ave + 273.15, 1.0));
        assign("csp.pt.tes.htf_density", (ssc_number_t)tes_htf_props.dens(T_HTF_ave + 273.15, 1.0));

		return;
	}
};

DEFINE_MODULE_ENTRY(ui_tes_calcs, "Calculates values for all calculated values on UI TES page(s)", 0)
