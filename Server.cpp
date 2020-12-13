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
#include "ServerStub.h"
#include <map>
#include <future>
#include <signal.h>
#include "CommitLog.h"
#include <algorithm>


std::mutex mtx;
std::condition_variable condition_var;
#define F_FLUSH 0 // 1 to sync to file after every write and 0 to use system default buffer write.
#define CRC_ENABLED 1 // 1 to enable cyclic redundnacy check to check for data inconsistency.
#define WAL_ENABLED 1 // 1 to enable write ahead log and 0 to disable.
#define BATCH_SIZE 10 // should be 1 or greater
#define READ_DELAY 0 // This will add delay using while loop for read operation. to test load balancer throughput.


class ServerSocket{

    int client_port_addr;
    int my_factory_port;
    int my_factory_id;
    std::list <robot_inf *> admin_thread_list;
    std::list <std::promise<void> *> barrier_list;
    std::vector<std::thread> workers;
    std::vector<std::thread> simple_workers;
    std::vector<std::thread> ifa_thread_workers;
    std::map<int, int> customer_record;
    std::vector<map_op> smr_log;
    std::vector<factory_rec> factories_record;
    smr_lcommit smr_repl_commit;
    int recheck_allow;
    CommitLog clog;

    public:

        /*****
         * This is the constructor for server program which set the port address to start the server on
        */
        ServerSocket(int client_port_addr_in, int my_factory_ifa_port){
            client_port_addr = client_port_addr_in;
            my_factory_port = my_factory_ifa_port;
        }


        /********
         * This is the engineer thread body which is spawned by the main thread for every customer request it recieves.
         * A single engineer thread will cater to all the orders of the same client that it is connected to, and once it sends
         * robot information back to the client the thread is closed. . It also wait for an admin  thread to respond back. 
         * Once the response is received, the regular engineer thread sends back the
         * robot information with added expert id to the customer.
        */
         void engineer_thread_body(int * p_socket, int eng_id){
            int socket = *p_socket;
            free(p_socket);

            ServerStub server_conn;
            server_conn.Init(socket);
            int send_last_committed_index=0;
                    
            while(true){
                order_inf make_order;
                if (recv(socket, &make_order, sizeof(make_order), MSG_PEEK) == 0){
                    if(send_last_committed_index){ 
                        engineer_thread_send_commited_index();
                    }
                    if (WAL_ENABLED){
                        clog.sync(); // call fflush to write all the left over data to disk
                    }
                    close(socket);
                    recheck_allow = 1;
                    break;
                }
                else{
                    if (server_conn.ReceiveRequest(&make_order)<0){
                        break;
                    }                   
                    // Send reply for a robot order
                    if (make_order.request_type == 1){ 
                        send_last_committed_index = 1;
                        engineer_thread_robot_def(server_conn, make_order, eng_id);
                    }
                    //Send the reply to read data
                    else if (make_order.request_type == 2 ){
                        send_last_committed_index = 0;
                        engineer_thread_read_def(server_conn, make_order);
                        //print disk throughput here
                        //clog.GetDiskThroughput();
                    }
                    }
            }

            return;
        }

    

        /******
         * This function commits the last entry in the log when backup server becomes primary
         */
        void recheckcommit(){
            
            if (smr_repl_commit.last_index >= 0 &&  smr_repl_commit.last_index > smr_repl_commit.committed_index){
                map_op &commited_mapop = smr_log.at(smr_repl_commit.last_index); 
                customer_record[commited_mapop.arg1]=commited_mapop.arg2;
                smr_repl_commit.committed_index=smr_repl_commit.last_index;
                }
        }

        /******
         * This function creates socket connection from PFA to IFA whenever the customer sends the request.
         * This function only run when recheck_allow = 1. This is to check if connection to all the backup are entact and
         * in case it is not it will re-try to connect whenever there is a new connection from client.
         */
        void recheckreplsock(){
            for (unsigned int i=0; i< factories_record.size(); i++){
                if (factories_record[i].socket < 0){
                    ServerStub pfa_conn;
                    int sock = pfa_conn.ReplicationConnectionInit(factories_record[i].factory_ip, factories_record[i].factory_port);
                    if (sock > 0){
                        factories_record[i].socket = sock;
                        pfa_conn.Init(sock);
                        pfa_conn.set_conn_type_admin();
                    }
                    else{
                        factories_record[i].socket = -1;
                    }
                }
            }
        }

