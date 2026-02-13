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


#ifndef _LIB_6PAR_NEWTON_H_
#define _LIB_6PAR_NEWTON_H_

#include <cmath>

#include "6par_jacobian.h"
#include "6par_lu.h"
#include "6par_search.h"

#include <limits>

/// Newton Method using Line Search and Backtracking
/// from Globally Convergent Methods for Nonlinear Systems of Equations, "Numerical Recipes in C"
template< typename Real, typename F, int n >
int newton( Real x[n], Real residual[n], bool &check, F &func, 
	int MAXITER, const Real TOLF, const Real TOLMIN, const Real STPMX,
	bool (*notify)(int iter, Real x[], Real resid[], const int, void *) = 0,
	void *notify_data = 0)
{
	const Real TOLX = std::numeric_limits<Real>::epsilon();
	
	int i,j,its;
	Real den,f,fold,stpmax,sum,temp,test;
	Real g[n],p[n],xold[n];
	Real fjac[n][n];
	
	Real lu[n][n];
	int permute[n];
		
	f = fminsum<Real, F, n>(x, residual, func);
	test=0.0;
	for (i=0;i<n;i++)
		if (std::abs(residual[i]) > test)
			test= std::abs(residual[i]);
		
	if (test < 0.01*TOLF)
	{
		check = false;
		return 0;
	}
	
	sum=0.0;
	for (i=0;i<n;i++)
		sum += x[i]*x[i];
		
	stpmax = STPMX*mymax(sqrt(sum), (Real)n);
	for (its=0;its<MAXITER;its++)
	{
		if ( notify != 0 )
		{
			bool ok = (*notify)(its, x, residual, n, notify_data);
			if (!ok)
				return -3;
		}

		jacobian<Real, F, n, n>( x, residual, fjac, func, 1e-8 );
		
		for (i=0;i<n;i++) 
		{
			sum=0.0;
			for (j=0;j<n;j++) sum += fjac[j][i]*residual[j];
			g[i]=sum;
		}
		
		for (i=0;i<n;i++)
			xold[i]=x[i];
			
		fold=f;
		
		for (i=0;i<n;i++)
			p[i] = -residual[i];
		
				
		if (!lu_decomp<Real, n>( fjac, lu, permute )) return false;			
		lu_solve<Real, n>( lu, permute, p, p );
		
		if (!search<Real, F, n>(xold, fold, g, p, x, f, stpmax, check, func, residual))
			return -2;
		
		test=0.0;
		for (i=0;i<n;i++)
			if (std::abs(residual[i]) > test)
				test= std::abs(residual[i]);
				
		if (test < TOLF)
		{
			check=false;
			return its+1;
		}
		
		if (check) {
			test=0.0;
			den=mymax(f,0.5*n);
			for (i=0;i<n;i++) {
				temp= std::abs(g[i])*mymax(std::abs(x[i]),1.0)/den;
				if (temp > test) test=temp;
			}
			check=(test < TOLMIN) ? true : false;
			return its+1;
		}
		
		test=0.0;
		for (i=0;i<n;i++) {
			temp=(std::abs(x[i]-xold[i]))/mymax(std::abs(x[i]),1.0);
			if (temp > test) test=temp;
		}
		
		if (test < TOLX)
			return its+1;
	}
	
	return -1;
}
#endif
