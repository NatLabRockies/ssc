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

#include "Receiver.h"
#include <math.h>
#include "exceptions.hpp"
#include <vector>
using namespace std;

//----------------FluxPoint -----------------
FluxPoint::FluxPoint(){over_flux = false; flux = 0.;};
	
void FluxPoint::Setup(double xloc, double yloc, double zloc, Vect &norm, double flux_max, double Area_factor){
	location.x = xloc; location.y = yloc; location.z = zloc;
	normal.i = norm.i; normal.j = norm.j; normal.k = norm.k;
	maxflux = flux_max;
	over_flux = false;
	area_factor = Area_factor;
};
void FluxPoint::Setup(sp_point &loc, Vect &norm, double flux_max, double Area_factor){
	location.x = loc.x; location.y = loc.y; location.z = loc.z;
	normal.i = norm.i; normal.j = norm.j; normal.k = norm.k;
	maxflux = flux_max;
	over_flux = false;
	area_factor = Area_factor;
}


//----------------Absorber surface-----------------
Receiver *FluxSurface::getParent(){return _rec_parent;}
int FluxSurface::getId() {return _id;};
FluxGrid *FluxSurface::getFluxMap(){return &_flux_grid;}
int FluxSurface::getFluxNX(){return _nflux_x;}
int FluxSurface::getFluxNY(){return _nflux_y;}
sp_point *FluxSurface::getSurfaceOffset(){return &_offset;}
double FluxSurface::getSurfaceWidth(){return _width;}
double FluxSurface::getSurfaceHeight(){return _height;}
double FluxSurface::getSurfaceRadius(){return _radius;}
double FluxSurface::getSurfaceArea(){return _area;}
double FluxSurface::getMaxObservedFlux(){return _max_observed_flux;}

void FluxSurface::setParent(Receiver *recptr){_rec_parent = recptr;}
void FluxSurface::setFluxPrecision(int nx, int ny){_nflux_x=nx; _nflux_y = ny;}
void FluxSurface::setMaxFlux(double maxflux){_max_flux = maxflux;}
void FluxSurface::setNormalVector(Vect &vect){
	_normal = vect;
}
void FluxSurface::setSurfaceOffset(sp_point &loc){_offset = loc;}
void FluxSurface::setSurfaceSpanAngle(double span_min, double span_max){_span_ccw = span_min; _span_cw = span_max;}
void FluxSurface::setSurfaceGeometry(double height, double width, double radius){
	_width = width;
	_height = height;
	_radius = radius;	//if radius is 0, assume flat surface.
						//For nonzero radii, the WIDTH WILL BE
}
void FluxSurface::setMaxObservedFlux(double fmax){ _max_observed_flux = fmax; }
	
//Declare the scripts
void FluxSurface::ClearFluxGrid(){
	for(unsigned int i=0; i<_flux_grid.size(); i++){
		for(unsigned int j=0; j<_flux_grid.at(i).size(); j++){
			_flux_grid.at(i).at(j).flux = 0.;
		}
	}
}

// Clear flux grid and reset max flux to zero
void FluxSurface::ClearFluxGridResetMaxFlux() {
	this->ClearFluxGrid();
	this->setMaxObservedFlux(0.);
}

