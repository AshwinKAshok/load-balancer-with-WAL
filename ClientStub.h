#include "ClientSock.h"
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <list>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>


class ClientStub{
    
    int connected_sock;
    ClientSocket newclient;
    
    public:

        /*******
         * initializes the client stub and establishes a new TCP connection to the server. 
        */
        int Init(std::string ip, int port){
            connected_sock =newclient.Init(ip, port);
            if (connected_sock<0){
                return -1;
            }
            return 0;
        }


        /*******
         * sends an order to the server and receives the robot (or robot information) from the server
         * Once the function is called the order details should be marshalled into a byte stream, which
         * is then sent through a socket connection to the server and then wait for a server's response.
         * Once the server responds back with robot information in a byte stream format, the byte
         * stream is unmarshalled to robot information and returned.
         * */
        int Order(order_inf new_order, robot_inf * get_robo){
            char *sending_data = new char[sizeof(new_order)];
            memcpy((void*)sending_data, (void*)&new_order, sizeof(new_order));
            if (check((send(connected_sock,sending_data, sizeof(new_order), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            };

            char *receiving_data = new char[sizeof(*get_robo)];
            if (check((recv(connected_sock, receiving_data, sizeof(*get_robo), 0)), "ERROR reading from socket") <0){
                return -1;
            };

            memcpy(get_robo, receiving_data, sizeof(*get_robo));
            //std::cout<<"sent and recieved order"<<std::endl;

            return 0;
        }


        /**********
         * This sends the information to server about it being a customer. This will
         * make the server treat all the request comming from this system as a customer request.
         */
        int set_conn_type(){
            conn_type repl_conn;
            repl_conn.is_customer=1;
            char *sending_conn_type = new char[sizeof(repl_conn)];
            memcpy((void*)sending_conn_type, (void*)&repl_conn, sizeof(repl_conn));
            if (check((send(connected_sock, sending_conn_type,sizeof(repl_conn), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            };
            return 0;
        }


        /*******
         * This sends a read request to the server and gets the response from the server.
         */
        int ReadRecord(order_inf new_order, c_rec* get_rec){
            char *sending_data = new char[sizeof(new_order)];
            memcpy((void*)sending_data, (void*)&new_order, sizeof(new_order));
            if (check((send(connected_sock,sending_data, sizeof(new_order), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            };

            char *receiving_data = new char[sizeof(*get_rec)];
            if (check((recv(connected_sock, receiving_data, sizeof(*get_rec), 0)), "ERROR reading from socket") <0){
                return -1;
            };
            memcpy(get_rec, receiving_data, sizeof(*get_rec));

            return 0;
        }

        /*******
         * This closes the socket with the server once all the order from the customer is successfully recieved.
         * */
        void close_socket(){
            close(connected_sock);
        }



        /*********
         * error handling code. Does not exit from program if error occurs. throws error 
         * and continues with the operation. This returns -1 if error occurs and it is upto 
         * the calling function to handle it accordingly.
         */
        int check(int check_code, const char *error_str){
            if (check_code < 0){
                //std::cout<<"Encountered ERROR...."<<std::endl;
                //perror(error_str);
                //exit(1);
                return -1;
            }
            return 0;
        }
};

