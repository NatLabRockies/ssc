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


#ifndef __LIB_CSP_TEST_H__
#define __LIB_CSP_TEST_H__

#include <gtest/gtest.h>
#include "../tcs/flat_plate_solar_collector.h"

namespace solar_thermal
{
    const double kErrorToleranceLo = 0.001;    // 0.1%
    const double kErrorToleranceHi = 0.01;     // 1.0%

    class FpcFactory {
    public:
        FpcFactory() {};

        virtual std::unique_ptr<FlatPlateArray> MakeFpcArray() const = 0;
        std::unique_ptr<FlatPlateArray> MakeFpcArray(FlatPlateCollector* flat_plate_collector,
            CollectorLocation* collector_location,
            CollectorOrientation* collector_orientation,
            ArrayDimensions* array_dimensions,
            Pipe* inlet_pipe,
            Pipe* outlet_pipe) const;
        virtual std::unique_ptr<FlatPlateCollector> MakeCollector() const = 0;
        std::unique_ptr<FlatPlateCollector> MakeCollector(CollectorTestSpecifications* collector_test_specifications) const;
        std::unique_ptr<TimeAndPosition> MakeTimeAndPosition() const;

        virtual std::unique_ptr<CollectorTestSpecifications> MakeTestSpecifications() const = 0;
        virtual CollectorLocation MakeLocation() const = 0;
        virtual CollectorOrientation MakeOrientation() const = 0;
        virtual std::unique_ptr<Pipe> MakePipe() const = 0;
        virtual std::unique_ptr<ExternalConditions> MakeExternalConditions() const = 0;
        virtual tm MakeTime() const = 0;
        virtual ArrayDimensions MakeArrayDimensions() const = 0;
    };

    class DefaultFpcFactory : public FpcFactory {
    public:
        DefaultFpcFactory() {};

        virtual std::unique_ptr<FlatPlateArray> MakeFpcArray() const;
        virtual std::unique_ptr<FlatPlateCollector> MakeCollector() const;

        virtual std::unique_ptr<CollectorTestSpecifications> MakeTestSpecifications() const;
        virtual CollectorLocation MakeLocation() const;
        virtual CollectorOrientation MakeOrientation() const;
        virtual std::unique_ptr<Pipe> MakePipe() const;
        virtual std::unique_ptr<ExternalConditions> MakeExternalConditions() const;
        virtual tm MakeTime() const;
        virtual ArrayDimensions MakeArrayDimensions() const;
    };
}

#endif