void FluxSurface::DefineFluxPoints(var_receiver &V, int rec_geom, int nx, int ny){
	/*
	Given the receiver geometry in "_parent", create a grid of flux hit test points.

	Flux points are in the global coordinate system but do not include receiver offset or tower height.

    */
	if(nx > 0) _nflux_x = nx;
	if(ny > 0) _nflux_y = ny;

	//if(rec_geom > -1) _type = rec_geom;	//Use the argument. Otherwise, use the locally set value

	switch (rec_geom)
	{
	case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CLOSED:
	{			//0 | Continuous closed cylinder - external
		
		//The flux for this geometry assumes that the cylinder is vertical (no zenith displacement)
        //Flux points are stored beginning lower edge, clockwise extent. Final entry upper edge counterclockwise extent

		_area = _height * _radius * PI * 2.;

		//Resize
		_flux_grid.resize(_nflux_x);	//number of rows

		//The azimuthal span taken up by each flux point
		double daz = (_span_cw - _span_ccw)/double(_nflux_x);	

		double faz;
		sp_point floc;
		Vect fnorm;
		double dz = _height/double(_nflux_y);	//height of each flux node
		for(int i=0; i<_nflux_x; i++){
			_flux_grid.at(i).resize(_nflux_y);	//number of columns

            faz = _span_cw - daz*(0.5 + (double)i); //The azimuth angle of the point
			//Calculate the normal vector
			fnorm.i = sin(faz);
			fnorm.j = cos(faz);
			fnorm.k = 0.;
			//Calculate the x-y position
			floc.x = fnorm.i * _radius;
			floc.y = fnorm.j * _radius;
			//Calculate the z position
			for(int j=0; j<_nflux_y; j++){
				floc.z = -_height/2.+dz*(0.5 + (double)j);
				//Set the location
				_flux_grid.at(i).at(j).Setup(floc, fnorm, _max_flux);
			}
		}
		break;
	}
    case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CAV:
    case Receiver::REC_GEOM_TYPE::POLYGON_CAV:
    {
        _area = _height * _width;
        _flux_grid.resize(_nflux_x); //Number of rows

        double rec_dw = _width / _nflux_x;
        double rec_dh = _height / _nflux_y;

        Vect xHat, yHat;
        yHat.i = -sin(V.rec_azimuth.val * D2R) * sin(V.rec_elevation.val * D2R);
        yHat.j = -cos(V.rec_azimuth.val * D2R) * sin(V.rec_elevation.val * D2R);
        yHat.k = cos(V.rec_elevation.val * D2R);
        xHat = Toolbox::crossprod(yHat, _normal);

        sp_point floc;
        //floc.Set(0, 0, 0);

        for (int i = 0; i < _nflux_x; i++) {
            _flux_grid.at(i).resize(_nflux_y);	//number of columns
            for (int j = 0; j < _nflux_y; j++) {
                //
                floc.x = ((-_width + rec_dw) / 2. + (double)i * rec_dw) * xHat.i + ((-_height + rec_dh) / 2. + (double)j * rec_dh) * yHat.i;
                floc.y = ((-_width + rec_dw) / 2. + (double)i * rec_dw) * xHat.j + ((-_height + rec_dh) / 2. + (double)j * rec_dh) * yHat.j;
                floc.z = ((-_width + rec_dw) / 2. + (double)i * rec_dw) * xHat.k + ((-_height + rec_dh) / 2. + (double)j * rec_dh) * yHat.k;


                //Set up the point
                _flux_grid.at(i).at(j).Setup(floc, _normal, _max_flux);
            }
        }

        break;
    }
	case Receiver::REC_GEOM_TYPE::CYLINDRICAL_OPEN:
	{		
		//1 | Continuous open cylinder - external
		//2 | Continuous open cylinder - internal cavity

		/*
		The flux map for this geometry allows an angling in the zenith direction of the surface.
		The coordinates of the flux map are with respect to the xyz location of the receiver centroid.
		These coordinates account for zenith rotation of the receiver. 
        
        Flux points are stored beginning lower edge, clockwise extent. Final entry upper edge counterclockwise extent
		
        */
        
        double intmult = ( rec_geom == Receiver::REC_GEOM_TYPE::CYLINDRICAL_CAV ? -1. : 1. );   //-1 multiplier for values that are inverted on the internal face of a cylinder
        double spansize = (_span_cw - _span_ccw) * intmult;

		_area = _height * _radius * spansize;

		//Resize
		_flux_grid.resize(_nflux_x); //Number of rows
		
		//The azimuthal span taken up by each point
		double daz = spansize/double(_nflux_x)  ;
		double rec_az = atan2(_normal.i, _normal.j);	//The azimuth angle of the receiver
		double rec_zen = acos(_normal.k);	//The zenith angle of the receiver at rec_az
		double rec_dh = _height/double(_nflux_y);

		double faz, fzen;
		sp_point floc;
		Vect fnorm;
		for(int i=0; i<_nflux_x; i++){
			_flux_grid.at(i).resize(_nflux_y);	//number of columns

			faz = _span_cw - daz*(0.5+double(i));
			fzen = rec_zen*cos(rec_az - faz);	//Local receiver zenith angle
			for(int j=0; j<_nflux_y; j++){
				//Calculate the xyz position assuming no rotation, then rotate into the position of the receiver
				floc.x = _radius*sin(faz);
				floc.y = _radius*cos(faz);
				floc.z = -_height/2.+rec_dh*(0.5 + double(j));
				//Calculate the normal vector
				fnorm.i = sin(faz)*cos(fzen)*intmult;
				fnorm.j = cos(faz)*cos(fzen)*intmult;
				fnorm.k = sin(fzen);

				//rotate about the x axis (zenith)
				Toolbox::rotation(rec_zen, 0, floc);	//Rotate the actual point
				Toolbox::rotation(rec_zen, 0, fnorm);	//rotate the normal vector
				//rotate about the z axis (azimuth)
				Toolbox::rotation(rec_az, 2, floc);     //point
                Toolbox::rotation(rec_az, 2, fnorm);    //normal vector

				//the point "floc" is now rotated into the receiver coordinates
				_flux_grid.at(i).at(j).Setup(floc, fnorm, _max_flux);
			}
		}
		break;
	}
	case Receiver::REC_GEOM_TYPE::PLANE_RECT:
	case Receiver::REC_GEOM_TYPE::FALL_FLAT:
	{		//3 | Planar rectangle, or 7 | Planar surface of a cavity, or flat falling particle curtain
		/* 
		The receiver is a rectangle divided into _nflux_x nodes in the horizontal direction and
		_nflux_y nodes in the vertical direction. Each node is of area A_rec/(_nflux_x * _nflux_y).
		*/

		_area = _height * _width;

		_flux_grid.resize(_nflux_x); //Number of rows
		double rec_az = atan2(_normal.i, _normal.j);	//The azimuth angle of the receiver
		double rec_zen = acos(_normal.k);				//The zenith angle of the receiver at rec_az
		double rec_dh = _height/double(_nflux_y);
		double rec_dw = _width/double(_nflux_x);

		sp_point floc;
		for(int i=0; i<_nflux_x; i++){
			_flux_grid.at(i).resize(_nflux_y);	//number of columns
			for(int j=0; j<_nflux_y; j++){
				//Calculate the position assuming no rotation, then rotate according to the receiver orientation
				floc.x = (-_width + rec_dw)/2. + i*rec_dw;
				floc.y = (-_height + rec_dh)/2. + j*rec_dh;
				floc.z = 0.;
				
				//Rotate about the x axis (zenith)
				Toolbox::rotation(-rec_zen, 0, floc);
				//Rotate about the z axis (azimuth)
				Toolbox::rotation(PI + rec_az, 2, floc);

				//Set up the point
				_flux_grid.at(i).at(j).Setup(floc, _normal, _max_flux);
			}
		}
		break;
	}
	case Receiver::REC_GEOM_TYPE::FALL_CURVE:
	{
		/*
		The coordinates of the flux map are with respect to the xyz location of the receiver centroid.
		Flux points are stored beginning lower edge, clockwise extent. Final entry upper edge counterclockwise extent
		*/
		double spansize = _span_cw - _span_ccw;
		_area = _height * _radius * spansize;
		_flux_grid.resize(_nflux_x); // Resize Number of rows

		//The azimuthal span taken up by each point
		double daz = spansize / double(_nflux_x);
		double rec_az = atan2(_normal.i, _normal.j);	//The azimuth angle of the receiver
		double rec_zen = acos(_normal.k);				//The zenith angle of the receiver at rec_az
		double rec_dh = _height / double(_nflux_y);

		double faz;
		sp_point floc;
		Vect fnorm;
		for (int i = 0; i < _nflux_x; i++) {
			_flux_grid.at(i).resize(_nflux_y);	//number of columns

			faz = _span_cw - daz * (0.5 + double(i));
			for (int j = 0; j < _nflux_y; j++) {
				//Calculate the xyz position assuming no rotation, then rotate into the position of the receiver
				floc.x = -_radius * sin(faz);
				floc.y = -_radius * cos(faz);
				floc.z = -_height / 2. + rec_dh * (0.5 + double(j));
				//Calculate the normal vector
				fnorm.i = sin(faz);
				fnorm.j = cos(faz);
				fnorm.k = 0; // curtain is always vertical 

				//rotate about the z-axis (azimuth)
				Toolbox::rotation(rec_az, 2, floc);     //point
				Toolbox::rotation(rec_az, 2, fnorm);    //normal vector

				_flux_grid.at(i).at(j).Setup(floc, fnorm, _max_flux);
			}
		}
		break;
	}
	case Receiver::REC_GEOM_TYPE::PLANE_ELLIPSE:
	{		//4 | Planar ellipse
		/* 
		The receiver is a rectangle divided into _nflux_x nodes in the horizontal direction and
		_nflux_y nodes in the vertical direction. Each node is of area A_rec/(_nflux_x * _nflux_y).
		*/

		_area = PI * _width * _height/4.;

		_flux_grid.resize(_nflux_x); //Number of rows
		double rec_az = atan2(_normal.i, _normal.j);	//The azimuth angle of the receiver
		double rec_zen = acos(_normal.k);	//The zenith angle of the receiver at rec_az
		double rec_dh = _height/double(_nflux_y);
		double rec_dw = _width/double(_nflux_x);
		
		sp_point floc;
		for(int i=0; i<_nflux_x; i++){
			_flux_grid.at(i).resize(_nflux_y);	//number of columns
			for(int j=0; j<_nflux_y; j++){
				//Calculate the position assuming no rotation, then rotate according to the receiver orientation
				floc.x = (-_width + rec_dw)/2. + i*rec_dw;
				floc.y = 0.;
				floc.z = (-_height + rec_dh)/2. + j*rec_dh;
				
				//Calculate an area factor to account for incongruence of the rectangular node with the elliptical aperture
				double rect[] = {floc.x, floc.z, rec_dw, rec_dh};
				double ellipse[] = {_width, _height};
				double afactor = fmin(fmax(Toolbox::intersect_ellipse_rect(rect, ellipse)/(rec_dw*rec_dh), 0.), 1.);

				//Rotate about the x axis (zenith)
				Toolbox::rotation(-rec_zen + PI/2., 0, floc);       //unlike plane rect, the points start in X-Z plane
				//Rotate about the z axis (azimuth)
				Toolbox::rotation(PI + rec_az, 2, floc);

				//Set up the point
				_flux_grid.at(i).at(j).Setup(floc, _normal, _max_flux, afactor);
			}
		}
		break;
	}
	case Receiver::REC_GEOM_TYPE::POLYGON_CLOSED:
	{
		
		//The flux for this geometry assumes that the cylinder is vertical (no zenith displacement)

		_area = _height * _radius * PI * 2.;

		//Resize
		_flux_grid.resize(_nflux_x);	//number of rows

		//The azimuthal span taken up by each flux point
		double span = (_span_cw - _span_ccw);
		double daz = span/double(_nflux_x);	//span will always be 2 PI for this

		int npanels = V.n_panels.val; 
		//calculate the angular span each panel occupies
		double panel_az_span = span / (double)npanels;
		
		//pre-calculate normal vectors for each of the panels
		vector<Vect> panel_normals(npanels);
		vector<double> panel_radii(npanels);
		vector<double> panel_azimuths(npanels);
		
        double rec_az = atan2(_normal.i, _normal.j);	//The azimuth angle of the receiver
		double rec_zen = acos(_normal.k);	//The zenith angle of the receiver at rec_az
		
		for(int i=0; i<npanels; i++){
			double paz = _span_cw - (i + 0.5)*panel_az_span;
			double 
				sinpaz = sin(paz),
				cospaz = cos(paz);
			panel_normals.at(i).Set(sinpaz, cospaz, 0.);
			panel_radii.at(i) = _radius*cos(panel_az_span/2.) ;
			panel_azimuths.at(i) = paz + rec_az;

            //rotate panels according to receiver elevation/azimuth
            Toolbox::rotation(rec_zen + PI/2., 0, panel_normals.at(i));
            Toolbox::rotation(rec_az, 2, panel_normals.at(i));

		}

		double faz;
		Vect fnorm;
		for(int i=0; i<_nflux_x; i++){
			_flux_grid.at(i).resize(_nflux_y);	//number of columns

			faz = _span_cw - daz*(0.5 + double(i)) + rec_az;	//The azimuth angle of the point

			//which panel does this flux point belong to?
			int ipanl = (int)(floor(faz / panel_az_span));

			//the normal vector is the same as the panel to which it belongs
			fnorm.Set(panel_normals.at(ipanl));
			
			//Determine the flux point position, which must lie along an existing panel
			
			double h = panel_radii.at(i)/cos( panel_azimuths.at(i) - faz ); //hypotenuse 

			//x-y location of flux point
			sp_point floc;
			floc.x = h * sin(faz - rec_az);
			floc.y = h * cos(faz - rec_az);

			//Calculate the z position
			double dz = _height/double(_nflux_y);	//height of each flux node
			for(int j=0; j<_nflux_y; j++){
				floc.z = -_height/2.+dz*(0.5 + double(j));

                // rotate
                Toolbox::rotation(rec_zen + PI/2., 0, floc);
                Toolbox::rotation(rec_az, 2, floc);

				//Set the location
				_flux_grid.at(i).at(j).Setup(floc, fnorm, _max_flux);
			}
		}
		break;
	}
	case Receiver::REC_GEOM_TYPE::POLYGON_OPEN:
	default:
		break;
	}

}

