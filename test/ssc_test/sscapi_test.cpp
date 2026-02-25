/*
BSD 3-Clause License

Copyright Alliance for Energy Innovation, LLC. See also https://github.com/NREL/ssc/blob/develop/LICENSE


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


#include <string>
#include <cmath>
#include <gtest/gtest.h>

#include "../ssc/vartab.h"

TEST(sscapi_test, json_to_ssc_data) {

    
    std::string json_string = R"({"num": 5})";
    ssc_data_t dat = json_to_ssc_data(json_string.c_str());
    auto vt = static_cast<var_table *>(dat);
    EXPECT_EQ(vt->lookup("num")->num[0], 5);
    ssc_data_free(dat);



    
    json_string = R"({"str": "string"})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table *>(dat);
    EXPECT_STRCASEEQ(vt->lookup("str")->str.c_str(), "string");
    ssc_data_free(dat);

    
    json_string = R"({"arr": [1, 2]})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table *>(dat);
    EXPECT_EQ(vt->lookup("arr")->num[0], 1);
    EXPECT_EQ(vt->lookup("arr")->num[1], 2);
    ssc_data_free(dat);

    
    json_string = R"({"mat": [[1, 2], [3, 4]]})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table *>(dat);
    EXPECT_EQ(vt->lookup("mat")->num[0], 1);
    EXPECT_EQ(vt->lookup("mat")->num[1], 2);
    EXPECT_EQ(vt->lookup("mat")->num[2], 3);
    EXPECT_EQ(vt->lookup("mat")->num[3], 4);
    ssc_data_free(dat);

    
    json_string = R"({"datarr": ["one", 2]})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table *>(dat);
    EXPECT_STRCASEEQ(vt->lookup("datarr")->vec[0].str.c_str(), "one");
    EXPECT_EQ(vt->lookup("datarr")->vec[1].num[0], 2);
    ssc_data_free(dat);

    
    json_string = R"({"datmat": [["one", 2], [3, {"four": 4}]]})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table *>(dat);
    EXPECT_STRCASEEQ(vt->lookup("datmat")->vec[0].vec[0].str.c_str(), "one");
    EXPECT_EQ(vt->lookup("datmat")->vec[0].vec[1].num[0], 2);
    EXPECT_EQ(vt->lookup("datmat")->vec[1].vec[0].num[0], 3);
    EXPECT_EQ(vt->lookup("datmat")->vec[1].vec[1].table.lookup("four")->num[0], 4);
    ssc_data_free(dat);


    
    json_string = R"({"table": {"entry": 1}})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table *>(dat);
    EXPECT_EQ(vt->lookup("table")->table.lookup("entry")->num[0], 1);
    ssc_data_free(dat);


    json_string = R"({"wrong": format})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table *>(dat);
    EXPECT_GT(vt->lookup("error")->str.size(), 0);
    ssc_data_free(dat);
    
}






TEST(sscapi_test, ssc_data_to_json) {
    var_table vt;
    vt.assign("num", 1);
    auto json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"num\":1.0}");
    vt.clear();
    delete json_string;

    vt.assign("str", var_data("string"));
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"str\":\"string\"}");
    vt.clear();
    delete json_string;

    vt.assign("arr", std::vector<double>({ 1, 2 }));
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"arr\":[1.0,2.0]}");
    vt.clear();
    delete json_string;

    double vals[4] = { 1, 2, 3, 4 };
    vt.assign("mat", var_data(vals, 2, 2));
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"mat\":[[1.0,2.0],[3.0,4.0]]}");
    vt.clear();
    delete json_string;
    
    std::vector<var_data> vars = { var_data("one"), 2 };
    vt.assign("datarr", vars);
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"datarr\":[\"one\",2.0]}");
    vt.clear();
    delete json_string;
    
    std::vector<std::vector<var_data>> vars_mat = { vars, std::vector<var_data>({3, 4}) };
    vt.assign("datmat", vars_mat);
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"datmat\":[[\"one\",2.0],[3.0,4.0]]}");
    vt.clear();
    delete json_string;
    
    var_table tab;
    tab.assign("entry", 1);
    vt.assign("table", tab);
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"table\":{\"entry\":1.0}}");
    vt.clear();
    delete json_string;

}



////////////////////////////  RapidJSON testing
TEST(sscapi_test, rapidjson_to_ssc_data) {
    std::string json_string = R"({"num": 5})";
    ssc_data_t dat = json_to_ssc_data(json_string.c_str());
    auto vt = static_cast<var_table*>(dat);
    EXPECT_EQ(vt->lookup("num")->num[0], 5);
    ssc_data_free(dat);

    json_string = R"({"str": "string"})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table*>(dat);
    EXPECT_STRCASEEQ(vt->lookup("str")->str.c_str(), "string");
    ssc_data_free(dat);

    json_string = R"({"arr": [1, 2]})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table*>(dat);
    EXPECT_EQ(vt->lookup("arr")->num[0], 1);
    EXPECT_EQ(vt->lookup("arr")->num[1], 2);
    ssc_data_free(dat);
    
    json_string = R"({"mat": [[1, 2], [3, 4]]})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table*>(dat);
    EXPECT_EQ(vt->lookup("mat")->num[0], 1);
    EXPECT_EQ(vt->lookup("mat")->num[1], 2);
    EXPECT_EQ(vt->lookup("mat")->num[2], 3);
    EXPECT_EQ(vt->lookup("mat")->num[3], 4);
    ssc_data_free(dat);
    
    json_string = R"({"datarr": ["one", 2]})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table*>(dat);
    EXPECT_STRCASEEQ(vt->lookup("datarr")->vec[0].str.c_str(), "one");
    EXPECT_EQ(vt->lookup("datarr")->vec[1].num[0], 2);
    ssc_data_free(dat);
    
    json_string = R"({"datmat": [["one", 2], [3, {"four": 4}]]})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table*>(dat);
    EXPECT_STRCASEEQ(vt->lookup("datmat")->vec[0].vec[0].str.c_str(), "one");
    EXPECT_EQ(vt->lookup("datmat")->vec[0].vec[1].num[0], 2);
    EXPECT_EQ(vt->lookup("datmat")->vec[1].vec[0].num[0], 3);
    EXPECT_EQ(vt->lookup("datmat")->vec[1].vec[1].table.lookup("four")->num[0], 4);
    ssc_data_free(dat);

   // parse error returned because of malformed json (extra "}" at end) not caught by jsoncpp 
    json_string = R"({"table": {"entry": 1}}})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table*>(dat);
    if (vt->is_assigned("table"))
        EXPECT_EQ(vt->lookup("table")->table.lookup("entry")->num[0], 1);
    else // error test fails - report error code and not throw SEH error from parse error
        EXPECT_STRCASEEQ(vt->lookup("error")->str.c_str(), "The document root must not be followed by other values.");
    ssc_data_free(dat);
    
    json_string = R"({"table": {"entry": 1}})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table*>(dat);
    if (vt->is_assigned("table"))
        EXPECT_EQ(vt->lookup("table")->table.lookup("entry")->num[0], 1);
    else // error test fails - report error code and not throw SEH error from parse error
        EXPECT_STRCASEEQ(vt->lookup("error")->str.c_str(), "error");
    ssc_data_free(dat);

    json_string = R"({"wrong": format})";
    dat = json_to_ssc_data(json_string.c_str());
    vt = static_cast<var_table*>(dat);
    EXPECT_GT(vt->lookup("error")->str.size(), 0);
    ssc_data_free(dat);

}



TEST(sscapi_test, ssc_data_to_rapidjson) {
    var_table vt;
    vt.assign("num", 1);
    auto json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"num\":1.0}");
    vt.clear();
    delete json_string;
    
    vt.assign("str", var_data("string"));
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"str\":\"string\"}");
    vt.clear();
    delete json_string;

    vt.assign("arr", std::vector<double>({ 1, 2 }));
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"arr\":[1.0,2.0]}");
    vt.clear();
    delete json_string;

    double vals[4] = { 1, 2, 3, 4 };
    vt.assign("mat", var_data(vals, 2, 2));
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"mat\":[[1.0,2.0],[3.0,4.0]]}");
    vt.clear();
    delete json_string;
    
    std::vector<var_data> vars = { var_data("one"), 2 };
    vt.assign("datarr", vars);
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"datarr\":[\"one\",2.0]}");
    vt.clear();
    delete json_string;
    
    std::vector<std::vector<var_data>> vars_mat = { vars, std::vector<var_data>({3, 4}) };
    vt.assign("datmat", vars_mat);
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"datmat\":[[\"one\",2.0],[3.0,4.0]]}");
    vt.clear();
    delete json_string;
    
    var_table tab;
    tab.assign("entry", 1);
    vt.assign("table", tab);
    json_string = ssc_data_to_json(&vt);
    EXPECT_STRCASEEQ(json_string, "{\"table\":{\"entry\":1.0}}");
    vt.clear();
    tab.clear();
    delete json_string;
    
}



