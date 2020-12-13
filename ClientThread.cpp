#include<stdio.h>
#include<iostream>
#include<arpa/inet.h>
#include<string.h>
#include<thread>
#include<vector>
#include<cstdlib>
#include<chrono>
#include<algorithm>
#include<numeric>
#include <iomanip>
#include "ClientThread.h"


ClientThreadClass::ClientThreadClass() {}


/******
         * This is the customer function which is spawned as a child thread by the main thread. Each customer has its 
         * own thread and unique customer id associated with it which creates a new socket and initiates connection to 
         * the server with iand process all the orders associated with the secific customer. Once all the robot 
         * information is recieved for the particular customer the thread closes itself and joins the main thread.
        */
void ClientThreadClass::Client_thread_body(int custid, const char *server_addr_str, int port_addr, int orders, int request_type){
    if (client_stub.Init(server_addr_str,port_addr) <0){
        std::cout<<"Error: Issue with the server"<<std::endl;
    }else{
        order_inf send_order;
        if (request_type == 1){ 
            robot_inf get_robo;
            int order_id = 1;
            send_order.customer_id = custid;
            send_order.order_number =  order_id;
            send_order.request_type = request_type;
            for (int i=0; i<orders; i++){          
                timer.Start();
                if(client_stub.Order(send_order, &get_robo)<0){
                    break;
                }
                timer.EndAndMerge();
                send_order.order_number = send_order.order_number + 1;
            }
        }

        else if(request_type == 2){
            c_rec get_record;
            send_order.customer_id = custid;
            send_order.order_number = -1;
            send_order.request_type = 2;
            for (int i=0; i<orders; i++){ 
                timer.Start();
                if(client_stub.ReadRecord(send_order, &get_record) >= 0){
                    //std::cout<<"Customer record: "<<get_record.customer_id<<":"<<get_record.last_order<<std::endl;
                    timer.EndAndMerge();
                }
                else{
                    std::cout<<"Error: Issue with the server, unable to read the record."<<std::endl;
                }
            }
        }
    client_stub.close_socket();
    }
}


ClientTimer ClientThreadClass::GetTimer() {
    return timer;
}