double FluxSurface::getTotalFlux(){
	//Determine the total flux on the surface
	double flux_tot=0.;
	for(int i=0; i<_nflux_x; i++){
		for(int j=0; j<_nflux_y; j++){
			flux_tot += _flux_grid.at(i).at(j).flux;
		}
	}
	return flux_tot;
}

//Express each node on the flux map as a relative contribution toward the total absorbed flux, which is equal to 1.0.
void FluxSurface::Normalize(){
	/* 
	Express each node on the flux map as a relative contribution toward the total 
	absorbed flux, which is equal to 1.0. 
	e.g:
	sum_i=0->nfx( sum_j=0->nfy( flux[i][j] )) = 1.0
	*/

	double flux_tot = getTotalFlux();
	flux_tot = max(flux_tot, 1.e-6); //make sure flux total is positive
	//Normalize by the total
	for(int i=0; i<_nflux_x; i++){
		for(int j=0; j<_nflux_y; j++){
			_flux_grid.at(i).at(j).flux *= 1./flux_tot;
		}
	}

}

void FluxSurface::Scale(double flux_mult /*-*/) {

	for (int i = 0; i < _nflux_x; i++) {
		for (int j = 0; j < _nflux_y; j++) {
			_flux_grid.at(i).at(j).flux *= flux_mult;
		}
	}
}

//-----------------Receiver----------------

void Receiver::Create(var_receiver &V, double tht)
{
    _var_receiver = &V;

    _is_enabled = V.is_enabled.val;
	
	_normal = PointVect(0.,0.,0.,0.,1.,0.); //Unit vector of the normal to the reciever

	DefineReceiverGeometry();

    updateCalculatedParameters(V, tht);

}

