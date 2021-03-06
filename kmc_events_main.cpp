#include <cstdio>
#include <iostream>
#include <vector>
#include "kmc_global.h"
#include "kmc_events.h"

using namespace std;

double class_events::main(){
	// a probability map will first generated by calculating all possible moves
	// then randomly picked the ACTUAL move based on the probability map

	// defect information
	vector <int>    etype; // type of the event: 0: ITL JUMP; 1: VCC JUMP; 7: F-P GENR; 8: VCC CRTN
	vector <double> rates; // transition rates
	vector <int>    ilist; // IDs in the lists
	vector <int>    nltcp; // ltcp of neighbors
	vector <int>	jatom; // the jumping atom
	
	// perform imaginary jumps and cal rates
	double irates= cal_ratesI(etype, rates, ilist, nltcp, jatom); // WARNING: irates before vrates so the recb map can be generated
	double vrates= cal_ratesV(etype, rates, ilist, nltcp, jatom);
    double crates= cvcc_rates; if(is_noflckr) crates *= NFratio;  // if no flickering, multiply by the NFratio
    etype.push_back(7); rates.push_back(rate_genr); // the genr event
	double sum_rates= vrates + irates + crates + rate_genr; // sum of all rates

    // check
    if(nA+nB+nV+nAA+nBB+nAB+nM != nx*ny*nz) error(2, "(jump) numbers of ltc points arent consistent, diff=", 1, nA+nB+nV+nAA+nBB+nAB+nM-nx*ny*nz); // check
    if(2*nAA+nA-nB-2*nBB       != sum_mag)  error(2, "(jump) magnitization isnt conserved", 2, 2*nAA+nA-nB-2*nBB, sum_mag);

    // perform the actual jump
    if(is_inf){
        if(0==list_inf.size()) error(2, "(main) no inf event is collected but is_inf= true");

        for(int a=0; a<1000; a ++){ // if iterate more than 1000 times, should due to sth weird
	        double ran= ran_generator();
            int i= list_inf[(int) (ran*list_inf.size())]; // the randomly picked inf event that will be performed
        
            if(rates[i]>0) error(2, "(main) an inf event with e gt 0", 1, rates[i]); // in inf events rates contain e
            else if(abs(rates[i] - einf)< 1e-10){
                switch(etype[i]){
                    case 0:  actual_jumpI(ilist[i], nltcp[i], jatom[i]); break;
			        case 1:  actual_jumpV(ilist[i], nltcp[i], jatom[i]); break;
                    default: error(2, "(main) an unknown event type in inf events", 1, etype[i]);
                }
        
                is_inf= false;
                einf= 0;
                list_inf.clear();

                NF_Nj= 0;       // (no flickering)
                NF_rates= 0;    // (no flickering)
                list_nf.clear();// (no flickering)

                return 0;
            }
        }

        error(2, "(main) inf event iterate more than 1000 times, weird!");
    }
    else if(NF_Nj != 0){                        // (no flickering)
	    double ran= ran_generator();
	    
        double acc_nf= 0; // accumulated rate
	    for(int a=0; a<list_nf.size(); a ++){   // (no flickering)
            int i= list_nf.at(a);               // (no flickering)
            if( (ran >= acc_nf) && (ran < (acc_nf + rates[i]/NF_rates) ) ){			
			    actual_jumpV(ilist[i], nltcp[i], jatom[i]);
                NF_Nj --;                       // (no flickering)
                NF_rates= 0;                    // (no flickering)
                list_nf.clear();                // (no flickering)
            
                return 1.0/sum_rates;           // (no flickering)
            }                                   // (no flickering)
            
            acc_nf += rates[i]/NF_rates;        // (no flickering)
        }                                       // (no flickering)
    }                                           // (no flickering)
    else{
	    double ran= ran_generator();
        
        if(ran < crates/sum_rates){
            double acc_cr= 0;
            for(auto it= cvcc.begin(); it != cvcc.end(); it ++){
                for(int a=0; a< (it->second).rates.size(); a ++){ // access creation paths of the atom
                    double rate_a= (it->second).rates[a]; if(is_noflckr) rate_a *= NFratio;  // if no flickering, multiply by the NFratio

                    if( (ran >= acc_cr) && (ran < (acc_cr + rate_a/sum_rates) ) ){
                        create_vcc(it->first, (it->second).mltcp[a]); 
                            
                        return 1.0/sum_rates;
                    }
                        
                    acc_cr += rate_a/sum_rates;
                }
            }
        }
        else{
	        double acc_rate= crates/sum_rates; // accumulated rate
	        for(int i=0; i<rates.size(); i ++){
                if( (ran >= acc_rate) && (ran < (acc_rate + rates[i]/sum_rates) ) ){			
		            switch(etype[i]){
                        case 0:  actual_jumpI(ilist[i], nltcp[i], jatom[i]); break;
			            case 1:  actual_jumpV(ilist[i], nltcp[i], jatom[i]); break;
                        case 7:  genr(); N_genr ++; break;
                        default: error(2, "(main) an unknown event type", 1, etype[i]);
                    }

	                return 1.0/sum_rates;
                }
		    
                acc_rate += rates[i]/sum_rates;
            }
        }
	}
    
    error(2, "(main) can't match any event :(");
}

