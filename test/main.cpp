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

#if defined( _WINDOWS) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <stdlib.h>
#include <iostream>
#include <gtest/gtest.h>

// in order to get MS V2017 update 2 to build without a bunch of C4996 "std::tr1:warning..."
#define _SILENCE_TR1_NAMESPACE_DEPRECIATION_WARNING

GTEST_API_ int main(int argc, char **argv) {

#if defined( _WINDOWS) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    printf("Running main() from gtest_main.cc\n");
    testing::InitGoogleTest(&argc, argv);

    //    filter to include
    //    ::testing::GTEST_FLAG(filter) = "CmodPVWatts*:CMPvwatts*";
    //::testing::GTEST_FLAG(filter) = "CmodHybridTest*";
    //::testing::GTEST_FLAG(filter) = "CMPvsamv1BatteryIntegration_cmod_pvsamv1.ResidentialDCBatteryModelPriceSignalDispatchGridExport";
    //::testing::GTEST_FLAG(filter) = "CMPvsamv1BatteryIntegration_cmod_pvsamv1.*";
    ::testing::GTEST_FLAG(filter) = "CMPvsamv1PowerIntegration_cmod_pvsamv1.UserArraySnowModel";
    //::testing::GTEST_FLAG(filter) = "CMGeothermal.*";
    //::testing::GTEST_FLAG(filter) = "save_as_JSON_test_run.*";
    //::testing::GTEST_FLAG(filter) = "csp_tower.PowerTowerCmod.Default_NoFinancial";
    //::testing::GTEST_FLAG(filter) = "CMPvsamv1PowerIntegration_cmod_pvsamv1.NoFinancialModelSystemDesign";
    //::testing::GTEST_FLAG(filter) = "CmodCashLoanTest*:CmodSingleOwnerTest*";
    //::testing::GTEST_FLAG(filter) = "CmodSaleLeasebackTest*:CmodThirdPartyOwnershipTest*:CmodHostDeveloperTest*:CmodMerchantPlantTest*:CmodLCOEFCRTest*";
    //::testing::GTEST_FLAG(filter) = "CmodLeveragedPartnershipFlipTest*:CmodAllEquityPartnershipFlipTest*";
    //::testing::GTEST_FLAG(filter) = "Solesca*";
    //::testing::GTEST_FLAG(filter) = "etes_ptes_test*";
    //::testing::GTEST_FLAG(filter) = "csp_tower.PowerTowerCmod*";
    //::testing::GTEST_FLAG(filter) = "CmodFresnelPhysicalTest.MSLFDefault";

    //    filter to exclude
    //    ::testing::GTEST_FLAG(filter) = "-PVSmoothing_lib_battery_dispatch*";

    // run multiple test
    //    ::testing::GTEST_FLAG(filter) = "CMPvwattsv8Integration_cmod_pvwattsv8.DefaultNoFinancialModel_cmod_pvwattsv8:CMPvwattsv8Integration_cmod_pvwattsv8.NonAnnual";

    int status = RUN_ALL_TESTS();

    //    sleep(10); //used for single test instruments leak detector on macOS

    if (!status)
        printf("Tests Pass!\n");
    return status;
}