        /********
         * This function performs the functionality of engineer thread and ships the robot to the customer.
         * This function runs when the request_type=1.
         */
        void engineer_thread_robot_def(ServerStub engineer, order_inf make_order, int eng_id){
            //std::cout<<"inside engineer thread";
            robot_inf ship_order = assemble_robot(make_order, eng_id);
            std::promise<void> barrier;                      // create promise
            std::future<void> barrier_future = barrier.get_future();   // engagement with future
            std::unique_lock<std::mutex> ul(mtx);
            admin_thread_list.push_back(&ship_order);
            barrier_list.push_back(&barrier);
            ul.unlock();
            condition_var.notify_one();
            barrier_future.wait();
            engineer.ShipRobot(ship_order);
            return;
        }


        /********
         * This function sends the commited index to all replicas when the client closes the connection
         */
        void engineer_thread_send_commited_index(){
            robot_inf repl_commit_index;
            repl_commit_index.customer_id=-1;
            repl_commit_index.order_number=-1;
            std::promise<void> barrier;                      // create promise
            std::future<void> barrier_future = barrier.get_future();   // engagement with future
            std::unique_lock<std::mutex> ul(mtx);
            admin_thread_list.push_back(&repl_commit_index);
            barrier_list.push_back(&barrier);
            ul.unlock();
            condition_var.notify_one();
            barrier_future.wait();
            return;
        }


        /********
         * This function performs the functionality of engineer thread and sends reply to read request from the customer.
         * This function runs when the request_type=2.
         */
        void engineer_thread_read_def(ServerStub engineer, order_inf make_order){
            std::unique_lock<std::mutex> ul(mtx);
            c_rec send_rec;
            send_rec.customer_id=make_order.customer_id;
            if (customer_record.find(make_order.customer_id) == customer_record.end()){
                        send_rec.last_order =  -1;
            } else {
                        send_rec.last_order = customer_record[make_order.customer_id];
            }
            if(READ_DELAY){
                for (int i =0; i< 1000000; i++){
                    for (int i =0; i< 100000; i++){
                    }
                }
            }
            engineer.ReturnRecord(send_rec);
            return;
        }

        /********
         * This function performs recovery when the backup node recovers from failure.
         * This function checks the last log entry in itself and the last log entry of the primary and 
         * request the primary for all the logs in case there is difference of more than one.
         */
        void  initiate_recovery(ServerStub *ifa, int remote_last_log_index, int remote_last_commited_index){
            int ack =2;
                if (ifa->ReplicationAck(ack) < 0){
                    smr_repl_commit.primary_id = -1;
                        }
                else{
                    //send the local last commited index and get the data from there.
                    commit_in send_commited_index;
                    send_commited_index.committed_index = smr_repl_commit.committed_index;
                    ifa->send_commited_index(send_commited_index);
                    auto t1 = std::chrono::high_resolution_clock::now();
                    if (ifa->GetRecoveryLog(smr_log, smr_repl_commit.last_index, remote_last_log_index) < 0){
                        smr_repl_commit.primary_id = -1;
                    }
                    else{
                        auto t2 = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
                        std::cout<<"Recovered logs from remote server in: "<<duration<<" microseconds"<<std::endl;

                        for (int i=smr_repl_commit.last_index + 1 ; i<=remote_last_log_index; i++){
                            map_op &commited_mapop = smr_log.at(i); // prone to error 
                            if (WAL_ENABLED){
                                if (CRC_ENABLED){
                                    clog.writelogwithcrc(commited_mapop);
                                }
                                else{
                                    clog.writelog(commited_mapop);
                                }
                            }
                            if (i <=remote_last_commited_index){
                                customer_record[commited_mapop.arg1]=commited_mapop.arg2;
                                smr_repl_commit.committed_index=i;
                            }
                        }
                    }
                }
       }

