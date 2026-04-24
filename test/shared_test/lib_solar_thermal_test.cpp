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


#include <gtest/gtest.h>

#include "lib_solar_thermal_test.h"

using namespace solar_thermal;

//========Tests===================================================================================
//=== Using factory patterns to create the different physical and non-physical components=========

// Basic test of expected power gain and outlet temperature of a single flat plate collector
TEST(FlatPlateCollectorTest, TestFlatPlateCollectorNominalOperation)
{
    DefaultFpcFactory default_fpc_factory = DefaultFpcFactory();
    std::unique_ptr<FlatPlateCollector> flat_plate_collector = default_fpc_factory.MakeCollector();
    std::unique_ptr<TimeAndPosition> time_and_position = default_fpc_factory.MakeTimeAndPosition();
    std::unique_ptr<ExternalConditions> external_conditions = default_fpc_factory.MakeExternalConditions();

    HeatAndTempInOut heat_and_temp = flat_plate_collector->HeatFlowsAndOutletTemp(*time_and_position, *external_conditions);
    double net_heat_gain = heat_and_temp.Q_gain - heat_and_temp.Q_loss;  // [kWt]

    EXPECT_NEAR(net_heat_gain, 1.659, 1.659 * kErrorToleranceHi);
    EXPECT_NEAR(heat_and_temp.T_out, 50.26, 50.26 * kErrorToleranceHi);
}

// Basic test of expected power gain and outlet temperature of a flat plate collector array
// Uses a factory (abstract factory pattern) to create the different physical and non-physical components
TEST(FlatPlateArrayTest, TestFlatPlateArrayOfOneNominalOperation)
{
    DefaultFpcFactory default_fpc_factory = DefaultFpcFactory();
    std::unique_ptr<FlatPlateArray> flat_plate_array = default_fpc_factory.MakeFpcArray();
    tm timestamp = default_fpc_factory.MakeTime();
    std::unique_ptr<ExternalConditions> external_conditions = default_fpc_factory.MakeExternalConditions();
    external_conditions->inlet_fluid_flow.temp = 44.86;

    HeatAndTempInOut heat_and_temp = flat_plate_array->HeatFlowsAndOutletTemp(timestamp, *external_conditions);
    double net_heat_gain = heat_and_temp.Q_gain - heat_and_temp.Q_loss;  // [kWt]

    EXPECT_NEAR(net_heat_gain, 1.587, 1.587 * kErrorToleranceHi);
    EXPECT_NEAR(heat_and_temp.T_out, 49.03, 49.03 * kErrorToleranceHi);
}
//========/Tests==================================================================================

//========Factories:==============================================================================
//========FpcFactory (super class)================================================================
std::unique_ptr<FlatPlateArray> FpcFactory::MakeFpcArray(FlatPlateCollector* flat_plate_collector,
                                                         CollectorLocation* collector_location,
                                                         CollectorOrientation* collector_orientation,
                                                         ArrayDimensions* array_dimensions,
                                                         Pipe* inlet_pipe,
                                                         Pipe* outlet_pipe) const
{
    return std::unique_ptr<FlatPlateArray>(new FlatPlateArray(*flat_plate_collector, *collector_location,
        *collector_orientation, *array_dimensions, *inlet_pipe, *outlet_pipe));
}

std::unique_ptr<FlatPlateCollector> FpcFactory::MakeCollector(CollectorTestSpecifications* collector_test_specifications) const
{
    return std::unique_ptr<FlatPlateCollector>(new FlatPlateCollector(*collector_test_specifications));
}

std::unique_ptr<TimeAndPosition> FpcFactory::MakeTimeAndPosition() const
{
    auto time_and_position = std::unique_ptr<TimeAndPosition>(new TimeAndPosition);
    time_and_position->timestamp = this->MakeTime();
    time_and_position->collector_location = this->MakeLocation();
    time_and_position->collector_orientation = this->MakeOrientation();

    return time_and_position;
}
//========/FpcFactory============================================================================

