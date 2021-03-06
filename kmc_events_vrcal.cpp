#include <iostream>
#include <vector>
#include "kmc_global.h"
#include "kmc_events.h"

using namespace std;

double class_events::cal_ratesV(vector <int> &etype, vector <double> &rates, vector <int> &ilist, vector <int> &nltcp, vector <int> &jatom){
	double sum_rate= 0;
	if(nV != list_vcc.size()) error(2, "(cal_ratesV) vcc number inconsistent", 2, nV, list_vcc.size());
	
	for(int ivcc=0; ivcc < nV; ivcc ++){
		int i= (int) (list_vcc[ivcc].ltcp/nz)/ny;
		int j= (int) (list_vcc[ivcc].ltcp/nz)%ny;
		int k= (int)  list_vcc[ivcc].ltcp%nz;

		if(states[i][j][k] != 0) error(2, "(cal_ratesV) there's an non-vacancy in the vacancy list", 2, states[i][j][k], timestep);
		
		for(int a=0; a<n1nbr; a ++){
			int x= pbc(i+v1nbr[a][0], nx);
			int y= pbc(j+v1nbr[a][1], ny);
			int z= pbc(k+v1nbr[a][2], nz);
		
            if (marker[x][y][z]){ // if itl recb w/ vcc, vcc do the same. itlAB pass info and here we avoid double-counting
                marker[x][y][z]= false; // true denotes I-V recb via the atom. alter to false and don't cal rate
                continue;
            }

			if(1==states[x][y][z] || -1==states[x][y][z]){
				double em, mu;

				if     (states[x][y][z]== 1) { em= emvA; mu= muvA;} 
				else if(states[x][y][z]==-1) { em= emvB; mu= muvB;} 
				else error(2, "(cal_ratesV) wrong type for V to jump into", 1, states[x][y][z]);

				double e0= cal_energy(false, i, j, k, x, y, z);

				states[i][j][k]= states[x][y][z];
                if(srf[x][y][z]) states[x][y][z]= 4;
                else             states[x][y][z]= 0;

				double ediff= cal_energy(false, i, j, k, x, y, z) - e0;

				states[x][y][z]= states[i][j][k];
				states[i][j][k]= 0;
			
                if((em+0.5*ediff)<0){
                    double e= em+0.5*ediff;
                    rates.push_back(e);
                    is_inf= true;
                    list_inf.push_back(rates.size()-1);
                    if(e<einf) einf= e;
                }
                else rates.push_back(mu * exp(-beta*(em+0.5*ediff)));
							
				etype.push_back(1);
				ilist.push_back(ivcc);
				nltcp.push_back(x*ny*nz+y*nz+z);
				jatom.push_back(states[x][y][z]);
				
				sum_rate += rates.back();

                if((NF_Nj != 0) && (ivcc == NF_id) && (! srf[x][y][z])){ // (no flickering)
                    list_nf.push_back(rates.size()-1);                   // (no flickering)
				    NF_rates += rates.back();                            // (no flickering)
                }                                                        // (no flickering)
			}
		}
	}

	return sum_rate;
}
