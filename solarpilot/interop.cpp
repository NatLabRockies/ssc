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

#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <thread>

#include "interop.h"
#include "SolarField.h"
#include "STObject.h"
#include "solpos00.h"
#include "sort_method.h"

#include "LayoutSimulateThread.h"
#include "STSimulateThread.h"
#include "STObject.h"
#include "IOUtil.h"

using namespace std;

// Used to get intercept efficiency for each heliostat (For central receviers, requires a lot of rays to be traced)
#define HELIO_INTERCEPT false;  

//-------------------  arraystring ----------------

ArrayString::ArrayString(){data.clear();}
	
//wxArrayStr &operator=( ArrayString &array );
ArrayString &ArrayString::operator=( vector<string> &array )
{ 
	data = array; 
	return *this;
}
	
int ArrayString::size(){return (int)data.size();}
string ArrayString::operator[](int i){return data.at(i);}
string& ArrayString::at(int i){return data.at(i);}
	
void ArrayString::clear(){data.clear();}
void ArrayString::Clear(){data.clear();}	

void ArrayString::resize(int newsize){data.resize(newsize);}
void ArrayString::push_back(string value){data.push_back(value);}
void ArrayString::Add(string value){data.push_back(value);}

string ArrayString::back(){return data.back();}
int ArrayString::Index(string item){
	for(int i=0; i<(int)data.size(); i++){
		if(item == data.at(i)) return i;
	}
	return -1;
};

vector<string>::iterator ArrayString::erase(vector<string>::iterator position){ return data.erase( position ); }
vector<string>::iterator ArrayString::begin(){return data.begin(); }
//-------------------

//-------------------  par_variable ----------------
par_variable::par_variable()
{ 
    linked = false; 
    layout_required = false;
}
//-------------------

//------------------- SimControl -------------------
void SimControl::SetThreadCount( int nthread)
{
	_n_threads = max(min(int(std::thread::hardware_concurrency()), nthread), 1);
}

SimControl::SimControl()
{
	_n_threads = 1;
	_n_threads_active = 1;
	_is_mt_simulation = false;
	_cancel_simulation = false;

	_stthread = 0;
	_STSim = 0;
}

SimControl::~SimControl()
{
	if (_STSim != 0) delete _STSim;
	if (_stthread != 0) delete[] _stthread;

	_n_threads = 1;
	_n_threads_active = 1;
	_is_mt_simulation = false;
	_cancel_simulation = false;

	_stthread = 0;
	_STSim = 0;
}
//--------------------

//interop namespace

void interop::GenerateSimulationWeatherData(var_map &V, int design_method, ArrayString &wf_entries){
	/* 
	Calculate and fill the weather data steps needed for simulation and the associated time step.

	The weather data is filled in the variable set vset["solarfield"][0]["sim_step_data"].value

	wf_entries consists of a list strings corresponding to each time step. 
	Each string is comma-separated and has the following entries:
	day, hour, month, dn, tdry, pres/1000., wspd
	*/
	
	WeatherData *wdatvar = &V.sf.sim_step_data.Val(); 

    //convert ArrayString wf_entries to matrix_t<double>
    matrix_t<double> wf_entries_d;
    {
        int nr, nc;
        nr = (int)wf_entries.size();
        nc = (int)split(wf_entries.at(0), ",").size();

        wf_entries_d.resize(nr, nc);

        for (int i = 0; i < nr; i++)
        {
            std::vector<std::string> row = split(wf_entries.at(i), ",");

            for (int j = 0; j < nc; j++)
            {
                double val;
                to_double(row.at(j), &val);
                wf_entries_d.at(i, j) = val;
            }
        }
    }

	switch (design_method)
	{
    case var_solarfield::DES_SIM_DETAIL::SUBSET_OF_DAYSHOURS:
	{		//Subset of days/hours
		//Need to add this still
		V.amb.sim_time_step.Setval(0.);
		throw spexception("Simulation with a user-specified list of days/hours is not currently supported. Please use another option.");
		//break;
	}
    case var_solarfield::DES_SIM_DETAIL::SINGLE_SIMULATION_POINT:
	{  //2) Single design point=1;
		V.amb.sim_time_step.Setval(0.);
        vector<string> vdata = split(Ambient::getDefaultSimStep(), ",");
        int hour, dom, month;
        to_integer(vdata.at(0), &dom);
        to_integer(vdata.at(1), &hour);
        to_integer(vdata.at(2), &month);
        sim_params P;
        //dni, T, P, V, Wt
        to_double(vdata.at(3), &P.dni);
        to_double(vdata.at(4), &P.Tamb);
        to_double(vdata.at(5), &P.Patm);
        to_double(vdata.at(6), &P.Vwind);

        //calculate total annual DNI energy
        P.Simweight = 0.;
        for (int i = 0; i < wf_entries_d.nrows(); i++)
            P.Simweight += wf_entries_d.at(i, 3);

        wdatvar->resizeAll(1);
        wdatvar->setStep(0, dom, hour, month, P.dni, P.Tamb, P.Patm, P.Vwind, P.Simweight);
		break;
	}
    case var_solarfield::DES_SIM_DETAIL::DO_NOT_FILTER_HELIOSTATS:
	{	//3) Do not filter heliostats=0;
		wdatvar->clear();
		V.amb.sim_time_step.Setval(0.);
		break;
	}
    case var_solarfield::DES_SIM_DETAIL::ANNUAL_SIMULATION:
	{	//4) Annual simulation=3;
		*wdatvar = WeatherData(V.amb.wf_data.val); //vset["ambient"][0]["wf_data"].value;
        wdatvar->initPointers();

		V.amb.sim_time_step.Setval(3600.);
		break;
	}
    case var_solarfield::DES_SIM_DETAIL::EFFICIENCY_MAP__ANNUAL:
	{  //Efficiency map + annual simulation
		V.amb.sim_time_step.Setval(3600.);
		wdatvar->clear();
		
		vector<int> uday;
		vector<vector<double> > utime;
		double
			lat = V.amb.latitude.val*D2R,
			lng = V.amb.longitude.val*D2R,
			tmz = V.amb.time_zone.val,
			dni_des = V.sf.dni_des.val;
		int nday = V.sf.des_sim_ndays.val;
		Ambient::calcSpacedDaysHours(lat, lng, tmz, nday, 1., utime, uday);

		int nflux_sim = 0;
		for(int i=0; i<(int)utime.size(); i++)
			nflux_sim += (int)utime.at(i).size();
		
		DateTime dt;
		double hoy, hod;
		int month, dom;
		for(int i=0; i<nday; i++){
			int doy = uday.at(i);	//because of the doy calcluation used before, this is actually [0..364]
			for(int j=0; j<(int)utime.at(i).size(); j++){
				hod = utime.at(i).at(j)+12.;
				hoy = double( doy ) * 24.;
				dt.hours_to_date( hoy, month, dom );	//midnight on the month/day
								
                wdatvar->append(dom, hod, month, dni_des, 25., 1., 1., 1.0);
			}
		}
		break;

	}
    case var_solarfield::DES_SIM_DETAIL::LIMITED_ANNUAL_SIMULATION:
    case var_solarfield::DES_SIM_DETAIL::REPRESENTATIVE_PROFILES:
	case -1:  //for optimization
	{	//5) Limited annual simulation=4 || Representative profiles=5
		V.amb.sim_time_step.Setval(0.);  //calculate sun position at time specified
		wdatvar->clear();

		//Datetime object
		DateTime dt;
		double
			lat = V.amb.latitude.val,
			lng = V.amb.longitude.val,
			tmz = V.amb.time_zone.val;

		int nday, nskip;
		if( design_method == -1 ){
			nday = 4;
			nskip = 2;
		}
		else{
			nday = V.sf.des_sim_ndays.val;
			nskip = V.sf.des_sim_nhours.val;
		}
		

		double delta_day = 365./float(nday);
		double doffset;
		if(nday%2 == 1){ doffset = 0.; }
		else{ doffset = delta_day/2.; }
		vector<int> simdays(nday,0);
		
		int dinit = 171-(int)Toolbox::round( ( floor(float(nday)/2.) - 1 )*delta_day - doffset );
		int dcalc;
		for(int i=0; i<nday; i++){ 
			dcalc = int( Toolbox::round(dinit + i*delta_day) );
			if(dcalc < 1){ dcalc += 365; }
			else if(dcalc > 365){ dcalc += -365; }
			simdays.at(i) = dcalc - 1;		//Day based on 0 index [0..364]
		}
		//Sort chronologically
		quicksort(simdays, 0, nday-1);

		//Calculate the month and day for each item
		for(int i=0; i<nday; i++)
        {
			int month, dom;
			double hoy;
			int doy = simdays.at(i);	//because of the doy calcluation used before, this is actually [0..364]
			hoy = double( doy ) * 24.;
			dt.hours_to_date( hoy, month, dom );	//midnight on the month/day

			//---Get some info on the weather for each day---
			
			//Calculate the start and end of daytime hours
			dt.SetHour(12);
			dt.SetDate(2011,(int)month,(int)dom);
			doy++;	//Now correct for the doy index
			dt.SetYearDay(doy);	
			double hrs[2];
			Ambient::calcDaytimeHours(hrs, lat*D2R, lng*D2R, tmz, dt);
					

			//Add up the total and average DNI's
			double dni, tdry, pres, wind;
			
			double hrmid = (hrs[0] + hrs[1])/2. + hoy;
			int nhrs = (int)(floor((hrs[1] - hrs[0])/(double)nskip))*nskip;

			//make sure the start and end hours are symmetric about solar noon
			double nmidspan = (double)nhrs/2.;
			double hr_st = hrmid - nmidspan; 
			double hr_end = hrmid + nmidspan;
				

			//Handle the "limited annual.." and "representative profile" options differently
			if(design_method == var_solarfield::DES_SIM_DETAIL::LIMITED_ANNUAL_SIMULATION){	//Limited annual simulation
				
                //weighting fractions for DNI
				double fthis = fmin(0.5, hr_st - floor(hr_st)) + fmin(0.5, ceil(hr_st) - hr_st);
				double fcomp = 1.-fthis;

				//for integration, which way should we go?
				int iind = (hr_st - floor(hr_st)) < 0.5 ? -1 : 1;
                
                //preprocess the day's weather
				double jd = hr_st;
				while(jd<hr_end+.001){	//include hr_end
					//index associated with time jd
					int jind = (int)floor(jd);	//the (j-1) originally may have been an error
                    dni = wf_entries_d.at(jind, 3);
                    tdry = wf_entries_d.at(jind, 4);
                    pres = wf_entries_d.at(jind, 5);
                    wind = wf_entries_d.at(jind, 6);

					//Calculate weighting factor for this hour
					double hod = fmod(jd,24.);
					double step_weight;
					if(jd==hr_st){
						step_weight = hod - hrs[0] + nskip/2.;
					}
					else if(jd + nskip < hr_end + 0.001){
						step_weight = nskip;
					}
					else{
						step_weight = hrs[1] - hod + nskip/2.;
					}
					step_weight *= Toolbox::round(delta_day);

					//calculate the adjusted DNI based on the time surrounding the simulation position
					double dnimod, dnicomp;
					if(iind > 0)
                        dnicomp = wf_entries_d.at(min(8759, jind + 1));
					else
                        dnicomp = wf_entries_d.at(max(0,jind-1));

					dnimod = dni*fthis + dnicomp * fcomp;

					wdatvar->append(int(dom), hod, int(month), dnimod, tdry, pres, wind, step_weight);
					
                    jd += (double)nskip;	

				}

			}
			else	//Representative profile
			{
				//For each base day, loop through the days within the profile range. Average all values within the range.
				
				//Based on the simdays array, calculate the starting and ending day to include in the range
				int simprev;
				if(i==0) simprev = simdays.back();
				else simprev = simdays.at(i-1);
				int simnext;
				if(i==nday-1) simnext = simdays.at(0);
				else simnext = simdays.at(i+1);

				int dprev = 
					simprev > simdays.at(i) ? simdays.at(i)-(simprev-365) : simdays.at(i)-simprev;
				int dnext =
					simnext < simdays.at(i) ? simnext+365 - simdays.at(i) : simnext - simdays.at(i);

				int range_start = -dprev/2; //simdays.at(i)-dprev/2;
				int range_end = dnext/2; //simdays.at(i)+dnext/2;
				int range = range_end - range_start;

				//weighting fractions for DNI
				double fthis = fmin(0.5, hr_st - floor(hr_st)) + fmin(0.5, ceil(hr_st) - hr_st);
				double fcomp = 1.-fthis;

				//for integration, which way should we go?
				int iind = (hr_st - floor(hr_st)) < 0.5 ? -1 : 1;

				int dayind=0;
				int nwf = (int)wf_entries.size();
				double dnicomp;
				double jd=hr_st;

				while(jd < hr_end + 0.001){
					double tdry_per = 0., pres_per = 0., wind_per = 0., dni_per = 0., dni_per2 = 0.;
					for(int k=range_start; k<range_end; k++){
						int ind = (int)floor(jd)+k*24;
						if(ind < 0) ind += 8760;
						if(ind > 8759) ind += -8760;

                        dni = wf_entries_d.at(ind, 3);
                        tdry = wf_entries_d.at(ind, 4);
                        pres = wf_entries_d.at(ind, 5);
                        wind = wf_entries_d.at(ind, 6);

						//get the complement dni data
                        dnicomp = wf_entries_d.at(min(max(ind + iind, 0), nwf - 1), 3);

						dni_per += dni * fthis + dnicomp * fcomp;
						tdry_per += tdry;
						pres_per += pres;
						wind_per += wind;
					}

					dni_per = (dni_per + dni_per2)/(double)range;
					tdry_per *= 1./(double)range;
					pres_per *= 1./(double)range;
					wind_per *= 1./(double)range;
					//daily peak dni and total daily dni
					//dni_tot += dni_per/1000.;
					//if(dni_per > dni_peak) dni_peak = dni_per;

					//day, hour, month, dni, tdry, pres, wind
					double hod = fmod(jd,24.);

					//Calculate weighting factor for this hour
					double step_weight;
					if(jd==hr_st){
						step_weight = hod - hrs[0] + nskip/2.;
					}
					else if(jd + nskip < hr_end + 0.001){
						step_weight = nskip;
					}
					else{
						step_weight = hrs[1] - hod + nskip/2.;
					}
					step_weight *= (double)range;

					wdatvar->append(int(dom), hod, int(month), dni_per, tdry_per, pres_per, wind_per, step_weight);

					dayind++;
					jd+=nskip;
				}
			}
		}
		break;
	}
	default:
		break;
	}
}