void Receiver::updateCalculatedParameters(var_receiver &V, double tht)
{


    //update the receiver geometry type
    switch(_var_receiver->rec_type.mapval())
    {
        case var_receiver::REC_TYPE::EXTERNAL_CYLINDRICAL:
        {
		    if(! _var_receiver->is_open_geom.val)
            {
			    _rec_geom = _var_receiver->is_polygon.val ? 
				    Receiver::REC_GEOM_TYPE::POLYGON_CLOSED : 
				    Receiver::REC_GEOM_TYPE::CYLINDRICAL_CLOSED 
                    ;		/*	0 | Continuous closed cylinder - external	*/

		    }
		    else{
			    _rec_geom = _var_receiver->is_polygon.val ? 
				    Receiver::REC_GEOM_TYPE::POLYGON_OPEN : 
				    Receiver::REC_GEOM_TYPE::CYLINDRICAL_OPEN
                    ;		/*	1 | Continuous open cylinder - external	*/
		    }
			break;
	    }
        case var_receiver::REC_TYPE::CAVITY: 
        {
        	if(! _var_receiver->is_polygon.val){		/*	2 | Continuous open cylinder - internal cavity	*/
	    		_rec_geom = Receiver::REC_GEOM_TYPE::CYLINDRICAL_CAV;  //<< Should be this. Start with plane rect for debugging
				//_rec_geom = Receiver::REC_GEOM_TYPE::PLANE_RECT;
	    	}
	    	else
			{
                throw spexception("Unsupported geometry type");
                _rec_geom = Receiver::REC_GEOM_TYPE::POLYGON_CAV;			/*	7 | Discrete open N-polygon - internal cavity	*/
	    	}
			break;
	    }
        case var_receiver::REC_TYPE::FLAT_PLATE:
        {   //Flat plate
		    if(_var_receiver->aperture_type.mapval() == var_receiver::APERTURE_TYPE::RECTANGULAR){
			    _rec_geom = ( Receiver::REC_GEOM_TYPE::PLANE_RECT );			/*	3 | Planar rectangle	*/
		    }
		    else{
			    _rec_geom = ( Receiver::REC_GEOM_TYPE::PLANE_ELLIPSE );			/* 4 | Planar ellipse		*/
				// TODO: Can Plane_Ellipse be removed?
		    }
            break;
	    }
		case var_receiver::REC_TYPE::FALLING_PARTICLE:
		{	// Falling particle reciever
			if (_var_receiver->curtain_type.mapval() == var_receiver::CURTAIN_TYPE::FLAT) {
				_rec_geom = (Receiver::REC_GEOM_TYPE::FALL_FLAT);
			}
			else if (_var_receiver->curtain_type.mapval() == var_receiver::CURTAIN_TYPE::CURVED) {
				_rec_geom = (Receiver::REC_GEOM_TYPE::FALL_CURVE);
			}
			break;
		}
        default:
            break;
    };

    //aspect
	double height = V.rec_height.val;
	double aspect;
    switch(V.rec_type.mapval() )
    {
    case var_receiver::REC_TYPE::EXTERNAL_CYLINDRICAL:
    {
		//External receiver
		aspect = height/V.rec_diameter.val;
		//the aperture area is the projected area of the cylinder
		V.aperture_area.Setval(height * V.rec_diameter.val);
        break;
	}
	case var_receiver::REC_TYPE::CAVITY:
	{
		//cavity
		//Calculate the aperture height and set the value
		V.rec_cav_aph.Setval(V.rec_height.val * (1. - V.rec_cav_blip.val + V.rec_cav_tlip.val));
		//the dimensional depth of the cavity centroid offset
		double cdepth = V.rec_cav_cdepth.val * V.rec_cav_rad.val;
		//calculate the aperture width and set the value
		double max_width = sqrt(V.rec_cav_rad.val * V.rec_cav_rad.val - cdepth * cdepth) * 2.;
		V.rec_cav_apw.Setval(max_width * V.rec_cav_apwfrac.val);
		//aspect ratio of the aperture
		aspect = V.rec_cav_aph.Val() / V.rec_cav_apw.Val();
		//calculate aperture area
		V.aperture_area.Setval(V.rec_cav_aph.Val() * V.rec_cav_apw.Val());
		break;
	}
    case var_receiver::REC_TYPE::FLAT_PLATE:
    {
		//flat plate
		aspect = height/V.rec_width.val;
		//aperture area
		V.aperture_area.Setval(height * V.rec_width.val);
        break;
	}
	case var_receiver::REC_TYPE::FALLING_PARTICLE:
	{
		// Falling particle
		aspect = height / V.rec_width.val;
		//aperture area
		V.aperture_area.Setval(height * V.rec_width.val);
		//curtain height and width
		V.curtain_total_height.Setval(V.norm_curtain_height.val * V.rec_height.val);
		V.max_curtain_width.Setval(V.norm_curtain_width.val * V.rec_width.val);
		break;
	}
    //else{
    default:
        throw spexception("Invalid receiver type in UpdateCalculatedMapValues()");
    }

	V.rec_aspect.Setval( aspect ); 

	//Receiver area
	CalculateAbsorberArea();
	V.absorber_area.Setval( _absorber_area );   //calculated by CalculateAbsorberArea

	//receiver optical height
	double zoff = V.rec_offset_z_global.Val();
	V.optical_height.Setval( tht + zoff );

	//Estimated heat loss
	double tp = 0.;
	for(int i=0; i<(int)V.therm_loss_load.val.ncells(); i++)
        tp += V.therm_loss_load.val.at(i);

	double therm_loss_base = V.therm_loss_base.val;
	V.therm_loss.Setval( therm_loss_base * _absorber_area/1.e3 * tp);

	//Piping loss
	V.piping_loss.Setval( (V.piping_loss_coef.val * tht + V.piping_loss_const.val)/1.e3 );

    //thermal efficiency
    double qdesplus = V.q_rec_des.Val() + V.piping_loss.Val() + V.therm_loss.Val();
    V.therm_eff.Setval(V.q_rec_des.Val() / qdesplus);

    updateUserFluxNormalization(V);
}

void Receiver::updateUserFluxNormalization(var_receiver &V)
{
    //user flux profile normalization
    if (V.flux_profile_type.mapval() == var_receiver::FLUX_PROFILE_TYPE::USER)
    {
        matrix_t<double> temp;
        double tot = 0.;
        for (size_t i = 0; i < V.user_flux_profile.val.nrows(); i++)
            for (size_t j = 0; j < V.user_flux_profile.val.ncols(); j++)
                tot += V.user_flux_profile.val.at(i, j);

        tot = 1. / tot;
        for (size_t i = 0; i < V.user_flux_profile.val.nrows(); i++)
            for (size_t j = 0; j < V.user_flux_profile.val.ncols(); j++)
                V.user_flux_profile.val.at(i, j) *= tot;
        
        V.n_user_flux_profile.Setval(temp);
    }
}


//------------Access functions
double Receiver::getReceiverWidth(var_receiver &V) 
{
    //[m] Returns either receiver (or aperture) width or diameter, depending on configuration
	switch (V.rec_type.mapval())
	{
		case var_receiver::REC_TYPE::EXTERNAL_CYLINDRICAL:
			return V.rec_diameter.val;
		case var_receiver::REC_TYPE::FLAT_PLATE:
			return V.rec_width.val;
		case var_receiver::REC_TYPE::CAVITY:
			return V.rec_cav_apw.Val();
		case var_receiver::REC_TYPE::FALLING_PARTICLE:
			return V.rec_width.val; 
    } 
} 

double Receiver::getReceiverThermalLoss()
{
    return _therm_loss;
}

double Receiver::getReceiverPipingLoss()
{
    return _piping_loss;
}

double Receiver::getThermalEfficiency()
{
    return _thermal_eff;
}