        /*********
         * This function initiates recovery from the commit log till the commited index. 
         */
        void initiate_recovery_from_log(){
            int last_commited_index = 0;
            ServerStub pfa_conn;
            commit_in get_commited_index;

            for (unsigned int i=0; i< factories_record.size(); i++){
                int sock = pfa_conn.ReplicationConnectionInit(factories_record[i].factory_ip, factories_record[i].factory_port);
                if (sock > 0){
                    pfa_conn.Init(sock);
                    if(pfa_conn.get_commited_index(&get_commited_index)<0){
                        continue;
                    }
                    if (get_commited_index.committed_index == -1){

                    }
                    last_commited_index = get_commited_index.committed_index;
                    close(sock);
                    break;
                }
            }

            std::string file_name = "commit_" + std::to_string(my_factory_id) + ".log";
            if (WAL_ENABLED){
                auto t1 = std::chrono::high_resolution_clock::now();
                if(CRC_ENABLED){
                    clog.Recoverwithcrc(smr_log, file_name.c_str());
                }
                else{
                    clog.Recover(smr_log, file_name.c_str());
                }
                auto t2 = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
                std::cout<<"Recovered logs from Commit Log in: "<<duration<<" microseconds"<<std::endl;
            }

            //std::cout<<smr_log.size()<<std::endl;
            if ((int)smr_log.size() == 0 && last_commited_index == 0){
                return;
            }
            // if the last commited index is more than what is there in log
            // there is a need to fetch the logs from primary server
            if (last_commited_index > (int)smr_log.size()){
                 last_commited_index = smr_log.size() - 1;

            }
            // if the server is unable to fetch the last commited index from 
            // replicas then write all the data in the log.

            if (last_commited_index == 0){
                last_commited_index = smr_log.size() - 1;
            }
            //std::cout<<last_commited_index<<std::endl;
            for (int i=0 ; i<=last_commited_index; i++){
                            map_op &commited_mapop = smr_log.at(i); // prone to error 
                            customer_record[commited_mapop.arg1]=commited_mapop.arg2;
                            smr_repl_commit.committed_index=i;
                            smr_repl_commit.last_index=i;

                            //std::cout<<commited_mapop.arg1<< " : "<< commited_mapop.arg2 <<std::endl;
                        }

            return;
       }


        /********
         * This is IFA thread which performs replication of logs and commit the last commited log to the 
         * map. This function works on the backup servers.
         */

        void ifa_thread_read(ServerStub *ifa){
            std::unique_lock<std::mutex>ul(mtx);
            repl_req get_repl_inf;
            if(ifa->GetReplicationRequest(&get_repl_inf)<0){
                smr_repl_commit.primary_id = -1;
            }
            else{
                if (get_repl_inf.last_index - smr_repl_commit.last_index > 1){
                    //std::cout<<"repl request:"<<get_repl_inf.last_index << "----"<< get_repl_inf.committed_index<<std::endl;
                    initiate_recovery(ifa, get_repl_inf.last_index, get_repl_inf.committed_index);
                }
                //check if the request is not just the last commited index record
                else if (get_repl_inf.mapop_record.arg1 != -1){
                    auto smr_log_index = smr_log.begin() + get_repl_inf.last_index;
                    //write log to disk before pushing it to the memory
                    if (WAL_ENABLED){
                        if (CRC_ENABLED){
                            clog.writelogwithcrc(get_repl_inf.mapop_record);
                        }
                        else{
                            clog.writelog(get_repl_inf.mapop_record);
                        } 
                        clog.sync(); // call fflush to write to disk
                    }
                    smr_log.insert(smr_log_index, get_repl_inf.mapop_record);
                    
                }
                smr_repl_commit.primary_id = get_repl_inf.factory_id;
                smr_repl_commit.last_index = get_repl_inf.last_index;
                //committed_index = -1 for first request; do not write that in customer record.
                if (get_repl_inf.committed_index != -1){
                    map_op &commited_mapop = smr_log.at(get_repl_inf.committed_index); // prone to error 
                    customer_record[commited_mapop.arg1]=commited_mapop.arg2;
                    smr_repl_commit.committed_index=get_repl_inf.committed_index;
                    }
            }
            return;
        }


