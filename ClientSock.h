#include <stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <string.h>
#include <string>
#include <unistd.h>
#include "RobotFactory.h"
#include <iostream>

class ClientSocket{

    const char *server_addr_str;
    int port_addr;
    public:

        /**********
         * This function accepts server IP address and port address and creates a TCP connection 
         * with the server and return the socket to send and receieve data
        */
        int Init(std::string server_ad, int port_addr_in){
            server_addr_str = server_ad.c_str();
            port_addr = port_addr_in;
            int sockfd;
            struct sockaddr_in serv_addr;
            
            if(check((sockfd = socket(AF_INET, SOCK_STREAM, 0)),"ERROR opening socket")<0){
                return -1;
            }
            bzero((char *) &serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port_addr);
            if(check((inet_pton(AF_INET, server_addr_str, &serv_addr.sin_addr)), "\nInvalid address/ Address not supported \n")<0){
                return -1;
            } 
            if(check((connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))),"ERROR connecting")<0){
                return -1;
            }
            return sockfd;
        }

        /*********
         * error handling code. throws error and exit from program.
         */
        int check(int check_code, const char *error_str){
            if (check_code < 0){
                //perror(error_str);
                //exit(1);
                return -1;
            }
            return 0;
        }

        /******
         *  ----currently not being used------
         * This is generic function to send and receieve data from the connected server.
         * ------untested code------
        */
        void send_data(int conn_sock, void *byte_stream, void *incomming_bytes){
            check((write(conn_sock,byte_stream, sizeof(byte_stream))), "ERROR writing to socket");
            check((read(conn_sock,incomming_bytes,sizeof(incomming_bytes))),"ERROR reading from socket"); 
            return;
        }

};
