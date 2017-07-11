/*
 * ourheader.h
 *
 *  Created on: Jun 8, 2017
 *      Author: fmlab3
 */
#ifndef OURHEADER_H_
#define OURHEADER_H_


/***************************** Include Files *******************************/
#include <stdint.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

#define BDD_PHY_ADDRESS 0x43C00000 //Intial the Base Address of your IP CORE * depends on upon Vivado synthesis process
#define BDD_INP_OFFSET 0           //Offest for INPUT Feed ... slvReg0 * depends on upon IP core Vivado synthesis process
#define BDD_OUT_OFFSET 4           //Offest for OUTPUT Feed ... slvReg1 * depends on upon IP core Vivado synthesis process

// You need to be in root to run this !! * But we are always in root Petalinux zynq core
#define MAP_SIZE 65536UL		// our 4K range of the AXI bdd * depends on upon Vivado synthesis process
#define MAP_MASK (MAP_SIZE-1)



//Returns pointers of any physical address
//Try  not touching this
void* get_v_addr(int phys_addr){
	void* mapped_base;
	int memfd;

	void* mapped_dev_base;
	off_t dev_base = phys_addr;

	memfd = open("/dev/mem", O_RDWR | O_SYNC);
	if(memfd == -1){
		printf("Cant open /dev/mem.\n");
		exit(0);
	}

	mapped_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, dev_base & ~MAP_MASK);
	if(mapped_base == (void*)-1){
		printf("Cant map the physial address to user-space !.\n");
		exit(0);
	}

	mapped_dev_base = mapped_base + (dev_base & MAP_MASK);
	return mapped_dev_base;
}


//Test rough code for debugging
/*
void check(){
	    int* last = get_v_addr(BDD_PHY_ADDRESS);

		for(int i=0;i<=4;i++){
	    	//int* bdd_addr = get_v_addr((int)(BDD_PHY_ADDRESS)+i);
	    	//int diff=bdd_addr-last;
	    	printf("BASE ADDRESS IS %d=====  %d >>>>> DIFF is ==== %d \n",i,get_v_addr((int)(BDD_PHY_ADDRESS)+i)-);
	    	last=bdd_addr;
	    }
}
*/

//Get Base Address not needed
void* getbaseaddrs()
{

	int* baseaddr = get_v_addr(BDD_PHY_ADDRESS);
	// int* bdd_inp = bdd_addr + BDD_INP_OFFSET;
	//int* bdd_out = bdd_addr + BDD_OUT_OFFSET;
	//*bdd_inp  = state;
	//int out = *bdd_out;
    return baseaddr;

}

//Get pointer to the offset address
void* getpointer(int offset)
{

	int* baseaddr = get_v_addr(BDD_PHY_ADDRESS+offset);
	// int* bdd_inp = bdd_addr + BDD_INP_OFFSET;
	//int* bdd_out = bdd_addr + BDD_OUT_OFFSET;
	//*bdd_inp  = state;
	//int out = *bdd_out;
	return baseaddr;
}

int getaddrsdiff(){
					int  addrsdiff  = get_v_addr((int)(BDD_PHY_ADDRESS)+1)-get_v_addr(BDD_PHY_ADDRESS);
					return addrsdiff;
}

//Reads the value at p address
int readme(int* p){
                    return *p;
}

//Writes the value v at p address
void writeme(int *p , int v){
	*p = v;
}
#endif /* OURHEADER_H_ */