        /***************
         * This is function of the admin thread. It works on the primary node as a PFA and sends the
         * replication request to the backup nodes. This also commit customer data to the map.
        */
        void admin_thread(int admin_id){
            std::unique_lock<std::mutex>ul(mtx);
            while (true){
                while (admin_thread_list.empty()){
                    condition_var.wait(ul);
                }
                if (recheck_allow == 1){
                    recheckcommit();
                    recheckreplsock();
                    recheck_allow = 0;
                    }
                robot_inf *ship_order_p = admin_thread_list.front();
                std::promise<void> *barrier = barrier_list.front();
                admin_thread_list.pop_front();
                barrier_list.pop_front();
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                map_op log_entry;
                log_entry.opcode = 1;
                log_entry.arg1 = ship_order_p->customer_id;
                log_entry.arg2 = ship_order_p->order_number;

                smr_repl_commit.primary_id = my_factory_id;

                //checks if it is not a last request to send commited index.
                if (ship_order_p->customer_id != -1){
                    smr_repl_commit.last_index = smr_repl_commit.last_index +1;
                    auto smr_log_index = smr_log.begin() + smr_repl_commit.last_index;

                    //TODO: insert log into disk here.
                    if (WAL_ENABLED){
                        if (CRC_ENABLED){
                            clog.writelogwithcrc(log_entry);
                        }
                        else{
                            clog.writelog(log_entry);
                        }
                    }
                    smr_log.insert(smr_log_index, log_entry);
                }
                //print the disk throughput
                // else{
                //     //clog.GetDiskThroughput();
                // }

                repl_req send_req;
                send_req.committed_index = smr_repl_commit.committed_index;
                send_req.factory_id = my_factory_id;
                send_req.last_index = smr_repl_commit.last_index;
                send_req.mapop_record = log_entry;
                //std::cout<<"sender repl request:"<<send_req.last_index << "----"<< send_req.committed_index<<std::endl;
                for (unsigned int i=0; i< factories_record.size(); i++){
                    if (factories_record[i].socket >= 0){
                        ServerStub pfa_conn;
                        pfa_conn.Init(factories_record[i].socket);
                        if(pfa_conn.SendReplicationRequest(send_req)<0){
                            factories_record[i].socket = -1;
                            continue;
                        }
                        repl_ack is_ack;

                        if(pfa_conn.GetReplicationAck(&is_ack)<0){
                            factories_record[i].socket = -1;
                            continue;
                        }
                        int is_success = is_ack.is_success;
                        if (is_success == 2){ // Check if the secondary needs to get recovery log
                            if(pfa_conn.SendRecoveryLog(smr_log)<0){
                                factories_record[i].socket = -1;
                                continue;
                            }
                            repl_ack reack;
                            if(pfa_conn.GetReplicationAck(&reack)<0){
                                factories_record[i].socket = -1;
                                continue;
                            }
                            is_success = reack.is_success;
                        }
                        if (is_success !=1){
                            factories_record[i].socket = -1;
                            continue;
                        }
                        //else{std::cout<<"Replication successful"<<std::endl;}
                    }
                }
                //checks if it is not a last request to send commited index.
                if (ship_order_p->customer_id != -1){
                    customer_record[log_entry.arg1]=log_entry.arg2;
                    smr_repl_commit.committed_index = smr_repl_commit.last_index;
                    ship_order_p->admin_id = admin_id;
                }
                barrier->set_value();
            }
        }

        /*********
         * error handling code. Does not exit from program if error occurs. throws error 
         * and respond the calling function with -1. It is upto calling function how to handle it.
        */
        int check(int check_code, const char *error_str){
            if (check_code < 0){
                //std::cout<<"Encountered ERROR..."<<std::endl;
                //perror(error_str);
                //exit(1);
                return -1;
            }
            return 0;
        }