double Receiver::getAbsorberArea()
{
    return _absorber_area;
}

int Receiver::getGeometryType()
{
    return _rec_geom;
}
    
FluxSurfaces *Receiver::getFluxSurfaces(){ return &_surfaces; }

var_receiver* Receiver::getVarMap(){return _var_receiver;}

std::vector< Heliostat* > *Receiver::getHeliostatPreferenceList()
{
    return &_heliostat_preference_list;
}

bool Receiver::isReceiverEnabled()
{
    return _is_enabled;
}

void Receiver::isReceiverEnabled(bool enable)
{
    _is_enabled = enable;
}

void Receiver::CalculateNormalVector(PointVect &NV){
	//If no normal vector is supplied, provide the default
	sp_point Vn;
	Vn.Set(0., 0., 0.);
	Receiver::CalculateNormalVector(Vn, NV);
}

void Receiver::CalculateNormalVector(sp_point &Hloc, PointVect &NV){
	/* 
	This subroutine should be used to calculate the normal vector to the receiver for a given heliostat location.
	Ultimately, the optical calculations should not use this method to calculate the normal vector. Instead, use
	the normal vector that is assigned to the receiver surface during setup. 

	In the case of continuous cylindrical surfaces, this method can be called during optical calculations.

	Given a heliostat at point Hloc{x,y,z}, return a normal vector to the receiver absorber surface.
	
	*/
	
    double rec_elevation = _var_receiver->rec_elevation.val * D2R;
	if (_var_receiver->rec_type.mapval() == var_receiver::REC_TYPE::FALLING_PARTICLE) rec_elevation = 0.0;
    double rec_az = _var_receiver->rec_azimuth.val * D2R;

	//This will have to be modified to allow for multi-surface receivers and polygons. TO DO
	switch(_rec_geom)
	{
	case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CLOSED:
	case Receiver::REC_GEOM_TYPE::POLYGON_CLOSED:

		//External cylinder
		//use the view vector to determine the best normal to report
		
		//Polar coords for azimuth and zenith angles
		double vaz; 
		vaz = atan2(Hloc.x,Hloc.y);
		
		//What is the approximate aim point for the surface?
		NV.z = _var_receiver->optical_height.Val();
		NV.x = _var_receiver->rec_diameter.val/2. * sin(vaz) + _var_receiver->rec_offset_x_global.Val();		//[m] x-location of surface at angle vaz, given radius _var_receiver->rec_diameter.val/2
		NV.y = _var_receiver->rec_diameter.val/2. * cos(vaz) + _var_receiver->rec_offset_y_global.Val();		//[m] y-location "" "" ""

		//calculate the normal vector
        NV.i = sin(vaz)*cos(rec_elevation);
		NV.j = cos(vaz)*cos(rec_elevation);
		NV.k = sin(rec_elevation);
		break;
	case Receiver::REC_GEOM_TYPE::CYLINDRICAL_OPEN:
	case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CAV:
	case Receiver::REC_GEOM_TYPE::PLANE_RECT:
	case Receiver::REC_GEOM_TYPE::PLANE_ELLIPSE:
	case Receiver::REC_GEOM_TYPE::FALL_FLAT:
	case Receiver::REC_GEOM_TYPE::FALL_CURVE:
		//All other types should be simply equal to the user-specified az/zen
		//The approximate aim point is:
		NV.x = _var_receiver->rec_offset_x_global.Val();
		NV.y = _var_receiver->rec_offset_y_global.Val();
		NV.z = _var_receiver->optical_height.Val();
		//Calculate the unit vector
		NV.i = sin(rec_az)*cos(rec_elevation);
		NV.j = cos(rec_az)*cos(rec_elevation);
		NV.k = sin(rec_elevation);
		break;
	default:
		throw spexception("Unsupported receiver type");
	}

	return;

}

//------------------Scripts------------------