void class_events::actual_jumpV(int vid, int nltcp, int jatom){ // vcc id, neighbor ltcp and jumping atom
    if(jatom==1) Vja[0] ++; // track # of jumping atoms (see log file) 
	else         Vja[1] ++;
	
    int xv= (int) (list_vcc[vid].ltcp/nz)/ny; // vcc position
	int yv= (int) (list_vcc[vid].ltcp/nz)%ny;
	int zv= (int)  list_vcc[vid].ltcp%nz;
    if(states[xv][yv][zv] != 0 || itlAB[xv][yv][zv]) error(2, "(actual_jumpV) the jumping vcc is not a vcc", 1, xv*ny*nz+yv*nz+zv);

	int x= (int) (nltcp/nz)/ny; // neighbor position
	int y= (int) (nltcp/nz)%ny;
	int z= (int)  nltcp%nz;
    if(states[x][y][z] != 1 && states[x][y][z] != -1) error(2, "(actual_jumpV) the jumping atom is not an atom", 1, x*ny*nz+y*nz+z);

	states[xv][yv][zv]= states[x][y][z];
	
    if(srf[x][y][z]){ // if jump into srf atom, becomes vacuum
        nV --;
        nM ++;

        srf[x][y][z]= false;
        states[x][y][z]= 4;
        
        if     (list_vcc[vid].njump < 0) {}
        else if(list_vcc[vid].njump > 9) njump[9] ++;
        else                             njump[list_vcc[vid].njump] ++;
        
        list_vcc.erase(list_vcc.begin()+vid);
    
    }
    else{
        states[x][y][z]= 0;
    	list_vcc[vid].ltcp= x*ny*nz + y*nz + z;
    	if((x-xv)>nx/2) list_vcc[vid].ix --; if((x-xv)<-nx/2) list_vcc[vid].ix ++;
    	if((y-yv)>ny/2) list_vcc[vid].iy --; if((y-yv)<-ny/2) list_vcc[vid].iy ++;
    	if((z-zv)>nz/2) list_vcc[vid].iz --; if((z-zv)<-nz/2) list_vcc[vid].iz ++;
    
        if(list_vcc[vid].njump != -1) list_vcc[vid].njump ++;
    }
    
    srf_check(nltcp);
    srf_check(xv*ny*nz + yv*nz + zv);
    cvcc_rates += update_ratesC(xv*ny*nz+ yv*nz+ zv);
    cvcc_rates += update_ratesC(x *ny*nz+ y *nz+ z);
    for(int a=0; a<list_update.size(); a ++) cvcc_rates += update_ratesC(list_update[a]);
    list_update.clear();
}