void interop::GenerateSimulationWeatherData(var_map &vset, int design_method, vector<string> &wf_entries)
{
	/* 
	OVERLOAD to support simple vector<string> type.
	
	Calculate and fill the weather data steps needed for simulation and the associated time step.

	The weather data is filled in the variable set vset["solarfield"][0]["sim_step_data"].value

	wf_entries consists of a list strings corresponding to each time step. 
	Each string is comma-separated and has the following entries:
	day, hour, month,  dni, tdry, pres, wspd
	1..,  0..,  1-12, W/m2,    C,  bar,  m/s
	*/

	//create an array string and call the main method
	ArrayString wfdat;
	for(int i=0; i<(int)wf_entries.size(); i++)
		wfdat.Add( wf_entries.at(i) );

	interop::GenerateSimulationWeatherData(vset, design_method, wfdat);


}

bool interop::parseRange(string &range, int &rangelow, int &rangehi, bool &include_low, bool &include_hi){
	//take range string of form:
	// {dlow,dhi} 
	// where {} can be replaced by ( ), [ ], ( ], [ )

	//parse the string
	vector<string> t1 = split(range, ",");
	if(t1.size()<2) return false;

	string lop, rop, ops, ls, rs;
	ls = t1.at(0);
	rs = t1.at(1);
	lop = ls.at(0);
	rop = rs.at(rs.size()-1);
	//Convert range values to integers
	to_integer(ls.erase(0,1), &rangelow);
	to_integer(rs.erase(rs.size()-1,1), &rangehi);

	ops = lop+rop;
	if(ops == " "){return false;}	//no info, don't check

	if(lop == "(") include_low = false;
	else include_low = true;
	if(rop == ")") include_hi = false;
	else include_hi = true;

	return true;

}

void interop::ticker_initialize(int indices[], int n){
	for(int i=0; i<n; i++)
		indices[i]=0;
	
}

bool interop::ticker_increment(int lengths[], int indices[], bool changed[], int n){
	/* 
	take an index array 'indices[len = n]' with maximum lengths 'lengths[len =n]' and increase
	the indices by '1'. The indices work like a scrolling counter. For example:
	if: lengths = {3,2,2}
	1	|	indices = {0,0,0}
	-> increment
	2	|	indices = {0,0,1}
	-> increment
	3	|	indices = {0,1,0}
	-> increment
	4	|	indices = {0,1,1}
	etc... 

	When initialized, indices are as shown in step 1.

	Returns false if the ticker has been exhausted
	*/

	//initialize 'changed' array
	for(int i=0; i<n; i++) changed[i] = false;

	//increment 
	bool inc_next=true;
	bool complete = false;
	for(int i=n-1; i>-1; i+=-1){
		if(inc_next){
			indices[i]++;
			changed[i] = true;

			//check for completion
			if(i==0)
				complete = indices[0]==lengths[0];
		}
		inc_next = indices[i] > lengths[i]-1;
		if(! inc_next) break;
		indices[i] = 0;
	}

	
	return complete;
}

//Simulation methods

bool interop::PerformanceSimulationPrep(SolarField &SF, Hvector &helios){
	/* 
	Call this method when setting up a performance simulation (i.e. a flux simulation).

	This method updates the Receiver flux map structure, the heliostat aim points, and the 
	sun position. Once all are updated, a single performance simulation is executed at the 
	specified sun position or hour/day combo (depending on user input).

	AFTER THIS METHOD:
	(A) In the GUI --- Call either HermiteFluxSimulationHandler() or SolTraceFluxSimulation()
		to do a full performance simulation for the system.
	-or-
	(B) Externally --- Call the SolarField::HermiteFluxSimulation() method to simulate performance.
	*/
	
    var_map *V = SF.getVarMap();

	//Update the receiver surface flux densities
    FluxSimData *fd = SF.getFluxSimObject();
    //make sure simulation data is up to date
    fd->Create(*V);
	Rvector *recs = SF.getReceivers();
	
	for(unsigned int i=0; i<recs->size(); i++){
		recs->at(i)->DefineReceiverGeometry(V->flux.x_res.val, V->flux.y_res.val);
	}

	//update clouds
	double ext[2];
	SF.getLandObject()->getExtents(*V, ext );
	SF.getCloudObject()->Create(*V, ext);
	//if(SF.getCloudObject()->isCloudy()){
	for(int i=0; i<(int)helios.size(); i++){
		double eta_cloud = SF.getCloudObject()->ShadowLoss(*V,  *helios.at(i)->getLocation() );
		helios.at(i)->setEfficiencyCloudiness( eta_cloud );
		//update overall efficiency
		helios.at(i)->calcTotalEfficiency();
	}
	/*--- calculate aim points ---*/


	//need to update the SF sun position before continuing
    double az, zen;
	if(V->flux.flux_time_type.mapval() == var_fluxsim::FLUX_TIME_TYPE::SUN_POSITION){	//Sun position specified
		az = V->flux.flux_solar_az_in.val;
        zen = 90.0 - V->flux.flux_solar_el_in.val;
	}
	else{
		//day and time specified

		//Run a simulation for the specified conditions
		//<day of the month>, <hour of the day>, <month (1-12)>, <dni [W/m2]>,<amb. temperature [C]>, <atm. pressure [atm]>, <wind velocity [m/s]>
	
		int flux_day = V->flux.flux_day.val;
		double flux_hour = V->flux.flux_hour.val;
		int flux_month = V->flux.flux_month.val;
		DateTime DT;
        Ambient::setDateTime(DT, flux_hour, DT.GetDayOfYear(2011, flux_month, flux_day) );
		Ambient::calcSunPosition(*V, DT, &az, &zen); 
	}
    //update the map sun position
    V->flux.flux_solar_az.Setval( az );
    V->flux.flux_solar_el.Setval( 90. - zen );

    sim_params P;
    P.dni = V->flux.flux_dni.val;		// TODO: some dni values are in kW/m2 while this one is in W/m2...
	P.Tamb = 25.; 
	P.Patm = 1.;

	SF.Simulate(az*D2R, zen*D2R, P);

	return !SF.ErrCheck();
			
}

#ifdef SP_USE_SOLTRACE
bool interop::SolTraceFluxSimulation_ST(st_context_t cxt, int seed, ST_System &ST, 
										int callback(st_uint_t ntracedtotal, st_uint_t ntraced, st_uint_t ntotrace, st_uint_t curstage, st_uint_t nstages, void *data),
										void *par,
                                        vector<vector<double> > *st0data, vector<vector<double> > *st1data, bool save_stage_data, bool load_stage_data)
{
	/* 
	This method requires that a SolTrace context "st_context_t" has already been created and passed to 
	the method. 
	Create by calling:
	st_context_t cxt = st_create_context();

	This method sets up and executes a single-threaded SolTrace simulation. 
	Returns status (error = false, no error = true)

	Passes an optional pointer to a callback function that updates the GUI.
	*/	
	//int minrays = ST.sim_raycount;
	//int maxrays = ST.sim_raymax;
	
	//simulate, setting the UI callback and a pointer to the UI class
	
	st_sim_params( cxt, ST.sim_raycount, ST.sim_raymax, ST.sim_dynamic_group);
    //if(load_stage_data)
        //return st_sim_run_data(cxt, seed, st0data, st1data, false, callback, par) != -1;
    //else if(save_stage_data)
        //return st_sim_run_data(cxt, seed, st0data, st1data, true, callback, par) != -1;
    //else
	    return st_sim_run(cxt, seed, callback, par) != -1;
}


bool interop::SolTraceFluxSimulation_ST(st_context_t cxt, SolarField &SF, Hvector &helios, Vect &sunvect,
							   int callback(st_uint_t ntracedtotal, st_uint_t ntraced, st_uint_t ntotrace, st_uint_t curstage, st_uint_t nstages, void *data),
							   void *par,
                               vector<vector<double> > *st0data, vector<vector<double> > *st1data, bool save_stage_data, bool load_stage_data)
{
	//Overload to be called when maintaining ST_System is not important
	
	ST_System STSim;
	STSim.CreateSTSystem(SF, helios, sunvect);
	ST_System::LoadIntoContext(&STSim, cxt);
	int seed = SF.getFluxObject()->getRandomObject()->integer();
	return SolTraceFluxSimulation_ST(cxt, seed, STSim, callback, par, st0data, st1data, save_stage_data, load_stage_data);
}
#endif

void interop::UpdateMapLayoutData(var_map &V, Hvector *heliostats){
//Fill in the data
	int npos = (int)heliostats->size();
	Heliostat *H;

	string *var = &V.sf.layout_data.val;
	var->clear();
	string sdat; //, sdat2, sdat3, sdat4;
	
	for(int i=0; i<npos; i++){
		/* Fill in the data for each heliostat in the template */


		H = heliostats->at(i);	//shorthand the pointer

		sp_point *loc = H->getLocation();
		Vect *cant = H->getCantVector();
		sp_point *aim = H->getAimPoint();

		//Save the layout to the variable maps
		//Take special care for user-specified values vs. program calculated values.
		char tchar1[300];
		if(H->getVarMap()->focus_method.mapval() == var_heliostat::FOCUS_METHOD::USERDEFINED )
            sprintf(tchar1, "%f,%f", H->getFocalX(), H->getFocalY());
        else 
            sprintf(tchar1, "NULL,NULL");
		//sdat2 = string(tchar1);

		char tchar2[300];
		if(H->IsUserCant()){ sprintf(tchar2, "%f,%f,%f", cant->i, cant->j, cant->k); }
		else{ sprintf(tchar2, "NULL,NULL,NULL"); }
		//sdat3 = string(tchar2);

		char tchar3[300];
		sprintf(tchar3, "%f,%f,%f", aim->x, aim->y, aim->z);
		//sdat4 = string(tchar3);

		char tchar4[300];
		sprintf(tchar4, "%d,%d,%d,%f,%f,%f,%s,%s,%s\n",H->getVarMap()->id.val, H->IsEnabled() ? 1 : 0, H->IsInLayout() ? 1 : 0, loc->x, loc->y, loc->z, tchar1, tchar2, tchar3);
		sdat = string(tchar4);
		var->append(sdat);

	}

}