        /*****
         * This is the IFA thread body which gets spawned by the ifa listener thread when the
         * Admin of different factory tries to communicate
         */
        void ifa_thread_body(int * p_socket){
            int socket = *p_socket;
            free(p_socket);

            ServerStub server_conn;
            server_conn.Init(socket);
            conn_type get_conn;
            server_conn.GetConnType(&get_conn);

			if (get_conn.get_commit_req == 0)
				while(true){
					repl_req get_repl_inf;
					if (recv(socket, &get_repl_inf, sizeof(get_repl_inf), MSG_PEEK) == 0){
						close(socket);
						smr_repl_commit.primary_id = -1;
						break;
					}
					else{
						ifa_thread_read(&server_conn);
						int ack =1;
						if (server_conn.ReplicationAck(ack) < 0){
							smr_repl_commit.primary_id = -1;
							break;
						}
					}
				}
				//send the current commited index when requested. 
			else if (get_conn.get_commit_req){
				commit_in send_commited_index;
				send_commited_index.committed_index = smr_repl_commit.committed_index;
				server_conn.send_commited_index(send_commited_index);
				close(socket);
			}

            else{
                std::cout<<"Wrong connection type encountered... closing connection."<<std::endl;
            }
            return;
        }

        
        /******
         * This is the listener thread for IFA. this waits for communication from admin
         * of the factory which is currently the leader and spawns a new thread to communicate.
         */
        void ifa_listener_thread(){
            int serv_sock_fd, new_socket_fd, portno, clilent_len; 
            struct sockaddr_in serv_addr, client_addr;
            int client_queue_len = 10;

            check((serv_sock_fd = socket(AF_INET, SOCK_STREAM, 0)),"ERROR opening socket");

            //populate the server details
            bzero((char *) &serv_addr, sizeof(serv_addr));
            portno = my_factory_port;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            serv_addr.sin_port = htons(portno);

            //try to bind
            if(bind(serv_sock_fd, (struct sockaddr *) &serv_addr,sizeof(serv_addr))<0){ 
                perror("ERROR on binding to back channel port");
                exit(1);
            }

            //start listening
            if((listen(serv_sock_fd,client_queue_len))<0){
                perror("Listen Filed on back channel port");
                exit(1);
            } 
            clilent_len = sizeof(client_addr);
            //char client_address[clilent_len + 1];
            printf("Backend channel started. Waiting for connection.. \n");

            

            //imortal loop to keep on waiting for connection initiated by cient and spin up new thread for each client.
            while(true){
                check((new_socket_fd = accept(serv_sock_fd, (struct sockaddr *) &client_addr, (socklen_t*)&clilent_len)),
                "ERROR on accept");
                int *pclient = (int*)malloc(sizeof(int));
                *pclient = new_socket_fd;
                ifa_thread_workers.push_back(std::thread(&ServerSocket::ifa_thread_body, this, pclient));
            }


        }

