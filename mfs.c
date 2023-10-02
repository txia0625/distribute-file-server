#include "mfs.h"
#include <time.h>
#include <stdlib.h>





struct sockaddr_in senderAddr, serverAddr;
int sd;
int closesd;
fd_set fs;
int port_num;

struct timeval tv;

int MFS_Init(char *hostname, int port)
{
    
    printf("CLIENT :: INIT\n");
    int MIN_PORT = 20000;
    int MAX_PORT = 40000;

    srand(time(0));
     port_num = (rand() % (MAX_PORT - MIN_PORT) + MIN_PORT);

    sd = UDP_Open(port_num);

    while(sd == -1)
    {
        port_num = (rand() % (MAX_PORT - MIN_PORT) + MIN_PORT);
        sd = UDP_Open(port_num);
    }
    closesd = sd;
    
    int receive = UDP_FillSockAddr(&serverAddr, hostname, port);
    tv.tv_sec = 3;
    tv.tv_usec = 0;
/*
    FD_ZERO(&fs);
    FD_SET(sd, &fs);
  
    msg_t message;
    message.msgType = 0;
    printf("CLIENT :: Connecting to the server...\n");
 */ 
    while(receive < 0)
    {
        receive = UDP_FillSockAddr(&serverAddr, hostname, port);
    }



    //receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));

   /* while(receive < 0 )
    {
        receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));
    }



    msg_t rcMsg;
    //printf("sent for the first time \n");

    while(1)
    {
        fd_set temp = fs;

        int val = select(sd+1, &temp, NULL, NULL,&tv);

        if(val == 0)
        {
            printf("INIT Again\n");
            tv.tv_sec = 3;
            UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));

        }else if(val < 0) return -1;
        else break;
    }

*/
/*
    printf("CLIENT :: Waiting for the server to respond...\n");
    receive = UDP_Read(sd, &senderAddr, &rcMsg, sizeof(msg_t));

    if(receive < 0)
    {
        printf("CLIENT :: receive MSG FAILED");
        return -1;
    }


    if(rcMsg.inumber != 666)
    {
        perror("Server Connection FAILED\n");
       // perror("receiveD RESULT %d\n", rcMsg.inumber);
        return -1;
    } else printf("CLIENT :: Successfully Connected to the Server\n");
*/
    return 0;

}

int MFS_Lookup(int pinum, char *name)
{
    printf("CLIENT :: LOOKUP\n");
    msg_t message;
    msg_t rcMsg;
    message.msgType = 1;
    message.inumber = pinum;


    
    if(name == NULL || pinum <0) return -1;
    int i = 0;
    while(name[i] != '\0')
    {
        message.buffer[i] = name[i];
        i++;
    }//copy name into the message buffer

    message.buffer[i] = '\0';
    message.inumber = pinum;

    int receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));
    if(receive < 0)
    {
        perror("CLIENT :: SENDING MSG FAILED");
        return -1;
    }



    receive = UDP_Read(sd, &senderAddr, &rcMsg, sizeof(msg_t));
    if(receive < 0)
    {
        perror("CLIENT :: Receiving MSG FAILED");
        return -1;
    }

    

    return rcMsg.inumber;

}
int MFS_Stat(int inum, MFS_Stat_t *m)
{
    printf("CLIENT :: STAT\n");
    msg_t message;
    msg_t rcMsg;
    message.msgType = 2;
    

    
    if(inum < 0 || m == NULL) return -1;
    message.inumber = inum;

    int receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));
    if(receive < 0)
    {
        perror("CLIENT :: SENDING MSG FAILED");
        return -1;
    }



    receive = UDP_Read(sd, &senderAddr, &rcMsg, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: Receiving MSG FAILED");
        return -1;
    }
    if(rcMsg.inumber == -1) return -1;

    MFS_Stat_t* temp = (MFS_Stat_t*)rcMsg.buffer;
    m->size = temp->size;
    m->type = temp->type;
    return rcMsg.inumber;

}
int MFS_Write(int inum, char *buffer, int offset, int nbytes)
{
    printf("CLIENT :: WRITE\n");
    msg_t message;
    msg_t rcMsg;
    message.msgType = 3;

    
    if( buffer == NULL ||  nbytes <0 || nbytes > 4096 ||inum < 0) return -1;
    message.inumber = inum;
    message.offset = offset;
    message.bytes = nbytes;
    memcpy(message.buffer,buffer, nbytes);

    int receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: SENDING MSG FAILED");
        return -1;
    }





    receive = UDP_Read(sd, &senderAddr, &rcMsg, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: Receiving MSG FAILED");
        return -1;
    }
    

    return rcMsg.inumber;
}
int MFS_Read(int inum, char *buffer, int offset, int nbytes)
{
    printf("CLIENT :: READ\n");
    msg_t message;
    msg_t rcMsg;
    message.msgType = 4;

    
    if( buffer == NULL  ||  nbytes <=0 || nbytes > 4096 ||inum < 0 ) return -1;
    message.inumber = inum;
    message.offset = offset;
    message.bytes = nbytes;
    

    int receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: SENDING MSG FAILED");
        return -1;
    }



    receive = UDP_Read(sd, &senderAddr, &rcMsg, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: Receiving MSG FAILED");
        return -1;
    }
    

    if(rcMsg.inumber == -1) return -1;
    memcpy(buffer, rcMsg.buffer, nbytes);
    return 0;
}
int MFS_Creat(int pinum, int type, char *name)
{
    printf("CLIENT :: CREAT\n");
    msg_t message;
    msg_t rcMsg;
    message.msgType = 5;

    
    if(pinum < 0  || name== NULL ) return -1;
    
    if(type != 0 && type != 1) return -1;
    

    message.inumber = pinum;
    message.fileType = type;
    
    if(strlen(name) > 28) return -1;


    strcpy(message.buffer, name);

    int receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: SENDING MSG FAILED");
        return -1;
    }else printf("CLIENT :: SENT CREATE MSG");



    receive = UDP_Read(sd, &senderAddr, &rcMsg, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: Receiving MSG FAILED");
        return -1;
    }
    

    return rcMsg.inumber;
}


int MFS_Unlink(int pinum, char *name)
{
    printf("CLIENT :: UNLINK\n");
    msg_t message;
    msg_t rcMsg;
    message.msgType = 6;

    
    if(pinum < 0 ||  name == NULL ) return -1;

    message.inumber = pinum;
    if(strlen(name) > 28) return -1;

    strcpy(message.buffer, name);

    int receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: SENDING MSG FAILED");
        return -1;
    }



    receive = UDP_Read(sd, &senderAddr, &rcMsg, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: Receiving MSG FAILED");
        return -1;
    }
    

    return rcMsg.inumber;
}


int MFS_Shutdown()
{
    printf("CLIENT :: SHUTDOWN\n");
    msg_t message;
    msg_t rcMsg;
    message.msgType = 7;

    int receive =  UDP_Write(sd, &serverAddr, &message, sizeof(msg_t));
    if(receive < 0)
    {
        printf("CLIENT :: SENDING MSG FAILED");
        return -1;
    }




   // receive = UDP_Read(sd, &senderAddr, &rcMsg, sizeof(msg_t));
    //if(receive < 0)
    //{
      //  printf("CLIENT :: Receiving MSG FAILED");
       // return -1;
    //}

    UDP_Close(closesd);
    

    return 0;

}