//Initialization call to create the receiver surfaces
void Receiver::DefineReceiverGeometry(int nflux_x, int nflux_y) 
{
	/* 
	The process of defining receiver geometry for each receiver should be:

	1) Indicate which specific geometry type should be used with "_rec_geom"
	2) Calculate and set the number of surfaces used for the receiver. Resize "_surfaces".
	3) Calculate and set the normal vector for each surface (if not curved surfaces) with setNormalVector(Vect).
	4) Setup the geometry etc.. including setSurfaceGeometry, setSurfaceOffset, setSurfaceSpanAngle, if applicable.
	5) Define the precision of the flux map.
	6) Define the maximum flux for each panel.
	7) Call the method to set up the flux hit test grid.

	Geometries are:
	0	|	Continuous closed cylinder - external
	1	|	Continuous open cylinder - external	
	2	|	Continuous open cylinder - internal cavity
	3	|	Planar rectangle
	4	|	Planar ellipse
	5	|	Discrete closed N-polygon - external	
	6	|	Discrete open N-polygon - external
	7	|	Discrete open N-polygon - internal cavity
	*/
    int rec_type = _var_receiver->rec_type.mapval();

	if(rec_type == var_receiver::REC_TYPE::EXTERNAL_CYLINDRICAL){ //External
		//if(! _is_polygon){
		/*continuous external cylinders. Setup shares some common features..*/
			
		//this uses a single curved surface
		_surfaces.resize(1);

		//force nPanels to 1 for external receiver
		getVarMap()->n_panels.val = 1;		
				
		FluxSurface *S = &_surfaces.at(0);
		S->setParent(this);

		//Do setup
		sp_point loc;
        loc.Set(_var_receiver->rec_offset_x_global.Val(), _var_receiver->rec_offset_y_global.Val(), _var_receiver->rec_offset_z_global.Val());
		S->setSurfaceGeometry( _var_receiver->rec_height.val, 0., _var_receiver->rec_diameter.val/2. );
		S->setSurfaceOffset( loc );
		//For continuous cylindrical surfaces, the normal vector will define the azimuth and zenith of the receiver surface.
		Vect nv;
        double rec_az = _var_receiver->rec_azimuth.val * D2R;
        double rec_el = _var_receiver->rec_elevation.val * D2R;
		nv.i = sin(rec_az)*cos(rec_el);
		nv.j = cos(rec_az)*cos(rec_el);
		nv.k = sin(rec_el);
		S->setNormalVector(nv); 
						
		if(! _var_receiver->is_open_geom.val){
			S->setSurfaceSpanAngle(-PI,PI);	//Full surround
		}
			
		//Default setup will be for a single flux test point on the surface. In more detailed
		//flux mapping runs, this can be changed to whatever the desired resolution is.
		S->setFluxPrecision(nflux_x,nflux_y);
		S->setMaxFlux(_var_receiver->peak_flux.val);
		S->DefineFluxPoints(*_var_receiver, _rec_geom);
	}
	else if(rec_type == var_receiver::REC_TYPE::CAVITY){ //Cavity
		//Use the number of panels as the number of polygon facets. Each facet is its own surface.
		_surfaces.resize(1 + _var_receiver->n_panels.val); //cavity aperture plus panels

        //Create flux surface for aperture
        FluxSurface* S = &_surfaces.at(0);
        S->setParent(this);

        //calculate the span of the receiver surfaces (angle from edge of receiver to centroid to other receiver edge)
        double span = PI + 2. * asin(_var_receiver->rec_cav_cdepth.val);
        //span of a single panel
        double panel_span = span / _var_receiver->n_panels.val;
        //calculate single panel width
        double panel_width = panel_span * _var_receiver->rec_cav_rad.val;
        //calculate aperture width
        double ap_width = 2. * _var_receiver->rec_cav_rad.val * cos((span - PI) / 2.);

        sp_point loc;  //Define aperture location and geometry
        loc.Set(0., 0., 0.);   //the flux surface offset relative to receiver coordinates should be zero for single-aperture receivers
        S->setSurfaceGeometry(_var_receiver->rec_height.val, ap_width, 0.);
        S->setSurfaceOffset(loc);

        Vect nv;    //Define aperture normal vector
        double rec_az = _var_receiver->rec_azimuth.val * D2R;
        double rec_elevation = _var_receiver->rec_elevation.val * D2R;
        nv.i = sin(rec_az) * cos(rec_elevation);
        nv.j = cos(rec_az) * cos(rec_elevation);
        nv.k = sin(rec_elevation);
        S->setNormalVector(nv);
        S->setSurfaceSpanAngle(-PI / 2., PI / 2.);
        S->setFluxPrecision(nflux_x, nflux_y); //Aperture flux parameters
        S->setMaxFlux(_var_receiver->peak_flux.val);
        S->DefineFluxPoints(*_var_receiver, Receiver::REC_GEOM_TYPE::PLANE_RECT);

		//calculate the vector offset between the center of the aperture and the center of 
		//the circle circumscribing the panels - This is actually the reversed direction
		double cav_ap_offset = _var_receiver->rec_cav_cdepth.val * _var_receiver->rec_cav_rad.val;
		Vect ap_offset;
		ap_offset.i = cav_ap_offset * sin(_var_receiver->rec_azimuth.val * D2R) * cos(_var_receiver->rec_elevation.val * D2R);
		ap_offset.j = cav_ap_offset * cos(_var_receiver->rec_azimuth.val * D2R) * cos(_var_receiver->rec_elevation.val * D2R); //TODO: Why does this not have a cos(elevation)?
		ap_offset.k = cav_ap_offset * sin(_var_receiver->rec_elevation.val * D2R);

		for(int i=1; i<=_var_receiver->n_panels.val; i++){
			S = &_surfaces.at(i);
			S->setParent(this);

			//Setup the geometry etc.. including setSurfaceGeometry, setSurfaceOffset
			
			/*
			Calculate the panel width based on the total span angle. 
			
			The span angle is defined as follows
				* Assume the panels making up the cavity receiver surface are part of a regular polygon that can
				  be circumscribed by a circle.
				* Consider the circle that fully circumscribes the receiver surfaces. This circle passes through the 
				  vertices of the surfaces.
				* The span is the slice of the circle that ranges from most CCW edge to the most CW edge of all 
				  surfaces. 
				* The center of the circle does not need to lie in the aperture plane.
			*/
				
			
			S->setSurfaceGeometry(_var_receiver->rec_height.val, panel_width);
               
			//Calculate the azimuth angle of the receiver panel
			double paz = _var_receiver->rec_azimuth.val*D2R - PI + span/2. - panel_span * (double)(i-0.5);
            if (paz < -PI) { paz = paz + 2 * PI; }

			//Calculate the elevation angle of the panel
			double pel =  _var_receiver->rec_elevation.val*D2R*cos(-paz);
			//Set the surface normal vector
			Vect nv;
			nv.i = -sin(paz)*cos(pel);
			nv.j = -cos(paz)*cos(pel);
			nv.k = -sin(pel);
			S->setNormalVector(nv);
			//Calculate the centroid of the panel in global XYZ coords
			sp_point pc;
			pc.x = -nv.i * _var_receiver->rec_cav_rad.val - ap_offset.i + _var_receiver->rec_offset_x_global.Val();
			pc.y = -nv.j * _var_receiver->rec_cav_rad.val - ap_offset.j + _var_receiver->rec_offset_y_global.Val();
			pc.z = -nv.k * _var_receiver->rec_cav_rad.val - ap_offset.k + _var_receiver->rec_offset_z_global.Val();

			S->setSurfaceOffset(pc);
			//Define the precision of the flux map.
			S->setFluxPrecision(nflux_x,nflux_y);
			S->setMaxFlux(_var_receiver->peak_flux.val);
			//Call the method to set up the flux hit test grid.
			//S->DefineFluxPoints(*_var_receiver, _rec_geom);
            S->DefineFluxPoints(*_var_receiver, Receiver::REC_GEOM_TYPE::CYLINDRICAL_CAV);
		}
	}
	else if(rec_type == var_receiver::REC_TYPE::FLAT_PLATE){ //Flat plate
		//1) Indicate which specific geometry type should be used with "_rec_geom"
		//if(_var_receiver->aperture_type.mapval() == var_receiver::APERTURE_TYPE::RECTANGULAR){
		//	_rec_geom = ( Receiver::REC_GEOM_TYPE::PLANE_RECT );			/*	3 | Planar rectangle	*/
		//}
		//else{
		//	_rec_geom = ( Receiver::REC_GEOM_TYPE::PLANE_ELLIPSE );			/* 4 | Planar ellipse		*/
		//}
			
		//2) Calculate and set the number of surfaces used for the recever. Resize "_surfaces".
		_surfaces.resize(1);
		FluxSurface *S = &_surfaces.at(0);
        S->setParent(this);

		//3) Calculate and set the normal vector for each surface (if not curved surfaces) with setNormalVector(Vect).

		sp_point loc;
        loc.Set(0., 0., 0.);        //the flux surface offset relative to receiver coordinates should be zero for single-aperture receivers
        /*loc.Set( _var_receiver->rec_offset_x_global.Val(), _var_receiver->rec_offset_y_global.Val(), _var_receiver->rec_offset_z_global.Val() );*/
		
        S->setSurfaceGeometry( _var_receiver->rec_height.val, _var_receiver->rec_width.val, 0. );
		S->setSurfaceOffset( loc );
		
        //For continuous cylindrical surfaces, the normal vector will define the azimuth and zenith of the receiver surface.
		Vect nv;
        double rec_az = _var_receiver->rec_azimuth.val *D2R;
        double rec_elevation = _var_receiver->rec_elevation.val *D2R;
		nv.i = sin(rec_az)*cos(rec_elevation);
		nv.j = cos(rec_az)*cos(rec_elevation);
		nv.k = sin(rec_elevation);
		S->setNormalVector(nv);	
						
		//4) Setup the geometry etc.. including setSurfaceGeometry, setSurfaceOffset, setSurfaceSpanAngle, if applicable.
		S->setSurfaceSpanAngle(-PI/2., PI/2.);	
						
		//5) Define the precision of the flux map.
		S->setFluxPrecision(nflux_x,nflux_y);
			
		//6) Define the maximum flux for each panel.
		S->setMaxFlux(_var_receiver->peak_flux.val);
			
		//7) Call the method to set up the flux hit test grid.
		S->DefineFluxPoints(*_var_receiver, _rec_geom);	

	}
	else if (rec_type == var_receiver::REC_TYPE::FALLING_PARTICLE) {
		//1) Indicate which specific geometry type should be used with "_rec_geom"
		//_rec_geom = (Receiver::REC_GEOM_TYPE::PLANE_RECT); //Set during updateCalculatedParameters() call
		//2) Calculate and set the number of surfaces used for the recever.Resize "_surfaces".
		int n_troughs = _var_receiver->norm_heights_depths.val.nrows();
        if (_var_receiver->norm_heights_depths.val.at(0, 0) == 0) {
            n_troughs = 0; // norm_height of zero means there is no troughs
        }
		getVarMap()->n_panels.val = n_troughs + 1;
		_surfaces.resize(1 + n_troughs + 1); //Aperture plus curtains between troughs (n+1)
		//3) Calculate and set the normal vector for each surface (if not curved surfaces) with setNormalVector(Vect).
		//Create flux surface for aperture -> always at index zero
		FluxSurface* S = &_surfaces.at(0);
		S->setParent(this);
		S->setSurfaceGeometry(_var_receiver->rec_height.val, _var_receiver->rec_width.val, 0.);
		sp_point loc;
		loc.Set(0., 0., 0.);	//the flux surface offset relative to receiver coordinates should be zero for single-aperture receivers
		S->setSurfaceOffset(loc);

		// Aperture surface normal
		Vect nv;
		double rec_az = _var_receiver->rec_azimuth.val * D2R;
		double rec_elevation = 0.0; //_var_receiver->rec_elevation.val* D2R; Assuming no elevation angle
		nv.i = sin(rec_az) * cos(rec_elevation);
		nv.j = cos(rec_az) * cos(rec_elevation);
		nv.k = sin(rec_elevation);
		S->setNormalVector(nv);
		S->setSurfaceSpanAngle(-PI / 2., PI / 2.);
		S->setFluxPrecision(nflux_x, nflux_y); //Aperture flux parameters
		S->setMaxFlux(_var_receiver->peak_flux.val);
		S->DefineFluxPoints(*_var_receiver, Receiver::REC_GEOM_TYPE::PLANE_RECT);

		if (_var_receiver->norm_curtain_height.val < 1.0)
			throw spexception("Normalized curtain height must be greater than or equal to 1.0.");
		if (_var_receiver->norm_curtain_width.val < 1.0)
			throw spexception("Normalized curtain width must be greater than or equal to 1.0.");

		// _var_receiver->curtain_total_height.Val(); Can't be used because updateCalculatedParameters() is called after this one
		double max_height = _var_receiver->norm_curtain_height.val * _var_receiver->rec_height.val;	// Particle curtain maximum height
		double max_depth = _var_receiver->max_curtain_depth.val;		// Particle curtain maximum depth (from the aperture)
		double curtain_width = _var_receiver->norm_curtain_width.val * _var_receiver->rec_width.val; // Particle curtain width

		double last_trough_height = 1.0;	// Normalized height of previous trough 
		double last_trough_depth = 1.0;		// Normalized depth of the previous trough

		int y_flux_res;
		int nflux_y_remaining = nflux_y;
		// Generate all particle curtains - Starting at the top of the curtain and working down
		for (int i = 1; i <= n_troughs + 1; i++) {
			S = &_surfaces.at(i);
			S->setParent(this);
			
			double curtain_height;
			double height_norm = 0.0, depth_norm = 0.0;
			if (i != n_troughs + 1) { // Not the last curtain
				height_norm = _var_receiver->norm_heights_depths.val.at(i - 1, 0);
				depth_norm = _var_receiver->norm_heights_depths.val.at(i - 1, 1);

				// Value checking
				if (height_norm > last_trough_height)
					throw spexception("Troughs heights must be in descending order.");
				else if (height_norm <= 0.0)
					throw spexception("Troughs heights must be greater than zero.");
				else if (height_norm >= 1.0)
					throw spexception("Troughs heights must be less than one.");
				if (depth_norm > last_trough_depth)
					throw spexception("Troughs depths must be in descending order.");
				else if (depth_norm <= 0.0)
					throw spexception("Troughs depths must be greater than zero.");
				else if (depth_norm >= 1.0)
					throw spexception("Troughs depths must be less than one.");

				y_flux_res = (int)std::round((last_trough_height - height_norm) * nflux_y);
				nflux_y_remaining -= y_flux_res;
			}
			else {
				y_flux_res = nflux_y_remaining;
			}

			curtain_height = (last_trough_height - height_norm) * max_height;

			// Set up flux surface for a curved curtain
			bool is_curtain_curved = _var_receiver->curtain_type.mapval() == var_receiver::CURTAIN_TYPE::CURVED;
			double curtain_radius;
			if (is_curtain_curved)
			{
				curtain_radius = _var_receiver->curtain_radius.val - (1.0 - last_trough_depth) * max_depth;
				S->setSurfaceGeometry(curtain_height, 0., curtain_radius);
				double span = asin(curtain_width / 2 / _var_receiver->curtain_radius.val);
				S->setSurfaceSpanAngle(-span, span);

				double req_depth = curtain_radius - sqrt(pow(curtain_radius, 2.) - pow(curtain_width / 2., 2.));
				if (req_depth > max_depth)
					throw spexception("Cavity depth is not deep enough for curved curtain geometry. Increase curtain depth or increase curtain radius.");
				if (2. * curtain_radius < curtain_width)
					throw spexception("Curtain radius is too small for desired curtain width. Curtain radius must be at least half the curtain width." );
			}
			else
				S->setSurfaceGeometry(curtain_height, curtain_width, 0.);
			
			//Set the surface normal vector (always vertical)
			Vect nv;
			nv.i = sin(rec_az);
			nv.j = cos(rec_az);
			nv.k = 0;
			S->setNormalVector(nv);
			//Calculate the centroid of the curtain in global XYZ coords
			sp_point pc;
			double depth_offset = last_trough_depth * max_depth;
			if (is_curtain_curved) depth_offset -= curtain_radius; // Offset must account for radius of curtain
			pc.x = -nv.i * depth_offset;
			pc.y = -nv.j * depth_offset;
			pc.z = last_trough_height * max_height - curtain_height / 2.0
				- _var_receiver->rec_height.val / 2.0;
			S->setSurfaceOffset(pc);

			//5) Define the precision of the flux map.
			S->setFluxPrecision(nflux_x, y_flux_res);
			//6) Define the maximum flux for each panel.
			S->setMaxFlux(_var_receiver->peak_flux.val);
			//7) Call the method to set up the flux hit test grid.
			if (is_curtain_curved)
				S->DefineFluxPoints(*_var_receiver, Receiver::REC_GEOM_TYPE::FALL_CURVE);
			else
				S->DefineFluxPoints(*_var_receiver, Receiver::REC_GEOM_TYPE::PLANE_RECT);

			last_trough_height = height_norm;
			last_trough_depth = depth_norm;
		}
	}
}