        /********
         * This function is called by the main to start the server. This starts the server in an endless loop and ctrl-c
         * is to be used to exit from this. This function spawns thread pool of expert engineers and the number of threads 
         * in the thread pool is determined by the argument passed to this fucntion. This function also spawns new engineer
         * thread for each customer that connects to the server.
        */
        int start(int fac_id, std::vector<factory_rec> factories)
        {
            int serv_sock_fd, new_socket_fd, portno, clilent_len; 
            struct sockaddr_in serv_addr, client_addr;
            int client_queue_len = 1000;
            my_factory_id = fac_id;
            factories_record = factories;
            smr_repl_commit.factory_id = my_factory_id;
            smr_repl_commit.primary_id = -1;
            smr_repl_commit.last_index = -1;
            smr_repl_commit.committed_index = -1;


            recheck_allow=1;
            

            //initiate logging.
            std::string file_name = "commit_" + std::to_string(my_factory_id) + ".log";
            if (WAL_ENABLED){
                clog.Init(file_name.c_str(), F_FLUSH, BATCH_SIZE);
                initiate_recovery_from_log();
            }


            //thread pool creation
            // int exprt_id = rand() % 55 + 500;
            // for (int i=0; i < expert_thrd_no; i++){
            //     workers.push_back(std::thread(&ServerSocket::expert_thread_pool_body, this, exprt_id));
            //     exprt_id++;
            // }
            
            int admin_thread_id = my_factory_id + 100;
            workers.push_back(std::thread(&ServerSocket::admin_thread, this, admin_thread_id));
            workers.push_back(std::thread(&ServerSocket::ifa_listener_thread, this));
            
            check((serv_sock_fd = socket(AF_INET, SOCK_STREAM, 0)),"ERROR opening socket");

            //populate the server details
            bzero((char *) &serv_addr, sizeof(serv_addr));
            portno = client_port_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = INADDR_ANY;
            serv_addr.sin_port = htons(portno);

            //try to bind
            if(bind(serv_sock_fd, (struct sockaddr *) &serv_addr,sizeof(serv_addr))<0){ 
                perror("ERROR on binding");
                exit(1);
            }

            //start listening
            if((listen(serv_sock_fd,client_queue_len))<0){
                perror("Listen Filed");
                exit(1);
            } 
            clilent_len = sizeof(client_addr);
            //char client_address[clilent_len + 1];
            printf("Server Started. Waiting for connection.. \n");

            

            //imortal loop to keep on waiting for connection initiated by cient and spin up new thread for each client.
            while(true){
                int engineer_id = 100;
                check((new_socket_fd = accept(serv_sock_fd, (struct sockaddr *) &client_addr, (socklen_t*)&clilent_len)),
                "ERROR on accept");
                // inet_ntop(AF_INET, &client_addr, client_address, clilent_len);
                // printf("Client initiated connection from IP: %s\n", client_address);
                // bzero(&client_address,clilent_len + 1);

                int *pclient = (int*)malloc(sizeof(int));
                *pclient = new_socket_fd;
                
                simple_workers.push_back(std::thread(&ServerSocket::engineer_thread_body, this, pclient, engineer_id));
                engineer_id++;
                
            }
            return 0; 
        }

        /********
         * This function assembles the robot and all the details to the robot info. 
        */
        robot_inf assemble_robot(order_inf order, int engineer_id){
            robot_inf new_robot;
            new_robot.customer_id=order.customer_id;
            new_robot.engineer_id=engineer_id;
            new_robot.admin_id=-1;
            new_robot.order_number=order.order_number;
            new_robot.request_type=order.request_type;
            return new_robot;
        }
};



/*********
 * This is the main funciton where the program starts from, it takes port addess as the first argument, 
 * which specifies the port on which the server should start. Also it takes an additional optional 
 * arguemnt which specifies the number of expert threads that needs to be spawned in the thread pool 
 * to cater to request for special robots.
 * */
int main(int argc, char* argv[]){

    std::vector<factory_rec> factories;
    if (argc < 2 || argc%3 != 2){
        std::cout<<"Wrong number of arguments: "<<argc<<std::endl;
        exit(1);
    }

    for (int i=1 ;i<=atoi(argv[3]); i++){
        factory_rec factory;
        int id = (3 * i) + 1;
        factory.factory_id = atoi(argv[id]);
        factory.factory_ip = argv[id+1];
        factory.factory_port = atoi(argv[id+2]);
        factory.socket = -1;
        factories.push_back(factory);
    }


    int client_port_addr = atoi(argv[argc -1]);
    if (client_port_addr < 1024 || client_port_addr > 65535){
        std::cout<<"Please enter port number between 1024 and 65535"<<argc<<std::endl;
        exit(1);
    }

    int factory_port_addr = atoi(argv[1]);
    if (factory_port_addr < 1024 || factory_port_addr > 65535){
        std::cout<<"Please enter port number between 1024 and 65535"<<argc<<std::endl;
        exit(1);
    }

    int factory_id = atoi(argv[2]);

    ServerSocket newserver(client_port_addr, factory_port_addr);
    newserver.start(factory_id, factories);
}