bool interop::HermiteFluxSimulationHandler(sim_results& results, SolarField& SF, Hvector& helios)
{
	/*
	Call the Hermite flux evaluation algorithm and process.
	*/
	SF.HermiteFluxSimulation(helios,
		SF.getVarMap()->flux.aim_method.mapval() == var_fluxsim::AIM_METHOD::IMAGE_SIZE_PRIORITY    //to not re-simulate, aim strategy must be "IMAGE_SIZE"...
		&& helios.size() == SF.getHeliostats()->size());											//and all heliostats must be included.

	//Process the results
	double azzen[2];
	azzen[0] = D2R * SF.getVarMap()->flux.flux_solar_az.Val();
	azzen[1] = D2R * (90. - SF.getVarMap()->flux.flux_solar_el.Val());

	sim_params P;
	P.dni = SF.getVarMap()->flux.flux_dni.val;

	results.back().process_analytical_simulation(SF, P, 2, azzen, &helios);

	//if we have more than 1 receiver, create performance summaries for each and append to the results vector
	if (SF.getActiveReceiverCount() > 1)
	{
		//which heliostats are aiming at which receiver?
		unordered_map<Receiver*, Hvector> aim_map;
		for (Hvector::iterator h = helios.begin(); h != helios.end(); h++)
			aim_map[(*h)->getWhichReceiver()].push_back(*h);

		for (Rvector::iterator rec = SF.getReceivers()->begin(); rec != SF.getReceivers()->end(); rec++)
		{
			results.push_back(sim_result());
			Rvector recs = { *rec };
			results.back().process_analytical_simulation(SF, P, 2, azzen, &aim_map[*rec], &recs);
		}
	}

	return true;
}

#ifdef SP_USE_SOLTRACE
bool interop::SolTraceFluxSimulation(SimControl& SimC, sim_results& results, SolarField& SF, var_map& vset, Hvector& helios)
{
	/*
	Send geometry to Soltrace and get back simulation results.
	From Soltrace library, reference "runthreads.cpp" and "sysdata.cpp"

	Note that the SolTrace coordinate system for defining the sun position is typically:
	X -> positive west
	Y -> positive toward zenith
	Z -> positive north
	.. However ..
	We will use a coordinate system consistent with SolarPILOT geometry where:
	X -> positive east
	Y -> positive north
	Z -> positive zenith

	As long as the sun position vector passed to SolTrace is consistent with the field geometry
	that we're using, the results will be correct.

	*/
	bool is_load_raydata = vset.flux.is_load_raydata.val;
	bool is_save_raydata = vset.flux.is_save_raydata.val;
	//raydata_file
	std::string raydata_file = vset.flux.raydata_file.val;

	//check that the file exists
	vector<vector<double> > raydat_st0;
	vector<vector<double> > raydat_st1;
	int nsunrays_loadst = 0;
	if (is_load_raydata)
	{
		if (!ioutil::file_exists(raydata_file.c_str()))
			throw spexception("Specified ray data file does not exist. Looking for file: " + raydata_file);

		//Load the ray data from a file
		ifstream fdat(raydata_file);

		if (fdat.is_open())
		{

			string str;
			str.reserve(96);

			char line[96];
			bool nextloop = false;
			bool firstline = true;
			while (fdat.getline(line, 96))
			{
				//first line is number of sun rays
				if (firstline)
				{
					for (int i = 0; i < 16; i++)
					{
						if (line[i] == '\n' || i == 15)
						{
							to_integer(str, &nsunrays_loadst);
							str.clear();
							str.reserve(96);
							break;
						}
						else
						{
							str.push_back(line[i]);
						}
					}
					firstline = false;
					continue;
				}


				vector<double> dat(8);


				int ilast = 0;

				for (int i = 0; i < 96; i++)
				{
					//check for the transition character
					if (line[i] == '#')
					{
						nextloop = true;
						break;
					}

					if (line[i] == ',')
					{
						to_double(str, &dat[ilast++]);

						//clear the string
						str.clear();
						str.reserve(96);
						//if this was the 8th entry, go to next line
						if (ilast > 7)
							break;
					}
					else
					{
						str.push_back(line[i]);
					}
				}
				ilast = 0;

				if (nextloop) break;

				raydat_st0.push_back(dat);
			}

			//next loop to get stage 1 input rays
			while (fdat.getline(line, 96))
			{
				vector<double> dat(7);

				int ilast = 0;
				//int istr = 0;
				for (int i = 0; i < 96; i++)
				{
					if (line[i] == ',')
					{
						to_double(str, &dat[ilast++]);

						//clear the string
						str.clear();
						str.reserve(96);
						//if this was the 8th entry, go to next line
						if (ilast > 6)
							break;
					}
					else
					{
						str.push_back(line[i]);
					}
				}
				ilast = 0;

				raydat_st1.push_back(dat);
			}

			fdat.close();
		}

		//set the number of traced rays based on the length of the supplied data
		int nray = (int)raydat_st0.size();

		vset.flux.min_rays.val = nray;
	}
	//for saving, check that the specified directory exists. If none specified or if it doesn't exist, prepend the working directory.
	//TODO: Remove SPFrame and wxString dependence
	//if (is_save_raydata)
	//{
	//	if (!raydata_file.DirExists())
	//		raydata_file = _working_dir.GetPath(true) + raydata_file.GetName();
	//}

	bool err_maxray = false;
	int minrays, maxrays;
	vector<st_context_t> contexts;

	SimC._stthread = 0;    //initialize to null

	//get sun position and create a vector
	double el = vset.flux.flux_solar_el.Val() * D2R;
	double az = vset.flux.flux_solar_az.Val() * D2R;
	Vect sun = Ambient::calcSunVectorFromAzZen(az, PI / 2. - el);

	SimC._STSim = new ST_System;

	SimC._STSim->CreateSTSystem(SF, helios, sun);

	minrays = SimC._STSim->sim_raycount;
	maxrays = SimC._STSim->sim_raymax;

	vector< vector<vector< double > >* > st0datawrap;
	vector< vector<vector< double > >* > st1datawrap;

	if (SimC._n_threads > 1)
	{
		//Multithreading support
		SimC._stthread = new STSimThread[SimC._n_threads];
		SimC._is_mt_simulation = true;

		int rays_alloc = 0;
		int rays_alloc1 = 0;
		for (int i = 0; i < SimC._n_threads; i++)
		{
			//declare soltrace context
			st_context_t pcxt = st_create_context();
			//load soltrace data structure into context
			ST_System::LoadIntoContext(SimC._STSim, pcxt);
			//get random seed
			int seed = SF.getFluxObject()->getRandomObject()->integer();
			//setup the thread
			SimC._stthread[i].Setup(pcxt, i, seed, is_load_raydata, is_save_raydata);

			////Decide how many rays to trace for each thread. Evenly divide and allocate remainder to thread 0
			int rays_this_thread = SimC._STSim->sim_raycount / SimC._n_threads;
			if (i == 0) rays_this_thread += (SimC._STSim->sim_raycount % SimC._n_threads);
			//when loading ray data externally, we need to divide up receiver stage hits
			int rays_this_thread1 = raydat_st1.size() / SimC._n_threads;     //for receiver stage input rays
			if (i == 0) rays_this_thread1 += (raydat_st1.size() % SimC._n_threads);

			//if loading ray data, add by thread here
			if (is_load_raydata)
			{
				SimC._stthread[i].CopyStageRayData(raydat_st0, 0, rays_alloc, rays_alloc + rays_this_thread);
				SimC._stthread[i].CopyStageRayData(raydat_st1, 1, rays_alloc1, rays_alloc1 + rays_this_thread1);   //for receiver stage input rays
			}
			rays_alloc += rays_this_thread;
			rays_alloc1 += rays_this_thread1;

			st_sim_params(pcxt, rays_this_thread, SimC._STSim->sim_raymax / SimC._n_threads, SimC._STSim->sim_dynamic_group);
		}

		for (int i = 0; i < SimC._n_threads; i++)
		{
			thread(&STSimThread::StartThread, std::ref(SimC._stthread[i])).detach();
		}
		int ntotal = 0, ntraced = 0, ntotrace = 0, stagenum = 0, nstages = 0;

		// every now and then query the threads and update the UI;
		while (1)
		{
			int num_finished = 0;
			for (int i = 0; i < SimC._n_threads; i++)
				if (SimC._stthread[i].IsFinished())
					num_finished++;

			if (num_finished == SimC._n_threads)
				break;

			// threads still running so update interface
			int ntotaltraces = 0;
			for (int i = 0; i < SimC._n_threads; i++)
			{
				SimC._stthread[i].GetStatus(&ntotal, &ntraced, &ntotrace, &stagenum, &nstages);
				ntotaltraces += ntotal;
			}

			SimC.soltrace_callback(ntotal, ntraced, ntotrace, stagenum, nstages, SimC.soltrace_callback_data);

			// if dialog's cancel button was pressed, send cancel signal to all threads
			if (SimC._cancel_simulation)
			{
				for (int i = 0; i < SimC._n_threads; i++)
					SimC._stthread[i].CancelTrace();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(75));
		}

		// determine if any errors occurred
		bool errors_found = false;
		contexts.clear();
		for (int i = 0; i < SimC._n_threads; i++)
		{
			contexts.push_back(SimC._stthread[i].GetContextId());
			int code = SimC._stthread[i].GetResultCode();

			if (code < 0)
			{
				errors_found = true;
				err_maxray = true;
				std::string msg("Error occured in trace core thread" + std::to_string(i + 1) + ", code=" + std::to_string(code) + ".\n\n");
				SimC.message_callback(msg.c_str(), SimC.message_callback_data);
				break;  //Don't keep displaying if there's an error
			}
		}

		//Consolidate the stage 0 ray data if needed
		if (is_save_raydata && !errors_found)
		{
			for (int i = 0; i < SimC._n_threads; i++)
			{
				st0datawrap.push_back(SimC._stthread[i].GetStage0RayDataObject());
				st1datawrap.push_back(SimC._stthread[i].GetStage1RayDataObject());
			}
		}
	}
	else
	{
		//Create context
		st_context_t cxt = st_create_context();
		int seed = SF.getFluxObject()->getRandomObject()->integer();
		ST_System::LoadIntoContext(SimC._STSim, cxt);

		//Passes an optional pointer to a callback function that updates the GUI.
		int minrays = SimC._STSim->sim_raycount;
		int maxrays = SimC._STSim->sim_raymax;

		//simulate, setting the UI callback and a pointer to the UI class
		st_sim_params(cxt, SimC._STSim->sim_raycount, SimC._STSim->sim_raymax, SimC._STSim->sim_dynamic_group);

		bool sim_result = st_sim_run(cxt, seed, SimC.soltrace_callback, SimC.soltrace_callback_data) != -1;
		if(sim_result)
			contexts.push_back(cxt);
		else
			err_maxray = true;  //hit max ray limit if function returns false

		if (is_save_raydata && !err_maxray)
		{
			st0datawrap.push_back(&raydat_st0);
			st1datawrap.push_back(&raydat_st1);
		}
	}

	//reset the progress gauge
	SimC._is_mt_simulation = false;
	//_flux_gauge->SetValue(0);		//TODO: is this important?
	//Did the simulation terminate after reaching the max ray count?
	if (err_maxray)
	{
		std::string msg = "The simulation has reached the maximum number of rays (" + my_to_string(maxrays) +
			") without achieving the required number of ray hits (" + my_to_string(minrays) + ")." +
			" Consider increasing the 'Maximum number of generated rays' or decreasing the " +
			" 'Desired number of ray intersections'.";
		SimC.message_callback( msg.c_str(), SimC.message_callback_data);

		return false;
	}
	//Was the simulation cancelled during st_sim_run()?
	if (SimC._cancel_simulation)
	{
		SimC._cancel_simulation = false; //reset
		return false;
	}

	//Process the results
	int nint = 0;
	int nc = contexts.size();
	vector<int> csizes;
	for (int i = 0; i < nc; i++)
	{
		csizes.push_back(st_num_intersections(contexts.at(i)));
		nint += csizes.at(i);
	}

	double bounds[5]; //xmin, xmax, ymin, ymax, empty
	SimC._STSim->IntData.nsunrays = 0;

	SimC._STSim->IntData.AllocateArrays(nint);

	//Collect all of the results
	int ind = 0;
	int rstart = 0;
	for (int i = 0; i < nc; i++)
	{
		int cs = csizes.at(i);

		st_locations(contexts.at(i), &SimC._STSim->IntData.hitx[ind], &SimC._STSim->IntData.hity[ind], &SimC._STSim->IntData.hitz[ind]);
		st_cosines(contexts.at(i), &SimC._STSim->IntData.cosx[ind], &SimC._STSim->IntData.cosy[ind], &SimC._STSim->IntData.cosz[ind]);
		st_elementmap(contexts.at(i), &SimC._STSim->IntData.emap[ind]);
		st_stagemap(contexts.at(i), &SimC._STSim->IntData.smap[ind]);
		st_raynumbers(contexts.at(i), &SimC._STSim->IntData.rnum[ind]);

		// Make rays numbers unique by adding the previous context max ray number
		if (i != 0) {
			int c_max_rays = 0;
			for (int j = 0; j < cs; j++) {
				if (SimC._STSim->IntData.smap[ind + j] == 2) { // hit stage 2
					c_max_rays = SimC._STSim->IntData.rnum[ind + j - 1]; //max ray number was last ray of stage 1
					break; // once we hit stage 2 we're done
				}
			}
			rstart += c_max_rays; // Update ray numbers
			for (int j = 0; j < cs; j++) {
				SimC._STSim->IntData.rnum[ind + j] += rstart;
			}
		}

		int nsr;
		st_sun_stats(contexts.at(i), &bounds[0], &bounds[1], &bounds[2], &bounds[3], &nsr);    //Bounds should always be the same
		SimC._STSim->IntData.nsunrays += nsr;
		ind += cs;
	}

	//DNI
	double dni = vset.flux.flux_dni.val / 1000.;   //[kw/m2]
	

	//if the heliostat field ray data is loaded from a file, just specify the number of sun rays based on this value
	if (is_load_raydata)
		SimC._STSim->IntData.nsunrays = nsunrays_loadst;

	//Get bounding box and sun ray information to calculate power per ray
	SimC._STSim->IntData.q_ray = (bounds[1] - bounds[0]) * (bounds[3] - bounds[2]) / float(SimC._STSim->IntData.nsunrays) * dni;

	bool skip_receiver = false;
	if (!skip_receiver)
	{

		bounds[4] = (float)SimC._STSim->IntData.nsunrays;

		for (int i = 0; i < 5; i++)
			SimC._STSim->IntData.bounds[i] = bounds[i];
		SolTraceFluxBinning(SimC, SF);


		//Process the results
		sim_params P;
		P.dni = dni;
		double azzen[2] = { az, PI / 2. - el };
		results.back().process_raytrace_simulation(SF, P, 2, azzen, helios, SimC._STSim);
	}

	//If the user wants to save stage0 ray data, do so here
	if (is_save_raydata)
	{
		ofstream fout(raydata_file);
		fout.clear();
		//first line is number of sun rays
		fout << SimC._STSim->IntData.nsunrays << "\n";
		//write heliostat IN stage
		for (int i = 0; i < (int)st0datawrap.size(); i++)
		{
			for (int j = 0; j < (int)st0datawrap.at(i)->size(); j++)
			{
				for (int k = 0; k < 8; k++)
					fout << st0datawrap.at(i)->at(j).at(k) << ",";
				fout << "\n";
			}
		}
		//special separator
		fout << "#\n";
		//write receiver IN stage
		for (int i = 0; i < (int)st1datawrap.size(); i++)
		{
			for (int j = 0; j < (int)st1datawrap.at(i)->size(); j++)
			{
				for (int k = 0; k < 7; k++)
					fout << st1datawrap.at(i)->at(j).at(k) << ",";
				fout << "\n";
			}
		}

		fout.close();
	}

	//If the user wants to save the ray data, do so here
	if (vset.flux.save_data.val)
	{
		string fname = vset.flux.save_data_loc.val;
		if (fname == "")
		{
			std::string msg = "Notice: Ray data was not saved. No file was specified.";
			SimC.message_callback(msg.c_str(), SimC.message_callback_data);
		}
		else
		{
			FILE* file = fopen(fname.c_str(), "w");
			if (!file)
			{
				std::string msg = "File Error: Error opening the flux simulation output file. Please make sure the file is closed and the directory is not write-protected.";
				SimC.message_callback(msg.c_str(), SimC.message_callback_data);
				return false;
			}
			fprintf(file, "Pos X, Pos Y, Pos Z, Cos X, Cos Y, Cos Z, Element Map, Stage Map, Ray Number\n");
			for (int i = 0; i < nint; i++)
			{
				fprintf(file, "%.3f, %.3f, %.3f, %.7f, %.7f, %.7f, %d, %d, %d\n",
					SimC._STSim->IntData.hitx[i], SimC._STSim->IntData.hity[i], SimC._STSim->IntData.hitz[i],
					SimC._STSim->IntData.cosx[i], SimC._STSim->IntData.cosy[i], SimC._STSim->IntData.cosz[i],
					SimC._STSim->IntData.emap[i], SimC._STSim->IntData.smap[i], SimC._STSim->IntData.rnum[i]);
			}
			std::fclose(file);  // REMOVE: fclose(file);
		}

	}
	//Clean up
	SimC._STSim->IntData.DeallocateArrays();
	if (SimC._stthread != 0) delete[] SimC._stthread;

	return true;
}