void class_events::actual_jumpI(int iid, int nltcp, int jatom){
    int xi= (int) (list_itl[iid].ltcp/nz)/ny; // itl position
	int yi= (int) (list_itl[iid].ltcp/nz)%ny;
	int zi= (int)  list_itl[iid].ltcp%nz;
	if(! (-2==states[xi][yi][zi] || (0==states[xi][yi][zi] && itlAB[xi][yi][zi]) || 2==states[xi][yi][zi]))
        error(2, "(actual_jumpI) the jumping itl is not an itl", 1, xi*ny*nz+yi*nz+zi);

	int x= (int) (nltcp/nz)/ny; // neighbor position
	int y= (int) (nltcp/nz)%ny;
	int z= (int)  nltcp%nz;
	
    if(4==states[x][y][z]){
        if(! is_inf) error(2, "(actual_jumpI) an itl to srf jump isnt instant event");        
        rules_recb(true,  iid, nltcp, jatom);
    }
    else if(0==states[x][y][z]){
        for(int i= 0; i<list_vcc.size(); i ++){ // brutal search for the vcc in the list
    		if(nltcp==list_vcc[i].ltcp){
                rules_recb(false, iid, i, jatom);
                break;
            }
        }
    }
    else{
    	if(jatom==1) Ija[0] ++;
    	else         Ija[1] ++;
	    
        switch(states[xi][yi][zi]){ // update numbers before jump
		    case  2: nAA --; break;
    		case  0: nAB --; itlAB[xi][yi][zi]= false; break;
    		case -2: nBB --; break;
    		default: error(2, "(jump) could not find the Itl type in --", 1, states[xi][yi][zi]);
    	}
    	switch(states[x][y][z]){
    		case  1: nA --;  break;
    		case -1: nB --;  break;
    		default: error(2, "(jump) could not find the Atom type in --", 1, states[x][y][z]);
    	}

	    states[xi][yi][zi] -= jatom; // jumping
	    states[x][y][z]    += jatom;
	
	    switch(states[x][y][z]){ // update numbers after jump
		    case  2: nAA ++; break;
		    case  0: nAB ++; itlAB[x][y][z]= true; break;
		    case -2: nBB ++; break;
		    default: error(2, "(jump) could not find the Itl type in ++", 1, states[x][y][z]);
	    }
	    switch(states[xi][yi][zi]){
		    case  1: nA ++;  break;
		    case -1: nB ++;  break;
		    default: error(2, "(jump) could not find the Atom type in ++", 1, states[xi][yi][zi]);
	    }
	
	    list_itl[iid].ltcp= x*ny*nz + y*nz + z;
//	    list_itl[iid].dir=  nid;
//	    list_itl[iid].head= states[x][y][z] - jatom;
	
	    if((x-xi)>nx/2) list_itl[iid].ix --; if((x-xi)<-nx/2) list_itl[iid].ix ++;
	    if((y-yi)>ny/2) list_itl[iid].iy --; if((y-yi)<-ny/2) list_itl[iid].iy ++;
	    if((z-zi)>nz/2) list_itl[iid].iz --; if((z-zi)<-nz/2) list_itl[iid].iz ++;
    }
    
    srf_check(nltcp);
    srf_check(xi*ny*nz + yi*nz + zi);
    cvcc_rates += update_ratesC(xi*ny*nz+ yi*nz+ zi);
    cvcc_rates += update_ratesC(x *ny*nz+ y *nz+ z);
    for(int a=0; a<list_update.size(); a ++) cvcc_rates += update_ratesC(list_update[a]);
    list_update.clear();
}
	
void class_events::create_vcc(int altcp, int mltcp){
	int ja= *(&states[0][0][0]+altcp);
    if(ja != 1 && ja != -1) error(2, "(create_vcc) an ja isnt 1 or -1", 1, ja);

    // initialize the vcc in the list_vcc
	int vid= list_vcc.size();
	list_vcc.push_back(vcc());
	
	list_vcc[vid].ltcp= altcp;
	list_vcc[vid].ix= 0;
	list_vcc[vid].iy= 0;
	list_vcc[vid].iz= 0;
    list_vcc[vid].njump= 0;

	// Update states
	*(&states[0][0][0]+mltcp)= ja;
	*(&states[0][0][0]+altcp)= 0;
       *(&srf[0][0][0]+mltcp)= true;
       *(&srf[0][0][0]+altcp)= false;

    nV ++;
    nM --;

    srf_check(altcp);
    srf_check(mltcp);
    cvcc_rates += update_ratesC(altcp);
    cvcc_rates += update_ratesC(mltcp);
    for(int a=0; a<list_update.size(); a ++) cvcc_rates += update_ratesC(list_update[a]);
    list_update.clear();

    if(is_noflckr){
        NF_id= vid;         // vcc id for staight jumps (no flickering)
        NF_Nj= N_NFjumps;   // N of jumps remained (no flickering)
    }
}

// functions in backupfun:
//	int vpos[3];
//	events.vac_jump_random(par_pr_vjump, vpos);
//	events.vac_recb(vpos);
//	events.int_motions();