void Receiver::CalculateAbsorberArea(){

	/* 
	Calculate the receiver absorber surface area based on the geometry type. This doesn't consider
	the area of individual tubes or elements, only the area of the major geometrical surfaces.

	The local variable _absorber_area is set, which can be accessed via
	getReceiverAbsorberArea()
	*/

	int recgeom = _rec_geom; 

	switch (recgeom)
	{
	case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CLOSED:
		_absorber_area = ( _var_receiver->rec_height.val * _var_receiver->rec_diameter.val * PI );
		break;
	case Receiver::REC_GEOM_TYPE::PLANE_RECT:
		_absorber_area = ( _var_receiver->rec_height.val * _var_receiver->rec_width.val );
		break;
	case Receiver::REC_GEOM_TYPE::POLYGON_CAV:
    case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CAV:
	{
		//calculate the span of the receiver surfaces
		double span = PI + 2. * asin(_var_receiver->rec_cav_cdepth.val);
		//span of a single panel
		double panel_span = span / _var_receiver->n_panels.val;
		//calculate single panel width
		double panel_width = panel_span * _var_receiver->rec_cav_rad.val;
		double panel_area = panel_width * _var_receiver->rec_height.val;

		_absorber_area = panel_area * (double)_var_receiver->n_panels.val;
		break;
	}
	case Receiver::REC_GEOM_TYPE::FALL_FLAT:
	{
		_absorber_area = _var_receiver->curtain_total_height.Val() * _var_receiver->max_curtain_width.Val();
		break;
	}
	case Receiver::REC_GEOM_TYPE::FALL_CURVE:
	{
		FluxSurfaces* surfaces = this->getFluxSurfaces();
		double area = 0;
		for (int i = 1; i < surfaces->size(); i++) {
			area += surfaces->at(i).getSurfaceArea();
		}
		_absorber_area = area;
		break;
	}
	//unsupported receiver geometries
	case Receiver::REC_GEOM_TYPE::CYLINDRICAL_OPEN:
		//_absorber_area = ( _var_receiver->rec_height.val * _var_receiver->rec_diameter.val * fabs(_var_receiver->span_max.val*D2R - _var_receiver->span_min.val*D2R)/2. );
		//break;
	case Receiver::REC_GEOM_TYPE::PLANE_ELLIPSE:
		//_absorber_area = ( PI * _var_receiver->rec_height.val * _var_receiver->rec_width.val/4. );
		//break;
	case Receiver::REC_GEOM_TYPE::POLYGON_CLOSED:
		//_absorber_area = ( _var_receiver->rec_height.val * (double)_var_receiver->n_panels.val * _var_receiver->rec_diameter.val/2.*tan(2.*PI/_var_receiver->n_panels.val) );
		//break;
	case Receiver::REC_GEOM_TYPE::POLYGON_OPEN:
	default:
		throw std::runtime_error("Unsupported receiver type was selected.");
		break;
	}
	
}