bool interop::SolTraceFluxBinning(SimControl& SimC, SolarField& SF)
{
	//Collect all of the rays that hit the receiver(s) into the flux profile
	int rstage1 = SimC._STSim->StageList.size();

	for (int r = 0; r < (int)SF.getReceivers()->size(); r++)
	{
		Receiver* Rec = SF.getReceivers()->at(r);
		if (!Rec->isReceiverEnabled())
			continue;
		var_receiver* RV = Rec->getVarMap();
		var_heliostat* Hv = SF.getHeliostats()->front()->getVarMap();

		//.. for each receiver, 
		int recgeom = Rec->getGeometryType();

		//Pre-declare all relevant variables
		FluxSurface* fs;
		FluxGrid* fg;
		int e_ind, nfx, nfy,
			ibin, jbin;    //indices of the flux grid bin that the current ray will go into
		double rel, raz, rh, rw, paz, ph, pw, Arec, dqspec;
		Vect rayhit;
		//----------


		switch (recgeom)
		{
		case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CLOSED:        //0    |    Continuous closed cylinder - external
		{
			//There will be only one flux surface and flux grid. Get both objects.
			fs = &Rec->getFluxSurfaces()->at(0);
			fs->ClearFluxGrid();
			fg = fs->getFluxMap();
			e_ind = r + 1;    //element index for this receiver

			rel = RV->rec_elevation.val * D2R;
			raz = RV->rec_azimuth.val * D2R;
			rh = RV->rec_height.val;

			sp_point offset(RV->rec_offset_x_global.Val(), RV->rec_offset_y_global.Val(), RV->optical_height.Val());   //optical height includes z offset

			//The number of points in the flux grid 
			//(x-axis is angular around the circumference, y axis is receiver height)
			nfx = fs->getFluxNX();
			nfy = fs->getFluxNY();

			Arec = Rec->getAbsorberArea();
			dqspec = SimC._STSim->IntData.q_ray * Hv->reflect_ratio.val / Arec * (float)(nfx * nfy);

			for (int j = 0; j < SimC._STSim->IntData.nint; j++)
			{    //loop through each intersection

				if (SimC._STSim->IntData.smap[j] != rstage1 || std::abs(SimC._STSim->IntData.emap[j]) != e_ind) continue;    //only consider rays that interact with this element

				//Where did the ray hit relative to the location of the receiver?
				rayhit.Set(SimC._STSim->IntData.hitx[j] - offset.x, SimC._STSim->IntData.hity[j] - offset.y, SimC._STSim->IntData.hitz[j] - offset.z);

				//Do any required transform to get the ray intersection into receiver coordinates
				Toolbox::rotation(-raz, 2, rayhit);
				Toolbox::rotation(-rel, 0, rayhit);

				//Calculate the point location in relative cylindrical coorinates
				paz = 0.5 - atan2(rayhit.i, rayhit.j) / (2. * PI);    //0 for the flux grid begins at <S>, progresses CCW to 1
				ph = 0.5 + rayhit.k / rh;    //0 for flux grid at bottom of the panel, 1 at top

				//Calculate which bin to add this ray to in the flux grid
				ibin = int(floor(paz * nfx));
				jbin = int(floor(ph * nfy));

				//Add the magnitude of the flux to the correct bin
				fg->at(ibin).at(jbin).flux += dqspec;

			}

			break;
		}
		case Receiver::REC_GEOM_TYPE::CYLINDRICAL_OPEN:
		case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CAV:
			break;
		case Receiver::REC_GEOM_TYPE::PLANE_RECT:    //3    |    Planar rectangle
		{
			//Only one flux surface and grid per receiver instance
			fs = &Rec->getFluxSurfaces()->at(0);
			fs->ClearFluxGrid();
			fg = fs->getFluxMap();
			e_ind = r + 1;    //element index for this receiver

			rel = RV->rec_elevation.val * D2R;
			raz = RV->rec_azimuth.val * D2R;
			rh = RV->rec_height.val;
			rw = Rec->getReceiverWidth(*RV);

			sp_point offset(RV->rec_offset_x_global.Val(), RV->rec_offset_y_global.Val(), RV->optical_height.Val());   //optical height includes z offset

			//The number of points in the flux grid 
			//(x-axis receiver width, y axis is receiver height)
			nfx = fs->getFluxNX();
			nfy = fs->getFluxNY();

			Arec = Rec->getAbsorberArea();
			dqspec = SimC._STSim->IntData.q_ray * Hv->reflect_ratio.val / Arec * (float)(nfx * nfy);


			for (int j = 0; j < SimC._STSim->IntData.nint; j++)
			{    //loop through each intersection

				if (SimC._STSim->IntData.smap[j] != rstage1 || std::abs(SimC._STSim->IntData.emap[j]) != e_ind) continue;    //only consider rays that interact with this element

				//Where did the ray hit relative to the location of the receiver?
				rayhit.Set(SimC._STSim->IntData.hitx[j] - offset.x, SimC._STSim->IntData.hity[j] - offset.y, SimC._STSim->IntData.hitz[j] - offset.z);

				//Do any required transform to get the ray intersection into receiver coordinates
				Toolbox::rotation(PI - raz, 2, rayhit);
				Toolbox::rotation(-rel, 0, rayhit);

				//Calculate the point location in relative cylindrical coorinates
				pw = 0.5 + rayhit.i / rw;    //0 at "starboard" side, increase towards "port"
				ph = 0.5 + rayhit.k / rh;    //0 for flux grid at bottom of the panel, 1 at top

				//Calculate which bin to add this ray to in the flux grid
				ibin = int(floor(pw * nfx));
				jbin = int(floor(ph * nfy));

				//Add the magnitude of the flux to the correct bin
				fg->at(ibin).at(jbin).flux += dqspec;

			}
			break;
		}
		case Receiver::REC_GEOM_TYPE::FALL_FLAT:
		case Receiver::REC_GEOM_TYPE::FALL_CURVE:
		{
			// Aperture window
			int apstage = SimC._STSim->StageList.size() - 1; // Aperture is the penultimate stage
			fs = &Rec->getFluxSurfaces()->at(0);
			fs->ClearFluxGrid();
			fg = fs->getFluxMap();

			rel = RV->rec_elevation.val * D2R;
			raz = RV->rec_azimuth.val * D2R;
			rh = RV->rec_height.val;
			rw = RV->rec_width.val;

			//The number of points in the flux grid 
			//(x-axis receiver width, y axis is receiver height)
			nfx = fs->getFluxNX();
			nfy = fs->getFluxNY();

			Arec = rh*rw; // aperture area
			dqspec = SimC._STSim->IntData.q_ray * Hv->reflect_ratio.val / Arec * (float)(nfx * nfy);

			for (int j = 0; j < SimC._STSim->IntData.nint; j++)
			{    //loop through each intersection

				if (SimC._STSim->IntData.smap[j] != apstage || abs(SimC._STSim->IntData.emap[j]) != 1) continue;    //only consider rays that interact with this element

				//Where did the ray hit relative to the location of the receiver?
				rayhit.Set(SimC._STSim->IntData.hitx[j], SimC._STSim->IntData.hity[j], SimC._STSim->IntData.hitz[j]);
				// Hit position is in stage coordinates which already account for offset and azimuth

				//Calculate the point location in relative cylindrical coorinates
				pw = 0.5 + rayhit.i / rw;    //0 at "starboard" side, increase towards "port"
				ph = 0.5 + rayhit.k / rh;    //0 for flux grid at bottom of the panel, 1 at top

				//Calculate which bin to add this ray to in the flux grid
				ibin = int(floor(pw * nfx));
				jbin = int(floor(ph * nfy));

				//Add the magnitude of the flux to the correct bin
				fg->at(ibin).at(jbin).flux += dqspec;
			}

			// Get curtain surface flux
			int n_panels = RV->n_panels.val;
			// Loop through curtain surfaces
			for (int i = 1; i <= n_panels; i++) {
				fs = &Rec->getFluxSurfaces()->at(i);

				nfx = fs->getFluxNX();
				nfy = fs->getFluxNY();

				Arec = fs->getSurfaceArea();
				dqspec = SimC._STSim->IntData.q_ray * Hv->reflect_ratio.val / Arec * (float)(nfx * nfy);

				fs->ClearFluxGrid();
				fg = fs->getFluxMap();

				e_ind = i; //element index for this receiver
			
				rel = 0.0;
				raz = RV->rec_azimuth.val * D2R;

				rh = fs->getSurfaceHeight();
				rw = RV->max_curtain_width.Val();

				sp_point offset = *fs->getSurfaceOffset();
				double curtain_depth = sqrt(pow(offset.x, 2) + pow(offset.y, 2));

				for (int j = 0; j < SimC._STSim->IntData.nint; j++) // TODO: Looping through interactions multiple times is inefficient
				{    //loop through each intersection
					// Flux map is incident flux not absorbed
					if (SimC._STSim->IntData.smap[j] != rstage1 || abs(SimC._STSim->IntData.emap[j]) != e_ind) continue;    //only consider rays that interact with this element

					//Where did the ray hit relative to the location of the receiver?
					rayhit.Set(SimC._STSim->IntData.hitx[j], SimC._STSim->IntData.hity[j] + curtain_depth, SimC._STSim->IntData.hitz[j] - offset.z);

					//Calculate the point location in relative cylindrical coorinates
					pw = 0.5 + rayhit.i / rw;    //0 at "starboard" side, increase towards "port"
					ph = 0.5 + rayhit.k / rh;    //0 for flux grid at bottom of the panel, 1 at top

					//Calculate which bin to add this ray to in the flux grid
					ibin = int(floor(pw * nfx));
					jbin = int(floor(ph * nfy));

					//Add the magnitude of the flux to the correct bin
					fg->at(ibin).at(jbin).flux += dqspec;
				}
			}
			break;
		}
		case Receiver::REC_GEOM_TYPE::PLANE_ELLIPSE:
		case Receiver::REC_GEOM_TYPE::POLYGON_CLOSED:
		case Receiver::REC_GEOM_TYPE::POLYGON_OPEN:
		case Receiver::REC_GEOM_TYPE::POLYGON_CAV:
		default:
			return false;
			break;
		}
	}
	return true;
}
#endif

