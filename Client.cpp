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

class Client{

    const char *server_addr_str;
    int port_addr;
    int customers;
    int orders;
    int request_type;
    std::vector<std::thread> client_workers;
    std::vector<std::shared_ptr<ClientThreadClass>> client_vector;
    ClientTimer timer;

    public:
        /******
         * Constructor that accepts server ip address and port to establish connection with the server,
         * number of customers that wants to place order for robot and number of orders that each customer wants to place
         * It also requires information if the required robot is normal robot or special robot. 
        */
        Client(const char *server_ad, int port_addr_in, int cust, int ord, int request_typ){
            server_addr_str=server_ad;
            port_addr=port_addr_in;
            customers=cust;
            orders=ord;
            request_type=request_typ;
        }

        


        /***********
         * This function fetches all the record from the server and prints it in the 
         * standard out.
        */
        void print_record(int first_custid){
            ClientStub client_stub;
            if(client_stub.Init(server_addr_str,port_addr)<0){
                std::cout<<"Error: Issue with the server"<<std::endl;
            }
            else{
                order_inf send_order;
                std::cout<<"cust_id"<<"\t"<<"last_order"<<std::endl;
                for (int i=first_custid; i<(first_custid+orders); i++){
                    c_rec get_record;
                    send_order.customer_id = i;
                    send_order.order_number = -1;
                    send_order.request_type = 2;
                    if(client_stub.ReadRecord(send_order, &get_record) >=0 ){
                        std::cout<<get_record.customer_id<<"\t"<<get_record.last_order<<std::endl;
                    }else{
                        std::cout<<"Error: Issue with the server, unable to read the record."<<std::endl;
                    }
                }
            }
            client_stub.close_socket();
        }


        /*********
         * This function spawns a new thread and assigns a customer-id to each customer 
         * that wants to place order for the robot.
        */
        void run(){
            int cust_id = 3000;
            if (request_type == 3){
                client_workers.push_back(std::thread(&Client::print_record, this, cust_id));
                for (std::thread & th : client_workers){
                    if (th.joinable())
                        th.join();   
                }
            }
            else{
                timer.Start();
                for (int i=0; i<customers; i++){
                    int customer_id= i + cust_id;
                    auto client_cls = std::shared_ptr<ClientThreadClass>(new ClientThreadClass());

                    client_workers.push_back(std::thread(&ClientThreadClass::Client_thread_body, client_cls, customer_id, server_addr_str, port_addr, orders, request_type));
                    client_vector.push_back(std::move(client_cls));
                }
                for (std::thread & th : client_workers){
                    if (th.joinable())
                        th.join();   
                }
                timer.End();
                for (auto& cls : client_vector) {
		            timer.Merge(cls->GetTimer());	
	            }
            }
        }

        /*********
         * This function is called to join all the spawned thread to the main thread so that main thread waits
         * for all the child threads to complete before closing itself.
        */
        void close_threads(){
                for (std::thread & th : client_workers){
                    if (th.joinable())
                        th.join();   
                }
        }

        /*****
         * This function prints the performance metrics.
         *  This prints:
         * [avg latency ] [min latency ] [max latency ] [ throughput ]
        */
        void print_perf_metric(){
            timer.PrintStats();
        }

};


/*****
 * This is the main funciton where the program starts. It takes arguments to be used to connect to the robot factory server
 * and place the order based on the customer information provided.
 * The program should be run with the below arguments
 * ./ client [ip addr ] [ port #] [# customers ] [# orders ] [ robot type ]
 * server ip address
 * server port address
 * number of customers
 * number of orders per customer
 * robot type: 0: regular robot / 1: special robot
*/
int main (int argc, char* argv[]){
    const char *server_addr_str;
    int port_addr;
    int customers;
    int orders;
    int request_type;
    if (argc != 6){
        std::cout<<"Wrong number of arguments: Expecting 6, got"<<argc<<std::endl;
        exit(1);
    }

    server_addr_str = argv[1];
    port_addr = atoi(argv[2]);
    customers = atoi(argv[3]);
    orders = atoi(argv[4]);
    request_type = atoi(argv[5]);
    Client exec_client(server_addr_str, port_addr, customers, orders, request_type);
    exec_client.run();

    if (request_type == 1 || request_type ==2)
        {exec_client.print_perf_metric();}
    return 0;
    }