#include <string>
#include <cmath>
#include <iostream>
#include <fstream>
#include <gtest/gtest.h>

#include "../ssc/vartab.h"
#include "../rapidjson/document.h"
#include "../rapidjson/istreamwrapper.h"
#include "../rapidjson/filereadstream.h"

static const char* SSCDIR = std::getenv("SSCDIR");

char inputs_as_JSON[256] = {};
int nj1 = sprintf(inputs_as_JSON, "%s/test/input_json/pvwatts_merchantplant.json", SSCDIR);
char JSON_for_PySAM_PVWatts[256] = {};
int nj2 = sprintf(JSON_for_PySAM_PVWatts, "%s/test/input_json/pvwattsv7.json", SSCDIR);
char JSON_for_PySAM_Grid[256] = {};
int nj3 = sprintf(JSON_for_PySAM_Grid, "%s/test/input_json/grid.json", SSCDIR);
char JSON_for_PySAM_MerchantPlant[256] = {};
int nj4 = sprintf(JSON_for_PySAM_MerchantPlant, "%s/test/input_json/merchantplant.json", SSCDIR);

char inputs_as_JSON1[256] = {};
int np1 = sprintf(inputs_as_JSON1, "%s/test/input_json/pvwatts_merchantplant.json", SSCDIR);
char inputs_as_JSON2[256] = {};
int np2 = sprintf(inputs_as_JSON2, "%s/test/input_json/PV_Batt_MP.json", SSCDIR);
char inputs_as_JSON3[256] = {};
int np3 = sprintf(inputs_as_JSON3, "%s/test/input_json/PT_MP.json", SSCDIR);
char inputs_as_JSON4[256] = {};
int np4 = sprintf(inputs_as_JSON4, "%s/test/input_json/Trough_MP.json", SSCDIR);
char inputs_as_JSON5[256] = {};
int np5 = sprintf(inputs_as_JSON5, "%s/test/input_json/Wind_MP.json", SSCDIR);

// test table from SAM defaults for MSPT none
char SAM_default_as_JSON[256] = {};
int ns1 = sprintf(SAM_default_as_JSON, "%s/test/input_json/MSPT_None.json", SSCDIR);


TEST(save_as_JSON_test, pvwatts_mechant_plant_read_file_to_string) {
    std::ifstream test(inputs_as_JSON);
    std::string str((std::istreambuf_iterator<char>(test)), std::istreambuf_iterator<char>());
    EXPECT_EQ(str.length(), 5279819); // latest version changes - run ssc/test/input_json.sam and Generate code->JSON for Inputs for each case
}

TEST(save_as_JSON_test_parse, pvwatts_mechant_plant_rapidjson_parse_file) {
    std::ifstream test(inputs_as_JSON, std::ifstream::binary);
    rapidjson::Document document;
    rapidjson::IStreamWrapper isw(test);
    document.ParseStream(isw);
    if (document.HasParseError())
    {
        // report to the user the failure and their locations in the document.
        std::cout << document.GetParseError()
            << "\n";
    }
    EXPECT_FALSE(document.HasParseError());
}
TEST(save_as_JSON_test_parse, pvwatts_mechant_plant_rapidjson_parse_file_freadstream) {
    FILE* fp = fopen(inputs_as_JSON, "rb"); // non-Windows use "r"
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    rapidjson::Document document;
    document.ParseStream(is);
    if (document.HasParseError())
    {
        // report to the user the failure and their locations in the document.
        std::cout << document.GetParseError()
            << "\n";
    }
    EXPECT_TRUE(!document.HasParseError());
}


TEST(save_as_JSON_test_parse, pv_batt_mechant_plant_rapidjson_parse_file) {
    std::ifstream test(inputs_as_JSON2, std::ifstream::binary);
    rapidjson::Document document;
    rapidjson::IStreamWrapper isw(test);
    document.ParseStream(isw);
    if (document.HasParseError())
    {
        // report to the user the failure and their locations in the document.
        std::cout << document.GetParseError()
            << "\n";
    }
    EXPECT_TRUE(!document.HasParseError());
}

TEST(save_as_JSON_test_parse, pt_mechant_plant_rapidjson_parse_file) {
    std::ifstream test(inputs_as_JSON3, std::ifstream::binary);
    rapidjson::Document document;
    rapidjson::IStreamWrapper isw(test);
    document.ParseStream(isw);
    if (document.HasParseError())
    {
        // report to the user the failure and their locations in the document.
        std::cout << document.GetParseError()
            << "\n";
    }
    EXPECT_TRUE(!document.HasParseError());
}

TEST(save_as_JSON_test_parse, trough_mechant_plant_rapidjson_parse_file) {
    std::ifstream test(inputs_as_JSON4, std::ifstream::binary);
    rapidjson::Document document;
    rapidjson::IStreamWrapper isw(test);
    document.ParseStream(isw);
    if (document.HasParseError())
    {
        // report to the user the failure and their locations in the document.
        std::cout << document.GetParseError()
            << "\n";
    }
    EXPECT_TRUE(!document.HasParseError());
}



TEST(save_as_JSON_test_run, pvwatts_mechant_plant_rapidjson_read_file_to_ssc_var_table) {
    std::ifstream test(inputs_as_JSON);
    std::string json_str((std::istreambuf_iterator<char>(test)), std::istreambuf_iterator<char>());
    auto vt = json_to_ssc_data(json_str.c_str());
    ssc_number_t num;
    ssc_data_get_number(vt, "number_metrics", &num);
    EXPECT_EQ(num, 13);
    ssc_data_free(vt);
}