bool interop::DoManagedLayout(SimControl& SimC, SolarField& SF, var_map& V, LayoutSimThread* simthread)
{
	/*
	This method is called to create a field layout. The method automatically handles
	multithreading of hourly simulations.

	Call
	SF.Create()
	before passing the solar field to this method.

	*/

	//Make sure the solar field has been created
	if (SF.getVarMap() == 0)
	{
		std::string msg = "Error: The solar field Create() method must be called before generating the field layout.";
		SimC.message_callback(msg.c_str(), SimC.message_callback_data);
		return false;
	}

	//Is it possible to run a multithreaded simulation?
	int nsim_req = SF.calcNumRequiredSimulations();
	std::string msg;
#ifdef SP_USE_THREADS
	if (SimC._n_threads > 1 && nsim_req > 1)
	{
		//More than 1 thread and more than 1 simulation to run

		//Prepare the master solar field object for layout simulation
		WeatherData wdata;
		bool full_sim = SF.PrepareFieldLayout(SF, &wdata);

		//If full simulation is required...
		if (full_sim)
		{

			int nthreads = min(nsim_req, SimC._n_threads);

			//Duplicate SF objects in memory

			msg = "Preparing " + std::to_string(SimC._n_threads) + " threads for simulation";
			SimC.layout_log_callback(0., msg.c_str(), SimC.layout_log_callback_data);

			SolarField **SFarr;
			SFarr = new SolarField*[nthreads];
			for (int i = 0; i < nthreads; i++)
			{
				SFarr[i] = new SolarField(SF);
			}

			//Create sufficient results arrays in memory
			sim_results results;
			results.resize(nsim_req);

			//Calculate the number of simulations per thread
			int npert = (int)ceil((float)nsim_req / (float)nthreads);

			//Create thread objects
			simthread = new LayoutSimThread[nthreads];
			SimC._n_threads_active = nthreads;    //Keep track of how many threads are active
			SimC._is_mt_simulation = true;

			int
				sim_first = 0,
				sim_last = npert;
			for (int i = 0; i < nthreads; i++)
			{
				std::string si = my_to_string(i + 1);
				simthread[i].Setup(si, SFarr[i], &results, &wdata, sim_first, sim_last, false, false);
				sim_first = sim_last;
				sim_last = min(sim_last + npert, nsim_req);
			}

			msg = "Simulating " + std::to_string(nsim_req) + " design hours";
			SimC.layout_log_callback(0., msg.c_str(), SimC.layout_log_callback_data);

			//Run
			for (int i = 0; i < nthreads; i++)
				thread(&LayoutSimThread::StartThread, std::ref(simthread[i])).detach();

			//Wait loop
			while (true)
			{
				int nsim_done = 0, nsim_remain = 0, nthread_done = 0;
				for (int i = 0; i < nthreads; i++)
				{
					if (simthread[i].IsFinished())
						nthread_done++;

					int ns, nr;
					simthread[i].GetStatus(&ns, &nr);
					nsim_done += ns;
					nsim_remain += nr;
				}
				//TODO:  SimProgressUpdateMT(nsim_done, nsim_req); // uses wex
				SimC.layout_log_callback((double)nsim_done / (double)nsim_req, "", SimC.layout_log_callback_data);

				if (nthread_done == nthreads) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(75));
			}

			//Check to see whether the simulation was cancelled
			bool cancelled = false;
			for (int i = 0; i < nthreads; i++)
			{
				cancelled = cancelled || simthread[i].IsSimulationCancelled();
			}

			//check to see whether simulation errored out
			bool errored_out = false;
			for (int i = 0; i < SimC._n_threads; i++)
			{
				errored_out = errored_out || simthread[i].IsFinishedWithErrors();
			}
			if (errored_out)
			{
				//make sure each thread is cancelled
				for (int i = 0; i < SimC._n_threads; i++)
					simthread[i].CancelSimulation();

				//Get the error messages, if any
				string errmsgs;
				for (int i = 0; i < SimC._n_threads; i++)
				{
					for (int j = 0; j < (int)simthread[i].GetSimMessages()->size(); j++)
						errmsgs.append(simthread[i].GetSimMessages()->at(j) + "\n");
				}
				//Display error messages
				if (!errmsgs.empty())
					SimC.message_callback(errmsgs.c_str(), SimC.message_callback_data);
			}

			//Clean up dynamic memory
			for (int i = 0; i < nthreads; i++)
			{
				delete SFarr[i];
			}
			delete[] SFarr;
			delete[] simthread;
			simthread = 0;

			//If the simulation was cancelled per the check above, exit out
			if (cancelled || errored_out)
			{
				return false;
			}

			//For the map-to-annual case, run a simulation here
			if (V.sf.des_sim_detail.mapval() == var_solarfield::DES_SIM_DETAIL::EFFICIENCY_MAP__ANNUAL)
				SolarField::AnnualEfficiencySimulation(V.amb.weather_file.val, &SF, results);

			//Process the results
			SF.ProcessLayoutResults(&results, nsim_req);

		}
	}
	else
#endif
	{
		SimC._n_threads_active = 1;
		SimC._is_mt_simulation = false;
		bool simok = SF.FieldLayout();
		if (SF.ErrCheck() || !simok) return false;
	}

	//follow-on stuff
	Vect sun = Ambient::calcSunVectorFromAzZen(SF.getVarMap()->sf.sun_az_des.Val() * D2R, (90. - SF.getVarMap()->sf.sun_el_des.Val()) * D2R);
	SF.calcHeliostatShadows(sun);    if (SF.ErrCheck()) return false;
	V.land.bound_area.Setval(SF.getLandObject()->getLandBoundArea() / 4046.86 ); // [m^2] -> [acres]

	return true;

}

void interop::CreateResultsTable(sim_result& result, grid_emulator_base& table)
{
	try
	{
		//table.CreateGrid(result.is_soltrace ? 18 : 19, 6);
		table.CreateGrid(18, 6);

		table.SetColLabelValue(0, "Units");
		table.SetColLabelValue(1, "Value");
		table.SetColLabelValue(2, "Mean");
		table.SetColLabelValue(3, "Minimum");
		table.SetColLabelValue(4, "Maximum");
		table.SetColLabelValue(5, "Std. dev");

		int id = 0;
		table.AddRow(id++, "Total plant cost", "$", result.total_installed_cost, 0);
		//table.AddRow(id++, "Cost/Energy metric", "-", result.coe_metric);
		table.AddRow(id++, "Simulated heliostat area", "m^2", result.total_heliostat_area);
		table.AddRow(id++, "Simulated heliostat count", "-", result.num_heliostats_used, 0);
		table.AddRow(id++, "Power incident on field", "kW", result.power_on_field);
		table.AddRow(id++, "Power absorbed by the receiver", "kW", result.power_absorbed);
		table.AddRow(id++, "Power absorbed by HTF", "kW", result.power_to_htf);

		double nan = std::numeric_limits<double>::quiet_NaN();
		//------------------------------------
		if (result.is_soltrace)
		{
			table.AddRow(id++, "Cloudiness efficiency", "%", 100. * result.eff_cloud.wtmean, 2);
			table.AddRow(id++, "Shadowing and Cosine efficiency", "%", 100. * result.eff_cosine.wtmean, 2);
			table.AddRow(id++, "Reflection and Attenuation efficiency", "%", 100. * result.eff_reflect.wtmean, 2);
			table.AddRow(id++, "Blocking efficiency", "%", 100. * result.eff_blocking.wtmean, 2);
			table.AddRow(id++, "Image intercept efficiency", "%", 100. * result.eff_intercept.wtmean, 2);
			table.AddRow(id++, "Absorption efficiency", "%", 100. * result.eff_absorption.wtmean, 2);
			table.AddRow(id++, "Solar field optical efficiency", "%", 100. * result.eff_total_sf.wtmean / result.eff_absorption.wtmean, 2);
			table.AddRow(id++, "Optical efficiency incl. receiver", "%", 100. * result.eff_total_sf.wtmean, 2);
			table.AddRow(id++, "Incident flux", "kW/m2", result.flux_density.ave, -1, nan, result.flux_density.min, result.flux_density.max, result.flux_density.stdev);
			table.AddRow(id++, "No. rays traced", "-", result.num_ray_traced, 0);
			table.AddRow(id++, "No. heliostat ray intersections", "-", result.num_ray_heliostat, 0);
			table.AddRow(id++, "No. receiver ray intersections", "-", result.num_ray_receiver, 0);
		}
		else
		{   //results table for hermite simulation
			table.AddRow(id++, "Cloudiness efficiency", "%",
				100. * result.eff_cloud.wtmean, 2,
				100. * result.eff_cloud.ave,
				100. * result.eff_cloud.min,
				100. * result.eff_cloud.max,
				100. * result.eff_cloud.stdev);
			table.AddRow(id++, "Shading efficiency", "%",
				100. * result.eff_shading.wtmean, 2,
				100. * result.eff_shading.ave,
				100. * result.eff_shading.min,
				100. * result.eff_shading.max,
				100. * result.eff_shading.stdev);
			table.AddRow(id++, "Cosine efficiency", "%",
				100. * result.eff_cosine.wtmean, 2,
				100. * result.eff_cosine.ave,
				100. * result.eff_cosine.min,
				100. * result.eff_cosine.max,
				100. * result.eff_cosine.stdev);
			table.AddRow(id++, "Reflection efficiency", "%",
				100. * result.eff_reflect.wtmean, 2,
				100. * result.eff_reflect.ave,
				100. * result.eff_reflect.min,
				100. * result.eff_reflect.max,
				100. * result.eff_reflect.stdev);
			table.AddRow(id++, "Blocking efficiency", "%",
				100. * result.eff_blocking.wtmean, 2,
				100. * result.eff_blocking.ave,
				100. * result.eff_blocking.min,
				100. * result.eff_blocking.max,
				100. * result.eff_blocking.stdev);
			table.AddRow(id++, "Attenuation efficiency", "%",
				100. * result.eff_attenuation.wtmean, 2,
				100. * result.eff_attenuation.ave,
				100. * result.eff_attenuation.min,
				100. * result.eff_attenuation.max,
				100. * result.eff_attenuation.stdev);
			table.AddRow(id++, "Image intercept efficiency", "%",
				100. * result.eff_intercept.wtmean, 2,
				100. * result.eff_intercept.ave,
				100. * result.eff_intercept.min,
				100. * result.eff_intercept.max,
				100. * result.eff_intercept.stdev);
			table.AddRow(id++, "Absorption efficiency", "%", 100. * result.eff_absorption.wtmean, 2);
			table.AddRow(id++, "Solar field optical efficiency", "%",
				100. * result.eff_total_sf.wtmean / result.eff_absorption.wtmean, 2,
				nan,
				100. * result.eff_total_sf.min / result.eff_absorption.wtmean,
				100. * result.eff_total_sf.max / result.eff_absorption.wtmean,
				100. * result.eff_total_sf.stdev / result.eff_absorption.wtmean);
			table.AddRow(id++, "Optical efficiency incl. receiver", "%",
				100. * result.eff_total_sf.wtmean, 2,
				nan,
				100. * result.eff_total_sf.min,
				100. * result.eff_total_sf.max,
				100. * result.eff_total_sf.stdev);
			table.AddRow(id++, "Annualized heliostat efficiency", "%",
				100. * result.eff_annual.ave, 2,
				nan,
				100. * result.eff_annual.min,
				100. * result.eff_annual.max,
				100. * result.eff_annual.stdev);
			table.AddRow(id++, "Incident flux", "kW/m2",
				result.flux_density.ave, -1,
				nan,
				result.flux_density.min,
				result.flux_density.max,
				result.flux_density.stdev);
		}
	}
	catch (...)
	{
		throw spexception("An error occurred while trying to process the simulation results for display.");
	}

}




