/************* FOR THE FPGA UDP RECIEVE CODE *******************/
/*
 * author: Abhinav Himanshu
 * email:  abhinavhimanshu6@gmail.com
 * date:   5,July,2017
 *
 * Description ::  FPGA has determinized controller in the form of IP which takes state input in binary bits(BDD Variables).
 *               > Make sure state and input space is less than 32 bit or make changes in vivado synthesis and edit the IP core
 *                 FPGA receives x y and theta from broadcaster from non blocking receive
 *                 FPGA waits for the ping from Robot
 *                 Once a ping is received
 *                 It enters in a loop
 *                 Where it sends control inputs to ROBOT periodically
 *                 =================================================================
 *                 1) Turn on the Broadcaster ; 2) Then run the FPGA ; 3) Run the code on Robot
 */

/* Include files */
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <time.h>
#include "ourheader.h"

//State Parameters
#define sdim 3
#define udim 2

// Variable for state and input space
  int smask[sdim]={6,6,6};         //No. of BDD Variable in each dimension for state space
  int umask[udim]={3,3};           //No. of BDD Variable for each dimension in input space
  int valuesmask[3]={0,0,0};       //
  int valueinmask[2]={pow(2,3)-1,pow(2,3)-1}; // Mask variables for input space
  float lbs[sdim]={450,600,-3.4}; // lower bounds after SCOTS Quantization .. See in Controller.bdd file  and take first coordinate of each step
  float lbc[udim]={-240,-240};    // upper bounds Take from anywhere does not matter
  float etas[sdim]={30,30,0.2};   //eta values for STATE SPACE as specified in SCOTS FILE
  float etac[sdim]={120,120};     //eta values as INPUT SPACE specified in SCOTS FILE
  float tau= 0.7;
/*
  udpSocket1 takes data from Image Processing server(BroadCaster).
  Make sure it is binded to correct IP Address and Port NUM same IP should also be registered in "IPAddresses.network.txt" file in at BroadCaster.
  clientSocket is used to send data to the Robot periodically here in tau seconds.

*/

