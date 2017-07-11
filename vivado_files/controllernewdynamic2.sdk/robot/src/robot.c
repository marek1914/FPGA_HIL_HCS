/************* FOR THE FPGA UDP RECIEVE CODE *******************/
/*
* This is not a effecient way code and will recommend using the other code ROBOT 2
* Here robot pings everytime and gets the latest control inputs from the FPGA 
* Time calculations are done on the Robot
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <arpa/inet.h>
#include "ourheader.h"
#define sdim 3
#define udim 2
 // Manipulaltion variable for state and input space
  int smask[sdim]={6,6,6};
  int umask[udim]={3,3};
  int valuesmask[3]={pow(2,6)-1,pow(2,6)-1,pow(2,6)-1};
  int valueinmask[2]={pow(2,3)-1,pow(2,3)-1};
  float lbs[sdim]={450,600,-3.4};
  float lbc[udim]={-240,-240};
  float etas[sdim]={30,30,0.2};
  float etac[sdim]={120,120};



int main(){
int udpSocket1,udpSocket2,nBytes1,nBytes2;
  float buffer1[3]={1,2,3};//to recive data from Broadcaster
  float buffer2[2]={1,2};//to send data to Robot
  char pingbuffer[2];
  int index;
  char num[524287];
  int* w = getpointer(0);
  int* r = getpointer(4);
  int testcount=0;
  struct sockaddr_in FPGAAddr1,FPGAAddr2,clientAddr;
  struct sockaddr_storage serverStorage1,serverStorage2;
  socklen_t addr_size1,addr_size2, client_addr_size;

  /*Create UDP socket*/
  udpSocket1 = socket(PF_INET, SOCK_DGRAM, 0);//To recive data from Broadcaster
  if(udpSocket1 > 0)
      printf("UDP SOCKET 1 created\n");
  udpSocket2 = socket(PF_INET, SOCK_DGRAM, 0);//To send data to Robot
  if(udpSocket2 > 0)
	  printf("UDP SOCKET 2 created\n");


  //Make udpsocket2 non-blocking
  int flags = fcntl(udpSocket2, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(udpSocket2, F_SETFL, flags);
  /*Configure settings in address struct to recieve data from Broadcaster*/
 FPGAAddr1.sin_family = AF_INET;
 FPGAAddr1.sin_port = htons(9003);
 FPGAAddr1.sin_addr.s_addr = inet_addr("192.168.0.105");
 memset(FPGAAddr1.sin_zero, '\0', sizeof FPGAAddr1.sin_zero);
  /*Configure settings in address struct to send data to Robot*/
  FPGAAddr2.sin_family = AF_INET;
  FPGAAddr2.sin_port = htons(7809);
  FPGAAddr2.sin_addr.s_addr = inet_addr("192.168.0.105");
  memset(FPGAAddr2.sin_zero, '\0', sizeof FPGAAddr2.sin_zero);
  /*Bind socket 1 with address struct */
 bind(udpSocket1, (struct sockaddr *) &FPGAAddr1, sizeof(FPGAAddr1));
  /*Bind socket 2 with address struct */
 bind(udpSocket2, (struct sockaddr *) &FPGAAddr2, sizeof(FPGAAddr2));

  /*Initialize size variable to be used later on*/
  addr_size1 = sizeof serverStorage1;
  addr_size2 = sizeof serverStorage2;

  while(1){
           /* Try to receive any incoming UDP datagram. Address and port of requesting client will be stored on serverStorage variable */
          nBytes1= recvfrom(udpSocket1,buffer1,sizeof(buffer1),0,(struct sockaddr *)&serverStorage1, &addr_size1);

                  buffer1[1]= 1900-(buffer1[1]-580);//Match the new cartesian
                  buffer1[2]=-buffer1[2]; // Angle was flipped
                  printf("current state :  %f | %f | %f \n",buffer1[0],buffer1[1],buffer1[2]);
	      //Check if Robot requires data
          if(recvfrom(udpSocket2,pingbuffer,sizeof(pingbuffer),0,(struct sockaddr *)&serverStorage2, &addr_size2)>0){

        	             printf("Data Required\n");  //Data is required
	                     index=statetoindexS(buffer1);
     		             writeme(w,index);
                         usleep(1);
			             indextostateC(readme(r),buffer2);

			          if( (450<=buffer1[0] && buffer1[0]<=590 && 600<=buffer1[1] && buffer1[1]<=840 )|| buffer1[0]==-1 || buffer1[1]==1900-(-1-580) || buffer1[2]== -1 )
			           {
			        	printf("I am in Target or out of state space\n ");
			           	buffer2[0]=0.00;
			          	buffer2[1]=0.00;
			          }

                        //send control inputs corresponding to latest x y and theta
                         sendto(udpSocket2,buffer2,sizeof(buffer2),0,(struct sockaddr *)&serverStorage2,addr_size2);//Send control inputs to Robot
                         printf("Control Input Sent\n");
                         printf("Inputs :   %f | %f \n",buffer2[0],buffer2[1]);
	}

        else{
                         printf("Control NOT Required*******************************************************************************\n");
        }
  }

  return 0;
}

/**************************************************************************/

//Convert state space value into index
int statetoindexS(float state[]){
   		int id[sdim],index=0,i=0;
		for(i ;i<sdim;i++)
			id[i]=round(((state[i]-lbs[i])/etas[i]));
                index=id[sdim-1];
 		index=(index<<smask[2]);
                index=index|id[1];
                index=(index<<smask[1]);
                index=index|id[0];
return index;
}

//convert control input index into real control inputs
void indextostateC(int index, float state[]){
     					  state[0]=0;
     					  state[1]=0;
      					  state[0]= index & valueinmask[0];
      				      state[0]=lbc[0]+etac[0]*state[0];
      				      index=(index >> umask[0]);
      					  state[1]= index & valueinmask[1];
      				      state[1]=lbc[1]+etac[1]*state[1];
}