//-----


void stat_object::initialize(){ //int size){
	min = 9.e99; 
	max = -9.e99;
	sum = 0.;
    wtmean = 0.;
	stdev = 0.;
	ave = 0.;
}

void stat_object::zero()
{
    min = 0.;
    max = 0.;
    sum = 0.;
    wtmean = 0.;
    stdev = 0.;
    ave = 0.;
}

void stat_object::set(double _min, double _max, double _ave, double _stdev, double _sum, double _wtmean)
{
	min = _min;
	max = _max;
	ave = _ave;
	stdev = _stdev;
	sum = _sum;
    wtmean = _wtmean;
}

//-----------------------
//cost
//cost_categories::cost_categories(){
//	reset();
//}
//
//void cost_categories::reset()
//{
//	//initialize
//	c_heliostat = 0.;
//	c_receiver = 0.;
//	c_tower = 0.;
//	c_tes = 0.;
//	c_pb = 0.;
//	c_other = 0.;
//	c_total = 0.;
//}
//
//void cost_categories::calculateTotalCost(){
//			
//	c_total = c_heliostat + c_receiver + c_tower + c_tes + c_pb + c_other;
//}

//-----------------------
sim_result::sim_result(){
	initialize();
}

void sim_result::initialize(){

    total_heliostat_area = 0.;
    total_receiver_area = 0.;
    total_land_area = 0.;
    power_on_field = 0.;
    power_absorbed = 0.;
    power_thermal_loss = 0.;
    power_piping_loss = 0.;
    power_to_htf = 0.;
    power_to_cycle = 0.;
    power_gross = 0.;
    power_net = 0.;
    dni = 0.;
    solar_az = 0.;
    solar_zen = 0.;
    total_installed_cost = 0.;
    coe_metric = 0.;
    
    num_heliostats_used = 0;
	num_heliostats_avail = 0;
    num_ray_traced = 0;
    num_ray_heliostat = 0;
    num_ray_receiver = 0;
	_q_coe = 0.;
    
    time_date_stamp = "";
    aim_method = "";

    eff_total_heliostat.initialize();
	eff_total_sf.initialize();
	eff_cosine.initialize();
	eff_attenuation.initialize();
	eff_blocking.initialize();
	eff_shading.initialize();
	eff_reflect.initialize();
	eff_intercept.initialize();
	eff_absorption.initialize();
    eff_annual.initialize();
	flux_density.initialize();
    eff_cloud.initialize();

	flux_surfaces.clear();
	data_by_helio.clear();
}

void sim_result::zero()
{
    initialize();

    eff_total_heliostat.zero();
    eff_total_sf.zero();
    eff_cosine.zero();
    eff_attenuation.zero();
    eff_blocking.zero();
    eff_shading.zero();
    eff_reflect.zero();
    eff_intercept.zero();
    eff_absorption.zero();
    eff_annual.zero();
    eff_cloud.zero();
    flux_density.zero();
}

void sim_result::add_heliostat(Heliostat &H){
	H.getEfficiencyObject()->rec_absorptance = H.getWhichReceiver()->getVarMap()->absorptance.val;
	data_by_helio[ H.getId() ] = *H.getEfficiencyObject();
	num_heliostats_used++;
	total_heliostat_area += H.getArea();
	_q_coe += H.getPowerValue();
}

void sim_result::process_field_stats(){
	//Calculate statistics for all of the heliostats
    if( data_by_helio.size() == 0 )
        return;

	int nm = (data_by_helio.begin()->second).n_metric;
	double
		* sums = new double[nm],		// Why is this done on the heap rather than the stack?
		* stdevs = new double[nm],
		* mins = new double[nm],
		* maxs = new double[nm],
		* aves = new double[nm],
		* aves2 = new double[nm],		//Temporary array for calculating variance
		* wtmean = new double[nm];

	for(int i=0; i<nm; i++){ 
		sums[i] = 0.;
		stdevs[i] = 0.;
		mins[i] = 9.e9;
		maxs[i] = -9.e9;
		aves[i] = 0.;
		aves2[i] = 0.;
        wtmean[i] = 0.;
	}

	//Calculate metrics
	int n=0;
	double delta;
	for(unordered_map<int, helio_perf_data>::iterator it = data_by_helio.begin(); it != data_by_helio.end(); it++){
		n++;
		for(int j=0; j<nm; j++){
			double v = it->second.getDataByIndex(j);
			sums[j] += v;
			if(v > maxs[j]) maxs[j] = v;
			if(v < mins[j]) mins[j] = v;	

			//Average and variance estimates using Knuth's method
			delta = v - aves[j];
			aves[j] += delta / (double)n;
			aves2[j] += delta*(v-aves[j]);
		}
	}
	//Calculate final standard deviation
	for(int j=0; j<nm; j++)
		stdevs[j] = sqrt(aves2[j]/(double)(n-1));

	delete [] aves2;

    //calculate weighted average efficiency values
    std::vector<int> eff_cascade_indices = {
        helio_perf_data::PERF_VALUES::ETA_CLOUD,
        helio_perf_data::PERF_VALUES::ETA_SHADOW,
        helio_perf_data::PERF_VALUES::ETA_COS,
        helio_perf_data::PERF_VALUES::SOILING,
        helio_perf_data::PERF_VALUES::REFLECTIVITY,
        helio_perf_data::PERF_VALUES::ETA_BLOCK,
        helio_perf_data::PERF_VALUES::ETA_ATT,
        helio_perf_data::PERF_VALUES::ETA_INT,
        helio_perf_data::PERF_VALUES::REC_ABSORPTANCE
    };
    int nh = (int)data_by_helio.size();
    double *rowprod = new double[ nh ];
    for (int i = 0; i < nh; i++)
        rowprod[i] = 1.;    //initialize

    for (size_t i = 0; i < eff_cascade_indices.size(); i++)
    {
        int cascade_index = eff_cascade_indices.at(i); //keep track of current index

        //calculate the running product of losses 'rowprod[j]' for all heliostats j
        int j = 0;
        for (unordered_map<int, helio_perf_data>::iterator it = data_by_helio.begin(); it != data_by_helio.end(); it++)
            rowprod[j++] *= it->second.getDataByIndex(cascade_index);

        //Sum the running product vector and divide by number of heliostats for uncorrected weighted average
        for (j = 0; j < nh; j++)
            wtmean[cascade_index] += rowprod[j];
        wtmean[cascade_index] /= (double)(nh > 0 ? nh : 1);

        //Correct the current efficiency by taking out previous weighted efficiency values
        for (size_t k = 0; k < i; k++)
            wtmean[cascade_index] /= wtmean[eff_cascade_indices.at(k)];
    }

    delete[] rowprod;

	//Assign the named variables
    eff_total_heliostat.set(
        mins[helio_perf_data::PERF_VALUES::ETA_TOT],
        maxs[helio_perf_data::PERF_VALUES::ETA_TOT],
        aves[helio_perf_data::PERF_VALUES::ETA_TOT],
        stdevs[helio_perf_data::PERF_VALUES::ETA_TOT],
        sums[helio_perf_data::PERF_VALUES::ETA_TOT],
        wtmean[helio_perf_data::PERF_VALUES::ETA_TOT]);
    eff_cosine.set(
        mins[helio_perf_data::PERF_VALUES::ETA_COS],
        maxs[helio_perf_data::PERF_VALUES::ETA_COS],
        aves[helio_perf_data::PERF_VALUES::ETA_COS],
        stdevs[helio_perf_data::PERF_VALUES::ETA_COS],
        sums[helio_perf_data::PERF_VALUES::ETA_COS],
        wtmean[helio_perf_data::PERF_VALUES::ETA_COS]);
    eff_attenuation.set(
        mins[helio_perf_data::PERF_VALUES::ETA_ATT],
        maxs[helio_perf_data::PERF_VALUES::ETA_ATT],
        aves[helio_perf_data::PERF_VALUES::ETA_ATT],
        stdevs[helio_perf_data::PERF_VALUES::ETA_ATT],
        sums[helio_perf_data::PERF_VALUES::ETA_ATT],
        wtmean[helio_perf_data::PERF_VALUES::ETA_ATT]);
    eff_blocking.set(
        mins[helio_perf_data::PERF_VALUES::ETA_BLOCK],
        maxs[helio_perf_data::PERF_VALUES::ETA_BLOCK],
        aves[helio_perf_data::PERF_VALUES::ETA_BLOCK],
        stdevs[helio_perf_data::PERF_VALUES::ETA_BLOCK],
        sums[helio_perf_data::PERF_VALUES::ETA_BLOCK],
        wtmean[helio_perf_data::PERF_VALUES::ETA_BLOCK]);
	eff_shading.set(
		mins[ helio_perf_data::PERF_VALUES::ETA_SHADOW ], 
		maxs[ helio_perf_data::PERF_VALUES::ETA_SHADOW ], 
		aves[ helio_perf_data::PERF_VALUES::ETA_SHADOW ], 
		stdevs[ helio_perf_data::PERF_VALUES::ETA_SHADOW ], 
        sums[helio_perf_data::PERF_VALUES::ETA_SHADOW],
        wtmean[helio_perf_data::PERF_VALUES::ETA_SHADOW]);
    eff_intercept.set(
        mins[helio_perf_data::PERF_VALUES::ETA_INT],
        maxs[helio_perf_data::PERF_VALUES::ETA_INT],
        aves[helio_perf_data::PERF_VALUES::ETA_INT],
        stdevs[helio_perf_data::PERF_VALUES::ETA_INT],
        sums[helio_perf_data::PERF_VALUES::ETA_INT],
        wtmean[helio_perf_data::PERF_VALUES::ETA_INT]);
    eff_absorption.set(
        mins[helio_perf_data::PERF_VALUES::REC_ABSORPTANCE],
        maxs[helio_perf_data::PERF_VALUES::REC_ABSORPTANCE],
        aves[helio_perf_data::PERF_VALUES::REC_ABSORPTANCE],
        stdevs[helio_perf_data::PERF_VALUES::REC_ABSORPTANCE],
        sums[helio_perf_data::PERF_VALUES::REC_ABSORPTANCE],
        wtmean[helio_perf_data::PERF_VALUES::REC_ABSORPTANCE]);
    eff_annual.set(
        mins[helio_perf_data::PERF_VALUES::ANNUAL_EFFICIENCY],
        maxs[helio_perf_data::PERF_VALUES::ANNUAL_EFFICIENCY],
        aves[helio_perf_data::PERF_VALUES::ANNUAL_EFFICIENCY],
        stdevs[helio_perf_data::PERF_VALUES::ANNUAL_EFFICIENCY],
        sums[helio_perf_data::PERF_VALUES::ANNUAL_EFFICIENCY],
        wtmean[helio_perf_data::PERF_VALUES::ANNUAL_EFFICIENCY]);
    eff_cloud.set(
        mins[helio_perf_data::PERF_VALUES::ETA_CLOUD],
        maxs[helio_perf_data::PERF_VALUES::ETA_CLOUD],
        aves[helio_perf_data::PERF_VALUES::ETA_CLOUD],
        stdevs[helio_perf_data::PERF_VALUES::ETA_CLOUD],
        sums[helio_perf_data::PERF_VALUES::ETA_CLOUD],
        wtmean[helio_perf_data::PERF_VALUES::ETA_CLOUD]);
	eff_reflect.set(
		mins[ helio_perf_data::PERF_VALUES::REFLECTIVITY ]*mins[ helio_perf_data::PERF_VALUES::SOILING ], 
		maxs[ helio_perf_data::PERF_VALUES::REFLECTIVITY ]*maxs[ helio_perf_data::PERF_VALUES::SOILING ], 
		aves[ helio_perf_data::PERF_VALUES::REFLECTIVITY ]*aves[ helio_perf_data::PERF_VALUES::SOILING ], 
		stdevs[ helio_perf_data::PERF_VALUES::REFLECTIVITY ]*stdevs[ helio_perf_data::PERF_VALUES::SOILING ], 
		sums[ helio_perf_data::PERF_VALUES::REFLECTIVITY ]*sums[ helio_perf_data::PERF_VALUES::SOILING ],
        wtmean[helio_perf_data::PERF_VALUES::REFLECTIVITY] * wtmean[helio_perf_data::PERF_VALUES::SOILING]
	);
	eff_total_sf.set(
		mins[ helio_perf_data::PERF_VALUES::ETA_TOT ]* wtmean[ helio_perf_data::PERF_VALUES::REC_ABSORPTANCE ],
		maxs[ helio_perf_data::PERF_VALUES::ETA_TOT ]* wtmean[ helio_perf_data::PERF_VALUES::REC_ABSORPTANCE ],
		aves[ helio_perf_data::PERF_VALUES::ETA_TOT ]* wtmean[ helio_perf_data::PERF_VALUES::REC_ABSORPTANCE ],
		stdevs[ helio_perf_data::PERF_VALUES::ETA_TOT ]* wtmean[ helio_perf_data::PERF_VALUES::REC_ABSORPTANCE ],
		sums[ helio_perf_data::PERF_VALUES::ETA_TOT ]* wtmean[ helio_perf_data::PERF_VALUES::REC_ABSORPTANCE ],
        aves[helio_perf_data::PERF_VALUES::ETA_TOT] * wtmean[helio_perf_data::PERF_VALUES::REC_ABSORPTANCE]
	);
	
	delete [] sums;
	delete [] aves;
	delete [] stdevs;
	delete [] mins; 
	delete [] maxs;
	delete [] wtmean;
}

