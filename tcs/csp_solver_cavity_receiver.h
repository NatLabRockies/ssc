/*
BSD 3-Clause License

Copyright (c) Alliance for Energy Innovation, LLC. See also https://github.com/NatLabRockies/ssc/blob/develop/LICENSE
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


#ifndef __csp_solver_cavity_receiver_
#define __csp_solver_cavity_receiver_

#include "csp_solver_util.h"

#include "htf_props.h"
#include "csp_solver_core.h"
#include "csp_solver_pt_receiver.h"

#include "../splinter/include/definitions.h"

using namespace SPLINTER;


class C_cavity_receiver : public C_pt_receiver
{
public:

    enum E_mesh_types
    {
        quad,
        triangle_halfNgon,
        no_mesh
    };

    class C_rec_surface
    {
    public:
        util::matrix_t<double> vertices;    // (nr, nc) -> (vertex index, dimension [m] (i.e. xyx))
        E_mesh_types mesh_type;
        bool is_active_surf;        // True: active surface w/ HTF, False: passive surface no HTF
        double eps_sol;             //[-]
        double eps_therm;           //[-]
        double surf_elem_size;      //[m]

        C_rec_surface()
        {
            mesh_type = no_mesh;
            is_active_surf = false;
            eps_sol = eps_therm = surf_elem_size = std::numeric_limits<double>::quiet_NaN();
        }
    };

private:

    // ************************************
    // Design parameters passed in through constructor
    // ************************************
    int m_field_fl;
    util::matrix_t<double> m_field_fl_props;

    double m_dni_des;               //[W/m2]

    size_t m_nPanels;       //[-]
    size_t m_pipeWindings;  //[-]
    size_t m_modelRes;      //[-]
    size_t m_nPaths;        //[-]
    bool m_is_bottomUpFlow;     //[-]
    bool m_is_centerOutFlow;    //[-]
    double m_rec_span;      //[rad]

    double m_receiverHeight; //[m] Receiver opening height in meters
    double m_receiverWidth; //[m] Reciever opening width in meters
    double m_topLipHeight;  //[m] Height of top lip in meters
    double m_botLipHeight;  //[m] Height of bottom lip in meters
    double m_e_act_sol;     //[-] Absorbtivity in short wave range for active surfaces
    double m_e_pass_sol;    //[-] Absorbtivity in short wave range for passive surfaces
    double m_e_act_therm;   //[-] Emissivity in long wave range for active surfaces
    double m_e_pass_therm;  //[-] Emissivity in long wave range for passive surfaces

    E_mesh_types m_active_surface_mesh_type;
    E_mesh_types m_floor_and_cover_mesh_type;
    E_mesh_types m_lips_mesh_type;

    // ************************************

    // ************************************
    // Calculated stored parameters

    std::vector<C_rec_surface> mv_rec_surfs;    // vector of surface classes for each surface in cavity model
    std::vector<util::matrix_t<int>> m_v_elems; // each vector index is a surface; each row lists the nodes that define a mesh element
    util::matrix_t<double> m_nodesGlobal;       // each row lists x,y,z coordinates of each node

    util::matrix_t<int> m_elements;             // global element tracker; each row, indexed by m_surfIDs, lists the nodes that define a mesh element
    std::vector<util::matrix_t<int>> m_surfIDs; // global element count for each surface
    util::matrix_t<double> m_areas;             // global element areas, each row indexed by m_surfIDs
    Eigen::MatrixXd mE_areas;                   // global element areas, each row indexed by m_surfIDs
    util::matrix_t<double> m_centroids;         // global element centroids, each row indexed by m_surfIDs
    Eigen::MatrixXd mE_centroids;               // global element centroids, each row indexed by m_surfIDs
    std::vector<int> m_global_to_surf_index;    // global element to surface index (in mv_rec_surf)

    size_t m_nElems;                            // global element centroids, each row indexed by m_surfIDs
    double m_area_active_total;                 // [m2] total surface area of all active elements

    util::matrix_t<double> m_epsilonSol;        // global element solar emissivity 
    Eigen::MatrixXd mE_epsilonSol;              // global element solar emissivity
    util::matrix_t<double> m_epsilonTherm;      // global element thermal emissivity
    Eigen::MatrixXd mE_epsilonTherm;            // global element thermal emissivity

    std::vector<util::matrix_t<int>> m_FCA;     // fluid connectivity array

    util::matrix_t<double> m_F;                 // view factors

    util::matrix_t<double> m_FHatS;             // FHat solar
    Eigen::MatrixXd mE_FHatS;                   // FHat solar
    util::matrix_t<double> m_FHatT;             // FHat thermal
    Eigen::MatrixXd mE_FHatT;                   // FHat thermal

    Eigen::MatrixXd mE_rhoSol;                  // global element solar reflectivity
    Eigen::MatrixXd mE_rhoTherm;                // global element thermal reflectivity

    double m_d_in_rec_tube;             //[m]
    double m_A_cs_tube;                 //[m2]
    size_t m_Ntubes;                    //[-]
    double m_rel_roughness;             //[-]
    double m_A_aper;                    //[m2]
    double m_eta_therm_des;             //[-]

    // ************************************
    // Call variables
    double m_od_control;            //[-]

public:

	// Methods
	C_cavity_receiver(double dni_des /*W/m2*/,
        int field_fl /*-*/, util::matrix_t<double> field_fl_props,
        double od_rec_tube /*m*/, double th_rec_tube /*m*/, int tube_mat_code /*-*/,
        size_t nPanels /*-*/, double rec_height /*m*/, double rec_width /*m*/,
        double rec_span /*rad*/, double toplip_height /*m*/, double botlip_height /*m*/,
        double eps_active_sol /*-*/, double eps_passive_sol /*-*/, double eps_active_therm /*-*/, double eps_passive_therm /*-*/,
        E_mesh_types active_surface_mesh_type, E_mesh_types floor_and_cover_mesh_type,  E_mesh_types lips_mesh_type,
        double piping_loss_coefficient /*Wt/m2-K*/, double pipe_length_add /*m*/, double pipe_length_mult /*-*/,
        double h_tower /*m*/, double T_htf_hot_des /*C*/,
        double T_htf_cold_des /*C*/, double f_rec_min /*-*/, double q_dot_rec_des /*MWt*/,
        double rec_su_delay /*hr*/, double rec_qf_delay /*-*/, double m_dot_htf_max_frac /*-*/,
        double eta_pump /*-*/);

	~C_cavity_receiver() {};

	virtual void init();

	virtual void call(const C_csp_weatherreader::S_outputs& weather,
		const C_csp_solver_htf_1state& htf_state_in,
		const C_pt_receiver::S_inputs& inputs,
		const C_csp_solver_sim_info& sim_info);

	virtual void off(const C_csp_weatherreader::S_outputs& weather,
		const C_csp_solver_htf_1state& htf_state_in,
		const C_csp_solver_sim_info& sim_info);

	virtual void converged();

	virtual double get_pumping_parasitic_coef();

	virtual double area_proj();

    void steady_state_sln(double T_salt_cold_in /*K*/, double q_dot_inc /*Wt*/, double cp_htf /*J/kg-K*/,
        double T_amb /*K*/,
        const Eigen::MatrixXd& EsolarFlux /*W/m2*/,
        double tol_T_HTF_node_iter /*K*/, size_t count_T_HTF_node_iter /*-*/,
        double tol_rel_T_rec_node_iter /*-*/, size_t count_T_rec_node_iter /*-*/,
        double tol_abs_T_htf_target /*K*/, size_t count_T_htf_out_iter /*-*/,
        std::vector<double>& m_dot_paths /*kg/s*/, std::vector<double>& T_out_paths /*K*/,
        double& W_dot_pump /*MWe*/, double& q_gain_net_eb /*Wt*/, double& m_dot_htf_tot /*kg/s*/,
        double& T_htf_rec_no_tower_losses /*K*/, double& T_htf_tower_out_calc /*K*/, double& eta_thermal_calc /*-*/,
        double& q_dot_piping_losses /*Wt*/, double& q_dot_conv_losses /*Wt*/, double& q_dot_refl_losses /*Wt*/,
        double& q_dot_rad_losses /*Wt*/, double& q_dot_thermal_tower_out /*Wt*/, double& q_dot_thermal_rec_out_no_tower_losses /*Wt*/,
        bool& rec_is_off,
        Eigen::MatrixXd& E_h /*W/m2-K*/, Eigen::MatrixXd& E_T /*K*/,
        double& error_T_HTF_node_iter /*K*/, double& error_relmax_T_rec_node_iter /*-*/, double& error_T_htf_out /*K*/);

    void tube_UA_and_deltaP(std::vector<double> m_dot_paths /*kg/s*/, const Eigen::MatrixXd E_T_HTF /*K*/,
        Eigen::MatrixXd& UA, double& W_dot_pump /*MWe*/);

    void test_steady_state_matlab();

    void genOctCavity();

    void meshGeometry();

    void makeGlobalElems();

    void surfValuesToElems();

    void zigzagRouting();

    void VFMatrix();

    void FHatMatrix(const util::matrix_t<double>& eps,
        util::matrix_t<double>& F_hat, util::matrix_t<double>& rho,
        Eigen::MatrixXd& E_F_hat, Eigen::MatrixXd& E_rho);

    void matrixt_to_eigen(const util::matrix_t<double>& matrixt,
        Eigen::MatrixXd& eigenx);

    void eigen_to_matrixt(const Eigen::MatrixXd& eigenx,
        util::matrix_t<double>& matrixt);

    void eigen_to_matrixt(const Eigen::MatrixXi& eigenx,
        util::matrix_t<int>& matrixt);

    Eigen::MatrixXd furthest(const Eigen::MatrixXd cents, const Eigen::MatrixXd aimpoint);

    Eigen::MatrixXd nearest(const Eigen::MatrixXd cents, const Eigen::MatrixXd aimpoint);

    void hbarCorrelation(const Eigen::MatrixXd& T, double T_inf, Eigen::MatrixXd& h);

    void interpSolarFlux(const util::matrix_t<double>& fluxDist);

    void edgePairParameters(const util::matrix_t<double>& Po, const util::matrix_t<double>& Pf, const util::matrix_t<double>& Qo, const util::matrix_t<double>& Qf,
        double& D, util::matrix_t<double>& sOrigin, util::matrix_t<double>& sHat, util::matrix_t<double>& lHat, util::matrix_t<double>& lOrigin, bool& skew);

    void viewFactor(const util::matrix_t<double>& a, const util::matrix_t<double>& b, double& F_AB, double& F_BA);

    double fParallel(double s, double l, double d);

    double f_skew(double s, double l, double alpha, double cosAlpha, double sinAlpha, double d);

    double imagLi_2(double mag, double angle);

    double Cl(double theta);

    void meshMapped(const util::matrix_t<double>& poly, double elemSize,
        util::matrix_t<double>& nodes, util::matrix_t<int>& quads);

    void meshPolygon(const util::matrix_t<double>& poly, double elemSize);

    void meshHalfNgon(const util::matrix_t<double>& poly, double elemSize,
        util::matrix_t<double>& nodes, util::matrix_t<int>& quads);

    void crossproduct(const util::matrix_t<double>&, const util::matrix_t<double>&, util::matrix_t<double>& cross);

    void norm3Dvect(const util::matrix_t<double>&, util::matrix_t<double>& norm_vect);

    double mag_vect(const util::matrix_t<double>& vector_in);

    double dotprod3D(const util::matrix_t<double>&, const util::matrix_t<double>&);

    void flipup(const util::matrix_t<double>& a, util::matrix_t<double>& b);

    void sumcolumns(const util::matrix_t<double>&, util::matrix_t<double>&);

    void sum_int_columns(const util::matrix_t<int>& a, util::matrix_t<int>& summed);

    void diffrows(const util::matrix_t<double>& a, const util::matrix_t<double>& b, util::matrix_t<double>& a_less_b);

    void add_vect_rows(const util::matrix_t<double>& a, const util::matrix_t<double>& b, util::matrix_t<double>& a_plus_b);

    void scale_vect(const util::matrix_t<double>& a, double scale, util::matrix_t<double>& out_vect);

    void add_constant_to_each_element(int val, util::matrix_t<int>& a);

    void ave_columns(const util::matrix_t<double>&, util::matrix_t<double>&);

    double max_row_value(const util::matrix_t<double>& a);

    int max_row_int_value(const util::matrix_t<int>& a);

    double min_val_first_colum(const util::matrix_t<double>& a);

    double min_column_val(const util::matrix_t<double>& a, size_t n_c);

    double max_column_val(const util::matrix_t<double>& a, size_t n_c);

    int max_int_first_column(const util::matrix_t<int>& a);

    bool are_rows_equal(const util::matrix_t<double>& a, const util::matrix_t<double>& b, int i_row);

    void min_max_vects_from_columns(const util::matrix_t<double>& a, util::matrix_t<double>& max_vect, util::matrix_t<double>& min_vect);

    void transpose_matrix_t(const util::matrix_t<double>& a, util::matrix_t<double>& b);

    void transpose_int_matrix_t(const util::matrix_t<int>& a, util::matrix_t<int>& b);

    void to2D(const util::matrix_t<double>& poly, const util::matrix_t<double>& center,
        const util::matrix_t<double>& normal, const util::matrix_t<double>& xaxis,
        util::matrix_t<double>& poly_xy, util::matrix_t<double>& poly_rt);

    void map(const util::matrix_t<double>& poly2D, double elemSize,
        util::matrix_t<double>& nodes, util::matrix_t<int>& quads);

    void to3D(const util::matrix_t<double>& poly_xy, const util::matrix_t<double>& origin,
        const util::matrix_t<double>& normal, const util::matrix_t<double>& xaxis,
        util::matrix_t<double>& poly3d);

    // triMesh2D(fd, fh, h0, bbox, pfix, varargin)
    void triMesh2D(double h0, const util::matrix_t<double>& bbox, const util::matrix_t<double>& pfix,
        const util::matrix_t<double>& poly_2D);

    void pointToPoly(const util::matrix_t<double>& p, const util::matrix_t<double>& POLY,
                util::matrix_t<double>& d);

    double pointToLine(const util::matrix_t<double>& p, const util::matrix_t<double>& a,
        const util::matrix_t<double>& b);

    void inpolygon(const util::matrix_t<double>& p_x, const util::matrix_t<double>& p_y,
        const util::matrix_t<double>& poly_x, const util::matrix_t<double>& poly_y,
        util::matrix_t<int>& is_in_polygon);

    void polygon_normal_and_area(const util::matrix_t<double>& poly_a,
        util::matrix_t<double>& norm_vect, double& area, int& n_rows);

};

namespace cavity_receiver_helpers
{
    void calc_receiver_macro_geometry(double rec_height /*m*/, double rec_width /*m*/,
        double rec_span /*rad*/, size_t nPanels /*-*/,
        double& theta0 /*rad*/, double& panelSpan /*rad*/, double& panel_width /*m*/,
        double& rec_area /*m2*/, double& radius /*m*/, double& offset /*m*/);

    void calc_receiver_macro_geometry_sp_inputs(double rec_height /*m*/, double radius /*m*/,
        double f_offset /*-*/, size_t nPanels /*-*/,
        double& theta0 /*rad*/, double& panelSpan /*rad*/, double& panel_width /*m*/,
        double& rec_area /*m2*/, double& rec_width /*m*/, double& rec_span /*rad*/, double& offset /*m*/);

    double calc_total_receiver_absorber_area(double rec_height /*m*/, double rec_width /*m*/,
        double rec_span /*rad*/, size_t nPanels /*-*/);

    void test_cavity_case();
};

#endif // __csp_solver_cavity_receiver_