int main(){

  int udpSocket1,udpSocket2;//Sockets to recieve and send data
  float buffer1[3]={1,2,10};//to receive data from Broadcaster
  float buffer2[2]={0,0};   //to send data to Robot | some garbage value initially
  char ping='c';            //To store ping from Robot
  int index;               // used in real space to index conversion
  int* w = getpointer(0);  //Get pointer to INPUT Address of Controller IP CORE
  int* r = getpointer(4);  //Get pointer to OUTPUT Address of Controller IP CORE
  //int testcount=0;         //Debug variable to increment to make sure sync in Socket based communication
  struct sockaddr_in FPGAAddr1,FPGAAddr2;// Addrress to send and receive data.
  struct sockaddr_storage serverStorage2; // Only required if interested to get socket details of Sender
  socklen_t addr_size2; //// Only required if interested to get socket details of Sender
  initMask();

  clock_t start,stop; // Measure time on FPGA


  /*Create UDP socket*/
  udpSocket1 = socket(PF_INET, SOCK_DGRAM, 0);//To receive data from Broadcaster
  if(udpSocket1 > 0)
    printf("UDP SOCKET 1 created\n");
  else{
	  printf("Problem creating the Socket\n");
	  return udpSocket1;
  }    /*Create UDP socket*/
  udpSocket2 = socket(PF_INET, SOCK_DGRAM, 0);//To receive data from Broadcaster
  if(udpSocket2 > 0)
    printf("UDP SOCKET 2 created\n");
  else{
	  printf("Problem creating the Socket\n");
	  return udpSocket2;
  }



// It is necessary to get recvfrom NONBLOCKING | This makes udpSocket1 NonBlocking
     long save_fd = fcntl( udpSocket1, F_GETFL );
     save_fd |= O_NONBLOCK;
     fcntl(udpSocket1, F_SETFL, save_fd );
//

  /*Configure settings in address struct to receive data from Broadcaster*/
     FPGAAddr1.sin_family = AF_INET;
     FPGAAddr1.sin_port = htons(9003); // Put Port Num According to Image Processing Server (Broadcaster)
     FPGAAddr1.sin_addr.s_addr = inet_addr("192.168.0.105");//Put IP Address of FPGA
     memset(FPGAAddr1.sin_zero, '\0', sizeof FPGAAddr1.sin_zero);

     FPGAAddr2.sin_family = AF_INET;
     FPGAAddr2.sin_port = htons(7819);
     FPGAAddr2.sin_addr.s_addr = inet_addr("192.168.0.105");
     memset(FPGAAddr2.sin_zero, '\0', sizeof FPGAAddr2.sin_zero);

 // Open clientSocket to receive data from
 //    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);// Socket to send control inputs to Robot


     /*Bind socket 1 with address struct */
     bind(udpSocket1, (struct sockaddr *) &FPGAAddr1, sizeof(FPGAAddr1)); //Bind to Socket to given address
     /*Bind socket 2 with address struct */
     bind(udpSocket2, (struct sockaddr *) &FPGAAddr2, sizeof(FPGAAddr2));


     start=0;

     /*Wait for robot to ping */
     recvfrom(udpSocket2,ping,sizeof(ping),0,(struct sockaddr *)&serverStorage2, &addr_size2);



  while(1){

        /* Try to receive latest if any incoming UDP datagram. Address and port of requesting client will be stored on serverStorage variable */
	        int tmp=0;
	        tmp= recvfrom(udpSocket1,buffer1,sizeof(buffer1),0,0,0);

	        while(tmp>11){
		     tmp= recvfrom(udpSocket1,buffer1,sizeof(buffer1),0,0,0);
	        }

            buffer1[1]= 1900-(buffer1[1]-580);//to match the new cartesian
	        buffer1[2]=-buffer1[2]; // Angle was flipped
            printf("Current state is  %f | %f | %f \n",buffer1[0],buffer1[1],buffer1[2]);

           if(buffer1[0]!=-1 || buffer1[2]!=-10) //Something received from broadcaster
           {
/**********************************************************************************************************************/
	   /* |Real State| -> |Index| -> [ CONTROLLER ON FPGA] -> |Index| -> |Real Control Input| */
	                    index=statetoindexS(buffer1);    //Get index of current state space
                            writeme(w,index); 		     //Write index on Controller IP CORE input
                            usleep(1);                       //Read At
			    indextostateC(readme(r),buffer2);//Read At Controller IP CORE output


/**********************************************************************************************************************/
         // Stop if in target or Tracker does-not detect it
		 if( (450<=buffer1[0] && buffer1[0]<=590 && 600<=buffer1[1] && buffer1[1]<= 840 ) || buffer1[0]==-1 || buffer1[1]== -1 || buffer1[2]== -1 )
			{
			   printf("Either Target reached or not detected\n");
			   	buffer2[0]=0.00;
			    buffer2[1]=0.00;
			}
		    printf(" Input for the state is %f   |    %f \n",buffer2[0],buffer2[1]);
         //send control inputs corresponding to latest X Y and theta
                sendto(udpSocket2,buffer2,sizeof(buffer2),0,(struct sockaddr *)&serverStorage2,addr_size2);//Send control inputs to Robot
                 stop = clock();
         //wait untill tau seconds | Take time less than tau to be safe
		while ( ( ((float)( stop - start)*100000)/ CLOCKS_PER_SEC) < tau*100000) {
			           		stop = clock();
            }

	        start = clock();

            }

           //else keep on listening for broadcaster

        }


  return 0;

}

/**************************************************************************/

//Sets the value of valueMask variables
void initMask(){
				for(int i=0;i<sdim-1;i++){
					     valuesmask[i]=pow(2,smask[i])-1;
 			             }
				for(int i=0;i<udim-1;i++){
					     valueinmask[i]=pow(2,umask[i])-1;
				}
}


//Convert state space value to index
int statetoindexS(float state[]){
   		int id[sdim],index=0,i=0;
		for(i=0 ;i<sdim;i++)
			id[i]=round(((state[i]-lbs[i])/etas[i]));

		for(int i= sdim-1;i>0;i--){
      	                       index=index|id[i];
     	                       index=(index<<smask[i]);
        }
		index=index|id[0];
 return index;
}


//convert control input index to real control inputs
void indextostateC(int index, float state[]){

        for(int i=0;i<udim-1;i++){
             	state[i]=0;
               	state[i]= index & valueinmask[i];
               	state[i]=lbc[i]+etac[i]*state[i];
              	index=(index >> umask[i]);
           }

        state[udim-1]=0;
        state[udim-1]= index & valueinmask[udim-1];
        state[udim-1]=lbc[udim-1]+etac[udim-1]*state[udim-1];
}