void sim_result::process_flux_stats(Rvector *recs)
{
	//Determine the flux info
	double fave=0., fave2=0., fmax = -9.e9, fmin = 9.e9;
	int nf = 0;
	for( int i=0; i<(int)recs->size(); i++){
		int recgeom = recs->at(i)->getGeometryType();
		FluxSurfaces *fs = recs->at(i)->getFluxSurfaces();
		int start_surf = 0;
		switch (recs->at(i)->getVarMap()->rec_type.mapval())
		{
		case var_receiver::REC_TYPE::CAVITY:
		case var_receiver::REC_TYPE::FALLING_PARTICLE:
			start_surf = 1; // Skip aperture surface
			break;
		default:
			break;
		}

		for(int j=start_surf; j<(int)fs->size(); j++){
			FluxGrid *fg = fs->at(j).getFluxMap();
			int 
				nx = fs->at(j).getFluxNX(),
				ny = fs->at(j).getFluxNY();
			double v, delta;
			for(int k=0; k<nx; k++){
				for(int l=0; l<ny; l++){
					v = fg->at(k).at(l).flux;
						
					if(v > fmax) fmax = v;
					if(v < fmin) fmin = v;

					//Estimate variance - use Knuth's one-pass method (http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance) 
					nf++;
					delta = v - fave;
					fave += delta/(double)nf;
					fave2 += delta*(v-fave);

				}
			}
		}
	}
	//Calculate final variance value
	flux_density.stdev = sqrt(fave2/(double)(nf-1));
	flux_density.max = fmax;
	flux_density.min = fmin;
	flux_density.ave = fave;
}

void sim_result::process_analytical_simulation(SolarField &SF, sim_params &P, int nsim_type, double sun_az_zen[2], Hvector* helios, Rvector* receivers)
{
	is_soltrace = false;
	sim_type = nsim_type;

    var_map *V = SF.getVarMap();

    if (!helios)
        helios = SF.getHeliostats();
    if (!receivers)
        receivers = SF.getReceivers();

    receiver_names.clear();
    for(size_t i=0; i<receivers->size(); i++)
        receiver_names.push_back( receivers->at(i)->getVarMap()->rec_name.val );

	switch (sim_type)
	{
	case sim_result::SIM_TYPE::PARAMETRIC:
	case sim_result::SIM_TYPE::OPTIMIZATION:
	case sim_result::SIM_TYPE::LAYOUT:
	{
		//process only the ranking metric for each heliostat and the field avg. eff
		initialize();
		double effsum = 0.;
		for(unsigned int i=0; i<helios->size(); i++)
        {
			effsum += helios->at(i)->getEfficiencyTotal();
			add_heliostat(*helios->at(i));
		}
		
        eff_total_sf.ave = effsum / (double)helios->size() ;
		
        dni = P.dni; //W/m2
		power_on_field = total_heliostat_area * dni;	//[W]
		power_absorbed = power_on_field * eff_total_sf.ave;
        
        total_receiver_area = 0.;
        power_thermal_loss = 0.;
        power_piping_loss = 0.;
        
        for (Rvector::iterator rec = receivers->begin(); rec != receivers->end(); rec++)
        {
            if (!(*rec)->isReceiverEnabled())
                continue;
            total_receiver_area += (*rec)->getVarMap()->absorber_area.Val();
            power_thermal_loss += (*rec)->getReceiverThermalLoss();
            power_piping_loss += (*rec)->getReceiverPipingLoss();
        }

        power_to_htf = power_absorbed - (power_thermal_loss + power_piping_loss)*1.e6;

		solar_az = sun_az_zen[0];
		solar_zen = sun_az_zen[1];
        time_date_stamp = SF.getVarMap()->sf.des_sim_detail.val + " 12:00";
        aim_method = "Simple aimpoints";
		break;
	}
	case sim_result::SIM_TYPE::FLUX_SIMULATION:
	{
		initialize();
        for (unsigned int i = 0; i < helios->size(); i++)
		{
            if (helios->at(i)->IsInLayout() && helios->at(i)->IsEnabled())
            {
				add_heliostat(*helios->at(i));
            }
		}
		process_field_stats();
		dni =  SF.getVarMap()->flux.flux_dni.val/1000.;		// TODO: Why is this not using sim_params structure?
		power_on_field = total_heliostat_area * dni;	//[kW]
		power_absorbed = power_on_field * eff_total_sf.ave;

        total_receiver_area = 0.;
        power_thermal_loss = 0.;
        power_piping_loss = 0.; 

        for (Rvector::iterator rec = receivers->begin(); rec != receivers->end(); rec++)
        {
            if (!(*rec)->isReceiverEnabled())
                continue;
            total_receiver_area += (*rec)->getVarMap()->absorber_area.Val();
            power_thermal_loss += (*rec)->getReceiverThermalLoss()*1000.;
            power_piping_loss += (*rec)->getReceiverPipingLoss()*1000.;
        }

        power_to_htf = power_absorbed - (power_thermal_loss + power_piping_loss);

		solar_az = sun_az_zen[0];
		solar_zen = sun_az_zen[1];
        double hour = SF.getVarMap()->flux.flux_hour.val;
        std::stringstream ss;
        ss << DateTime::GetMonthName(SF.getVarMap()->flux.flux_month.val)
           << " " << SF.getVarMap()->flux.flux_day.val << " | "
           << std::setw(2) << std::setfill('0') << (int)hour << ":"
           << std::setw(2) << std::setfill('0') << (int)(std::fmod(hour,1.)*60.+.001);
        time_date_stamp = ss.str();
        aim_method = SF.getVarMap()->flux.aim_method.val + " aimpoints";

		SF.getFinancialObject()->calcPlantCapitalCost(*SF.getVarMap());	//Always update the plant cost
		total_installed_cost = V->fin.total_installed_cost.Val(); 
		coe_metric = total_installed_cost/_q_coe;
		
		process_flux_stats(receivers);

		break;
	}
	default:
		break;
	}

}

#ifdef SP_USE_SOLTRACE
void sim_result::process_raytrace_simulation(SolarField& SF, sim_params& P, int nsim_type, double sun_az_zen[2], Hvector& helios, ST_System* STsim)
{
	is_soltrace = true;
	/* sim_type: 2=flux simulation, 3=parametric */
	initialize();
	sim_type = nsim_type;
	if(sim_type == 2){
		num_heliostats_used = (int)helios.size();
		for(int i=0; i<num_heliostats_used; i++){
			total_heliostat_area += helios.at(i)->getArea();
		}
		var_heliostat* Hv = helios.front()->getVarMap();
        double dni = P.dni; //W/m2
		
		// vector of heliostat values with the ray as the index
		var_receiver* RV = SF.getReceivers()->at(0)->getVarMap();
		int max_rec_e = 1; //Maximum Receiver element number, it is assumed that heat absorbing elements are between zero and this value
		if (RV->rec_type.mapval() == var_receiver::REC_TYPE::FALLING_PARTICLE) max_rec_e = RV->n_panels.val;
		int rstage = STsim->StageList.size(); // Receiver stage
#if HELIO_INTERCEPT
		int max_rays = *std::max_element(STsim->IntData.rnum, STsim->IntData.rnum + STsim->IntData.nint);
		std::vector<int> ray_helios(max_rays+1, -1); // Rays start at 1
		int npanels = 1;
		if (Hv->is_faceted.val) {
			npanels = Hv->n_cant_x.val * Hv->n_cant_y.val;
		}
#endif
		//Process the ray data
		int st, st0=0, ray, ray0=0, el;
		int nhin=0, nhout=0, nhblock=0, nhabs=0, nrin=0, nrabs=0;
		int h_ind;
		for(int i=0; i<STsim->IntData.nint; i++){ //for each ray hit
			st = STsim->IntData.smap[i];	//Stage
			ray = STsim->IntData.rnum[i];	//Ray number
			el = STsim->IntData.emap[i];	//Element

			if (st == 1) { // Heliostat stage
#if HELIO_INTERCEPT
				//Determine the heliostat from the element number in stage 1, first intersection only
				if (ray_helios[ray] == -1) ray_helios[ray] = (std::abs(el) - 1) / npanels;
#endif
				if (el > 0 && ray != ray0) nhin++; // Reflected, no second reflection allowed
				else if (el < 0) { // Absorbed
					if (ray == ray0) nhblock++;    //Multiple intersections within heliostat field, meaning blocking occurred
					else {
						nhabs++; //Single intersection -- reflectivity loss
						nhin++;
					}
				}
			}
			else if (STsim->aperture_virtual_stage && st == rstage - 1) { // Aperture stage
				if (el > 0) nrin++; // Aperture hit
			}
			else if (st == rstage) { // Receiver stage
				if (!STsim->aperture_virtual_stage && el != 0) nrin++; // Reflected or Absorbed
				
				if (el < 0 && el >= -max_rec_e) nrabs++; // Absorbed only if ray hits heat absorbing receiver elements
			}
			ray0 = ray; // previous ray
		}
#if HELIO_INTERCEPT
		// Go through the rays again for updating intercept efficiency because 
		// we now know the heliostat associated with each ray
		std::vector<double> rec_hit_rays(helios.size(), 0.0);
		std::vector<double> ref_rays(helios.size(), 1.e-7);
		ray0 = 0;
		int intercept_stage = 2;
		if (STsim->aperture_virtual_stage) intercept_stage = STsim->StageList.size() - 1;
		for (int i = 0; i < STsim->IntData.nint; i++) { //for each ray hit
			st = STsim->IntData.smap[i];	//Stage
			ray = STsim->IntData.rnum[i];	//Ray number
			el = STsim->IntData.emap[i];	//Element
			h_ind = ray_helios[ray];	// Heliostat index

			// Repeated algorithm from above
			if (st == 1) { // Heliostat stage
				if (el > 0 && ray != ray0) ref_rays[h_ind] += 1.; // Reflected, no second reflection allowed
				else if (el < 0) { // Absorbed
					if (ray == ray0) ref_rays[h_ind] -= 1.;    //Multiple intersections within heliostat field, meaning blocking occurred
					else {
						ref_rays[h_ind] -= 1.; //Single intersection -- reflectivity loss
						ref_rays[h_ind] += 1.;
					}
				}
			}
			//Check if ray hits target on first intersection
			if (st == intercept_stage && ray != ray0 && el != 0) { // Aperture or recevier stage and new ray
				rec_hit_rays[h_ind] += 1.;
			}
			ray0 = ray;  // previous ray
		}
		// Using ray data to update heliostat intercept efficiency -> TODO: Fix this
		for (int i = 0; i < num_heliostats_used; i++) {
			helios.at(i)->setEfficiencyIntercept(rec_hit_rays[i] / ref_rays[i]);
			helios.at(i)->calcPowerEnergy(P);
		}
#endif

		int nsunrays = (int)STsim->IntData.bounds[4];
		double Abox = (STsim->IntData.bounds[0] - STsim->IntData.bounds[1])*(STsim->IntData.bounds[2] - STsim->IntData.bounds[3]);

        num_ray_traced = nsunrays;
        num_ray_heliostat = nhin;
        num_ray_receiver = nrin;

		// Adjust ray counts based on the heliostat reflective surface radio -> assumes constant precent of rays are lost
		nhin *= Hv->reflect_ratio.val;
		nhabs *= Hv->reflect_ratio.val;
		nhblock *= Hv->reflect_ratio.val;
		nrin *= Hv->reflect_ratio.val;
		nrabs *= Hv->reflect_ratio.val;
		nhout = nhin - nhabs - nhblock; // rays in field less absorbed and blocked

		power_on_field = total_heliostat_area *dni;	//[kW]
		power_absorbed = STsim->IntData.q_ray * nrabs;
        power_thermal_loss = SF.getReceiverTotalHeatLoss();
        power_piping_loss = SF.getReceiverPipingHeatLoss();
        power_to_htf = power_absorbed - (power_thermal_loss + power_piping_loss);
		
        eff_total_sf.set(0,0, 0, 0, 0., power_absorbed / power_on_field);
        eff_cosine.set(0.,0., 0., 0., 0., (double)nhin / (double)nsunrays*Abox / total_heliostat_area);
        eff_shading.set(1., 1., 1., 0., 1., 1.);        //shading is accounted for in the cosine calculation
		eff_blocking.set(0.,0., 0., 0., 0., 1. - (double)nhblock / (double)(nhin - nhabs));
		eff_attenuation.set(0., 0., 0., 0., 0., 1.);	// Accounted for within reflection efficiency
		eff_reflect.set(0., 0., 0., 0., 0., (double)(nhin - nhabs) / (double)nhin);
		eff_intercept.set(0., 0., 0., 0., 0., (double)nrin / (double)nhout);
		eff_absorption.set(0., 0., 0., 0., 0., (double)nrabs / (double)nrin);
		eff_total_heliostat.set(0., 0., 0., 0., 0., (double)nrabs / (double)nhin);
        eff_cloud.set(1., 1., 1., 0., 1., 1.);

		total_receiver_area = SF.calcReceiverTotalArea();
		solar_az = sun_az_zen[0];
		solar_zen = sun_az_zen[1];
        int month, day_of_month;
        double hour = SF.getVarMap()->flux.flux_hour.val;
        DateTime().hours_to_date(SF.getVarMap()->flux.flux_day.val * 24 + hour, month, day_of_month);
        std::stringstream ss;
        ss << DateTime::GetMonthName(month) << " " << day_of_month
            << std::setw(2) << std::setfill('0') << (int)hour << ":"
            << std::setw(2) << std::setfill('0') << (int)(std::fmod(hour, 1.)*60. + .001);
        time_date_stamp = ss.str();
        aim_method = SF.getVarMap()->flux.aim_method.val + " aimpoints";

		SF.getFinancialObject()->calcPlantCapitalCost(*SF.getVarMap());	//Always update the plant cost

		total_installed_cost = SF.getVarMap()->fin.total_installed_cost.Val();
		coe_metric = total_installed_cost/power_absorbed;

		process_flux_stats(SF.getReceivers());
	}
	else{

	}


}
#endif

