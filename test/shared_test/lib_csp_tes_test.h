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



#ifndef __LIB_CSP_TES_TEST_H__
#define __LIB_CSP_TES_TEST_H__

#include <memory>
#include <gtest/gtest.h>
#include "../tcs/storage_hx.h"
#include "csp_solver_two_tank_tes.h"

using Tank = Storage_HX;

namespace csp_common
{
    struct TankSpecifications;          // forward declaration
    struct TankState;
    struct TankExternalConditions;

    const double kErrorToleranceLo = 0.001;    // 0.1%
    const double kErrorToleranceHi = 0.01;     // 1.0%

    class TankFactory {
    public:
        TankFactory() {};
        virtual std::unique_ptr<Tank> MakeTank(TankSpecifications* tank_specifications) const = 0;
        virtual std::unique_ptr<TankSpecifications> MakeSpecifications() const = 0;
        virtual TankState MakeTankState() const = 0;
        virtual TankExternalConditions MakeExternalConditions() const = 0;
    };

    class DefaultTankFactory : public TankFactory {
    public:
        DefaultTankFactory() {};
        virtual std::unique_ptr<Tank> MakeTank(TankSpecifications* tank_specifications) const;
        virtual std::unique_ptr<TankSpecifications> MakeSpecifications() const;
        virtual TankState MakeTankState() const;
        virtual TankExternalConditions MakeExternalConditions() const;
    };

    struct TankSpecifications
    {
        int field_fluid;
        int store_fluid;
        HTFProperties fluid_field;
        HTFProperties fluid_store;
        bool is_direct;
        int config;
        double duty_des;
        double vol_des;
        double h_des;
        double u_des;
        double tank_pairs_des;
        double hot_htr_set_point_des;
        double cold_htr_set_point_des;
        double max_q_htr_cold;
        double max_q_htr_hot;
        double dt_hot_des;
        double dt_cold_des;
        double T_h_in_des;
        double T_h_out_des;
    };

    struct TankState
    {
        double m_prev;
        double T_prev;
    };

    struct TankExternalConditions
    {
        double m_dot_in;
        double m_dot_out;
        double T_in;
        double T_amb;
    };
}

#endif