void Receiver::CalculateThermalLoss(double load, double v_wind){
	/* 
	Calculate the thermal loss from the receiver. Update the local values of thermal and piping loss.

    _therm_loss [MWt]   Local value updated
    _piping_loss [MWt]   Local value updated

	Load is a normalized thermal load for the receiver. 
	V_wind is m/s.
	*/

	double
		fload = 0.,
		fwind = 0.;
	for(int i=0; i<(int)_var_receiver->therm_loss_load.val.ncells(); i++)
		fload += _var_receiver->therm_loss_load.val.at(i)*pow(load, i);
	for(int i=0; i<(int)_var_receiver->therm_loss_wind.val.ncells(); i++)
		fwind += _var_receiver->therm_loss_wind.val.at(i)*pow(v_wind, i);

	_therm_loss =  _var_receiver->therm_loss_base.val * fload * fwind * _absorber_area *1.e-3 ;    //_therm_loss_base [kWt/m2]

	//piping
	_piping_loss =  (_var_receiver->piping_loss_coef.val * _var_receiver->optical_height.Val() + _var_receiver->piping_loss_const.val)*1.e-3 ;

}

void Receiver::CalculateThermalEfficiency(double dni, double dni_des, double v_wind, double q_des){
    /* 
    Calculate thermal efficiency and update local values.

    Inputs:
        DNI         W/m2    DNI at current time
        dni_des     W/m2    DNI at system design point
        v_wind      m/s     Wind velocity at current time
        q_des       MWt     Design-point receiver output power

    Sets:
        _thermal_eff
        _therm_loss     (via CalculateThermalLoss)
        _piping_loss    (via CalculateThermalLoss)
    */

	double load = dni/dni_des;
	CalculateThermalLoss(load, v_wind);

    _thermal_eff = 1. - _therm_loss / (_therm_loss + q_des ); 

}

double Receiver::CalculateApparentDiameter(sp_point &Hloc)
{ 
	/* 
	[m] Return the apparent receiver diameter given the polygonal structure

	Take the specified heliostat location, the number of receiver panels, and the orientation
	of the primary receiver panel, and calculate the apparent width of the reciever.
	
	This convention assumes that the receiver diameter CIRCUMSCRIBES all panels. That is, 
	the maximum receiver apparent width is the specified receiver diameter.
	*/

	//only valid for cylindrical receivers
	switch (_rec_geom)
	{
	case Receiver::REC_GEOM_TYPE::CYLINDRICAL_CLOSED:
		return _var_receiver->rec_diameter.val;
		break;
	case Receiver::REC_GEOM_TYPE::POLYGON_CLOSED:
	{
		//First determine the azimuthal span between the heliostat location vector and the receiver 
		//main panel normal vector
		double alpha = std::abs(atan2(Hloc.x, Hloc.y) - _var_receiver->rec_azimuth.val*D2R);
		//Calculate the difference between the angle and the nearest panel normal
		double theta_hat = fmod(alpha, 2.*PI/_var_receiver->n_panels.val);
		//finally the width is:
		return cos(theta_hat)*_var_receiver->rec_diameter.val;
		break;
	}
	default:
		throw spexception("Attempting to calculate an apparent diameter for an unsupported receiver geometry.");
		break;
	}
}