void sim_result::process_flux(SolarField* SF, bool normalize) {
	flux_surfaces.clear();
	receiver_names.clear();
	int nr = (int)SF->getReceivers()->size();
	Receiver* rec;
	for (int i = 0; i < nr; i++) {
		rec = SF->getReceivers()->at(i);
		if (!rec->isReceiverEnabled()) continue;
		int n_surfaces = rec->getFluxSurfaces()->size();
		flux_surfaces.push_back(*rec->getFluxSurfaces());
		if (normalize) {
			if (n_surfaces == 1) {
				for (unsigned int j = 0; j < rec->getFluxSurfaces()->size(); j++) {
					flux_surfaces.back().at(j).Normalize();
				}
			}
			else {
				// For aperture (j = 0), use normalize, because we don't want it to count towards total flux
				flux_surfaces.back().at(0).Normalize();

				// Sum flux on all receiver panels
				double flux_tot = 0.0;
				for (unsigned int j = 1; j < rec->getFluxSurfaces()->size(); j++) {
					flux_tot += flux_surfaces.back().at(j).getTotalFlux();
				}

				for (unsigned int j = 1; j < rec->getFluxSurfaces()->size(); j++) {
					flux_surfaces.back().at(j).Scale(1.0 / flux_tot);
				}
			}
		}
		receiver_names.push_back(
			//*SF->getReceivers()->at(i)->getReceiverName() 
			SF->getReceivers()->at(i)->getVarMap()->rec_name.val
		);
	}

}

//------parametric------------------------
multivar::multivar(){wf_are_set = false;}

parametric::parametric(){wf_are_set = false;}

int multivar::size(){ return (int)variables.size(); }

void multivar::clear(){ variables.clear(); current_varpaths.Clear();}

par_variable &multivar::at(int index){ return variables.at(index); }

par_variable &multivar::operator[](int index){ return variables.at(index); }

par_variable &multivar::back(){ return variables.back(); }

void multivar::remove(int index){ 
	variables.erase(variables.begin()+index); 
	current_varpaths.erase(current_varpaths.begin()+index); 
}

void parametric::SetWeatherFileList(ArrayString &list){
	weather_files.clear();
	for(int i=0; i<list.size(); i++){
		weather_files.push_back(list[i]);
	}
	wf_are_set = true;
}


void multivar::addVar(spbase *var)
{
	/* 
	Add a new variable to the parametric analysis. If the variable already exists, overwrite it.
	*/

    
	//check to see whether this variable already exists. If so, overwrite it
	int curind = Index(var->name);
	par_variable *vback;

	if(curind > 0){
		variables.erase(variables.begin()+curind);
		variables.insert(variables.begin()+curind, par_variable());
		vback = &variables.at(curind);
	}
	else
	{
		current_varpaths.Add(var->name);
		variables.push_back(par_variable());
		vback = &variables.back();
	}
	
	vback->varname = var->name;
	vback->display_text = split(var->name, ".").at(0) + ": "+var->short_desc;
	vback->units = var->units;
	vback->selections.clear();
	
	if(var->name == "ambient.0.weather_file"){
		if(! wf_are_set) return;
        string ts;
        var->as_string(ts);
		vback->selections.Add(ts);
		vback->data_type = "location";
		//Load the possible weather files
		vback->choices.clear();
		for(int i=0; i<weather_files.size(); i++){
			vback->choices.push_back(weather_files[i]);
		}
	}
	else if(var->ctype == "combo"){
		vback->selections.Add( var->as_string() );
		vback->data_type = "combo";
		vback->choices.clear();
        vector<string> vchoices = var->combo_get_choices();
        for(int i=0; i<(int)vchoices.size(); i++)
            vback->choices.Add(vchoices.at(i));

	}
	else if(var->ctype == "checkbox"){
		string ts;
        var->as_string(ts);
		vback->selections.Add(ts);
		vback->data_type = "checkbox";
		vback->choices.clear();
		vback->choices.push_back("true");
		vback->choices.push_back("false");
	}
	else if(var->ctype == "bool"){
        string ts;
        var->as_string(ts);
		vback->selections.Add(ts);
		vback->data_type = "bool";
		vback->choices.clear();
		vback->choices.push_back("true");
		vback->choices.push_back("false");
	}
	else if(var->ctype == "int")
    {
		string ts;
        var->as_string(ts);
		vback->selections.Add(ts);
		vback->data_type = "int";
		//int imin, imax;
		//bool withlow, withhi;
		//interop::parseRange(var->range, imin, imax, withlow, withhi);
		/*for(int i=(withlow?imin:imin+1); i<(withhi?imax:imax-1); i++){
			vback->choices.push_back( my_to_string(i) );
		}*/

	}

	else{	//doubles
		string ts;
        var->as_string(ts);
		vback->selections.Add(ts);
		vback->data_type = "double"; //var->dattype;
	}
}

int multivar::Index(string pathname){
	return current_varpaths.Index(pathname);
}

//----- user par table -------------------
ArrayString &simulation_table::operator[](const string &varname){return data[varname];}
ArrayString &simulation_table::at(const string &varname){return data[varname];}
size_t simulation_table::nvar(){return data.size();}
size_t simulation_table::nsim(){return data.size() > 0 ? data.begin()->second.size() : 0;}

void simulation_table::ClearAll(){
	for(unordered_map<string, ArrayString>::iterator it=data.begin(); it != data.end(); it++)
		it->second.Clear();
	data.clear();
}

void simulation_table::getKeys(ArrayString &keys){
	keys.clear();

	for(unordered_map<string, ArrayString>::iterator it = data.begin(); it != data.end(); it++)
		keys.push_back(it->first);
	
}


void grid_emulator_base::CreateGrid(int nrow, int ncol)
{
	_nrow = nrow;
	_ncol = ncol;
	data.clear();
	data.resize(nrow);
	for (int i = 0; i < nrow; i++)
		data.at(i).resize(ncol);
	rowlabs.resize(nrow);
	collabs.resize(ncol);
}

bool grid_emulator_base::SetColLabelValue(int col, std::string value)
{
	collabs.at(col) = value;
	return true;
}

bool grid_emulator_base::SetRowLabelValue(int row, std::string value)
{
	rowlabs.at(row) = value;
	return true;
}

bool grid_emulator_base::SetCellValue(int row, int col, std::string value)
{
	data.at(row).at(col) = value;
	return true;
}

bool grid_emulator_base::SetCellValue(std::string value, int row, int col)
{
	return SetCellValue(row, col, value);
}

std::vector<std::string> grid_emulator_base::GetPrintableTable(std::string eol)
{
	std::vector<std::string> printable(_nrow + 1, "");

	std::string hdr;
	for (int i = 0; i < _ncol; i++)
		hdr.append(", " + collabs.at(i));
	printable[0] = hdr;

	for (int i = 0; i < _nrow; i++)
	{
		std::string line = rowlabs.at(i);

		for (int j = 0; j < _ncol; j++)
		{
			std::string tval = GetCellValue(i, j);

			tval.erase(std::remove(tval.begin(), tval.end(), ','), tval.end());
			line.append(", " + tval); //Remove any commas from cell values - the file is comma-delimited
		}
		printable[i + 1] = line.append(eol);

	}
	return printable;
}



void grid_emulator_base::AddRow(int row, std::string label, std::string units, double value, int sigfigs, double mean, double min, double max, double stdev)
{
	//Row adding method for simple performance runs

	if ((GetNumberCols() < 6) || (GetNumberRows() < row + 1))
		throw spexception("Sorry! Results table incorrectly formatted. Please contact solarpilot.support@nrel.gov for help.");

	bool is_currency = false;
	if (units.find("$") != std::string::npos) is_currency = true;

	//calculate a good precision
	if (sigfigs < 0)
	{
		int prec = 4 - (int)log10f(value);
		sigfigs = prec < 0 ? 0 : prec;
	}

	char cline[300];
	sprintf(cline, "%s.%df", "%", sigfigs);
	std::string infmt(cline);
	sprintf(cline, "%s.%df", "%", sigfigs + 2);
	std::string stfmt(cline);
	//wxString infmt = wxString::Format("%s.%df", "%", sigfigs);
	//wxString stfmt = wxString::Format("%s.%df", "%", sigfigs+2);

	SetRowLabelValue(row, label);
	SetCellValue(row, 0, units);
	//SetCellValue(row, 1, is_currency ? gui_util::FormatAsCurrency(value) : to_string(value, infmt.c_str()));
	SetCellValue(row, 1, to_string(value, infmt.c_str()));
	SetCellValue(row, 2, (mean == mean ? to_string(mean, infmt.c_str()) : ""));
	SetCellValue(row, 3, (min == min ? to_string(min, infmt.c_str()) : ""));
	SetCellValue(row, 4, (max == max ? to_string(max, infmt.c_str()) : ""));
	SetCellValue(row, 5, (stdev == stdev ? to_string(stdev, stfmt.c_str()) : ""));

}

int grid_emulator_base::GetNumberRows()
{
	return _nrow;
}
int grid_emulator_base::GetNumberCols()
{
	return _ncol;
}
std::string grid_emulator_base::GetRowLabelValue(int row)
{
	return rowlabs.at(row);
}
std::string grid_emulator_base::GetColLabelValue(int col)
{
	return collabs.at(col);
}
std::string grid_emulator_base::GetCellValue(int row, int col)
{
	return data.at(row).at(col);
}