//========DefaultFPCFactory (subclass)===========================================================
std::unique_ptr<FlatPlateArray> DefaultFpcFactory::MakeFpcArray() const
{
    std::unique_ptr<FlatPlateCollector> flat_plate_collector = this->MakeCollector();
    CollectorLocation collector_location = this->MakeLocation();
    CollectorOrientation collector_orientation = this->MakeOrientation();
    ArrayDimensions array_dimensions = this->MakeArrayDimensions();

    std::unique_ptr<Pipe> inlet_pipe = this->MakePipe();
    std::unique_ptr<Pipe> outlet_pipe = this->MakePipe();

    return std::unique_ptr<FlatPlateArray>(new FlatPlateArray(*flat_plate_collector, collector_location,
        collector_orientation, array_dimensions, *inlet_pipe, *outlet_pipe));
}

std::unique_ptr<FlatPlateCollector> DefaultFpcFactory::MakeCollector() const
{
    std::unique_ptr<CollectorTestSpecifications> collector_test_specifications = this->MakeTestSpecifications();
    return std::unique_ptr<FlatPlateCollector>(new FlatPlateCollector(*collector_test_specifications));
}

std::unique_ptr<CollectorTestSpecifications> DefaultFpcFactory::MakeTestSpecifications() const
{
    auto collector_test_specifications = std::unique_ptr<CollectorTestSpecifications>(new CollectorTestSpecifications());
    collector_test_specifications->FRta = 0.689;
    collector_test_specifications->FRUL = 3.85;
    collector_test_specifications->iam = 0.2;
    collector_test_specifications->area_coll = 2.98;
    collector_test_specifications->m_dot = 0.045528;         // kg/s   
    collector_test_specifications->heat_capacity = 4.182;    // kJ/kg-K

    return collector_test_specifications;
}

CollectorLocation DefaultFpcFactory::MakeLocation() const
{
    CollectorLocation collector_location;
    collector_location.latitude = 33.45000;
    collector_location.longitude = -111.98000;
    collector_location.timezone = -7;

    return collector_location;
}

CollectorOrientation DefaultFpcFactory::MakeOrientation() const
{
    CollectorOrientation collector_orientation;
    collector_orientation.tilt = 30.;
    collector_orientation.azimuth = 180.;

    return collector_orientation;
}

std::unique_ptr<Pipe> DefaultFpcFactory::MakePipe() const
{
    double inner_diameter = 0.019;
    double insulation_conductivity = 0.03;
    double insulation_thickness = 0.006;
    double length = 5;

    return std::unique_ptr<Pipe>(new Pipe(inner_diameter, insulation_conductivity, insulation_thickness, length));
}

std::unique_ptr<ExternalConditions> DefaultFpcFactory::MakeExternalConditions() const
{
    auto external_conditions = std::unique_ptr<ExternalConditions>(new ExternalConditions);
    external_conditions->weather.ambient_temp = 25.;
    external_conditions->weather.dni = 935.;
    external_conditions->weather.dhi = 84.;
    external_conditions->weather.ghi = std::numeric_limits<double>::quiet_NaN();
    external_conditions->weather.wind_speed = std::numeric_limits<double>::quiet_NaN();
    external_conditions->weather.wind_direction = std::numeric_limits<double>::quiet_NaN();
    external_conditions->inlet_fluid_flow.m_dot = 0.091056;          // kg/s
    external_conditions->inlet_fluid_flow.specific_heat = 4.182;     // kJ/kg-K
    external_conditions->inlet_fluid_flow.temp = 45.9;               // from previous timestep
    external_conditions->albedo = 0.2;

    return external_conditions;
}

tm DefaultFpcFactory::MakeTime() const
{
    tm time;
    // TODO - The timestamp should be generated from a string so all attributes are valid
    time.tm_year = 2012 - 1900;  // years since 1900
    time.tm_mon = 1 - 1;         // months since Jan. (Jan. = 0)
    time.tm_mday = 1;
    time.tm_hour = 12;
    time.tm_min = 30;
    time.tm_sec = 0;

    return time;
}

ArrayDimensions DefaultFpcFactory::MakeArrayDimensions() const
{
    ArrayDimensions array_dimensions;
    array_dimensions.num_in_parallel = 1;
    array_dimensions.num_in_series = 1;

    return array_dimensions;
}
//========/DefaultFpcFactory======================================================================