TEST(save_as_JSON_test_run, pvwatts_mechant_plant_rapidjson_read_file_run_pvwatts) {
    std::ifstream test(inputs_as_JSON);
    std::string json_str((std::istreambuf_iterator<char>(test)), std::istreambuf_iterator<char>());
    auto data = json_to_ssc_data(json_str.c_str());
    auto mod = ssc_module_create("pvwattsv8");
    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy.csv", SSCDIR);
    ssc_data_set_string(data, "solar_resource_file", file_path);
    ssc_module_exec_set_print(0);
    bool success = ssc_module_exec(mod, data);
    EXPECT_TRUE(success);
    ssc_number_t annual_energy;
    ssc_data_get_number(data, "annual_energy", &annual_energy);
    EXPECT_NEAR(annual_energy, 88486061.9, 88486061.9 / 1e6); // latest version changes - run ssc/test/input_json.sam and Generate code->JSON for Inputs for each case
    
    ssc_module_free(mod);
    ssc_data_free(data);
}


TEST(save_as_JSON_test_run, pvwatts_mechant_plant_rapidjson) {
    std::ifstream test(inputs_as_JSON);
    std::string json_str((std::istreambuf_iterator<char>(test)), std::istreambuf_iterator<char>());
    auto data = json_to_ssc_data(json_str.c_str());
    ssc_module_exec_set_print(0);
    auto mod_pv = ssc_module_create("pvwattsv8");
    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy.csv", SSCDIR);
    ssc_data_set_string(data, "solar_resource_file", file_path);
    bool success = ssc_module_exec(mod_pv, data);
    EXPECT_TRUE(success);
    auto mod_mp = ssc_module_create("merchantplant");
    success = ssc_module_exec(mod_mp, data);
    EXPECT_TRUE(success);
    ssc_number_t npv;
    ssc_data_get_number(data, "project_return_aftertax_npv", &npv);
    EXPECT_NEAR(npv, -59203599.4, std::abs(-59203599.4) / 1e6); // latest version changes - run ssc/test/input_json.sam and Generate code->JSON for Inputs for each case
    
    ssc_module_free(mod_pv);
    ssc_module_free(mod_mp);
    ssc_data_free(data);

}


TEST(save_as_JSON_test_run, pv_batt_mechant_plant_rapidjson) {
    std::ifstream test(inputs_as_JSON2);
    std::string json_str((std::istreambuf_iterator<char>(test)), std::istreambuf_iterator<char>());
    auto data = json_to_ssc_data(json_str.c_str());
    ssc_module_exec_set_print(0);
    auto mod_pv = ssc_module_create("pvsamv1");
    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/phoenix_az_33.450495_-111.983688_psmv3_60_tmy.csv", SSCDIR);
    ssc_data_set_string(data, "solar_resource_file", file_path);
    bool success = ssc_module_exec(mod_pv, data);
    EXPECT_TRUE(success);
    auto mod_grid = ssc_module_create("grid");
    success = ssc_module_exec(mod_grid, data);
    EXPECT_TRUE(success);
    auto mod_mp = ssc_module_create("merchantplant");
    success = ssc_module_exec(mod_mp, data);
    EXPECT_TRUE(success);
    ssc_number_t npv;
    ssc_data_get_number(data, "project_return_aftertax_npv", &npv);
    EXPECT_NEAR(npv, -60264986.5, std::abs(-60264986.5) / 1e6); // latest version changes - run ssc/test/input_json.sam and Generate code->JSON for Inputs for each case

    ssc_module_free(mod_pv);
    ssc_module_free(mod_grid);
    ssc_module_free(mod_mp);
    ssc_data_free(data);
}
   

TEST(save_as_JSON_test_run, pt_mechant_plant_rapidjson) {
    std::ifstream test(inputs_as_JSON3);
    std::string json_str((std::istreambuf_iterator<char>(test)), std::istreambuf_iterator<char>());
    auto data = json_to_ssc_data(json_str.c_str());
    ssc_module_exec_set_print(0);
    auto mod_pv = ssc_module_create("tcsmolten_salt");
    char file_path[256];
    int nfc1 = sprintf(file_path, "%s/test/input_cases/general_data/daggett_ca_34.865371_-116.783023_psmv3_60_tmy.csv", SSCDIR);
    ssc_data_set_string(data, "solar_resource_file", file_path);
    bool success = ssc_module_exec(mod_pv, data);
    EXPECT_TRUE(success);
    auto mod_grid = ssc_module_create("grid");
    success = ssc_module_exec(mod_grid, data);
    EXPECT_TRUE(success);
    auto mod_mp = ssc_module_create("merchantplant");
    success = ssc_module_exec(mod_mp, data);
    EXPECT_TRUE(success);
    ssc_number_t npv;
    ssc_data_get_number(data, "project_return_aftertax_npv", &npv);
    EXPECT_NEAR(npv, -570160106.5, std::abs(-570160106.5) / 1e7); // latest version changes - run ssc/test/input_json.sam and Generate code->JSON for Inputs for each case
    
    ssc_module_free(mod_pv);
    ssc_module_free(mod_grid);
    ssc_module_free(mod_mp);
    ssc_data_free(data);

}



TEST(SAM_as_JSON_test, sam_table_ssc_table) {
    std::ifstream test(SAM_default_as_JSON);
    std::string json_str((std::istreambuf_iterator<char>(test)), std::istreambuf_iterator<char>());
    auto data = json_to_ssc_data(json_str.c_str());
    auto tab = ssc_data_get_table(data, "adjust"); // owned by data - no need to free

    ssc_number_t sf_const;
    ssc_data_get_number(tab, "constant", &sf_const);
    EXPECT_EQ(sf_const, 4.0);

    auto  vt = static_cast<var_table*>(data);

    EXPECT_EQ(vt->lookup("adjust")->table.lookup("constant")->num[0], 4.0);

    ssc_data_free(data);
}




