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

class ServerStub{

    int sock_thread;
    public:

        /*********
         * initializes the server stub. Takes a listening socket that is already connected to a client. 
         */
        void Init(int socket_thread){
            sock_thread = socket_thread;
            return;
        }

        
        /*********
         * error handling code. Does not exit from program if error occurs. throws error 
         * and continues with the operation
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



        /**********
         * Receives order from the client.
         * The function waits for client order to arrive through the socket connection.
         * Once the order is received through the socket in a byte stream format, the byte stream 
         * should be unmarshalled to an order and should be returned
         */    
        int ReceiveRequest(order_inf * get_order){
            char *receiving_data = new char[sizeof(*get_order)];
            if (check((recv(sock_thread, receiving_data, sizeof(*get_order), 0)), "ERROR reading from socket") <0){
                return -1;
            };
            memcpy(get_order, receiving_data, sizeof(*get_order));
            delete receiving_data;
            return 0;
        }



        /***********
         *sends the robot information to the client who ordered it. It should marshal the robot information
         * into a byte stream, and send the byte stream through the socket connection to the customer.
         */
        int ShipRobot(robot_inf send_robot){
            char *sending_data = new char[sizeof(send_robot)];
            memcpy((void*)sending_data, (void*)&send_robot, sizeof(send_robot));
            if(check((send(sock_thread, sending_data,sizeof(send_robot), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            };
            //std::cout<<"shipping of order with order number: "<< send_robot.customer_id<<":"<<send_robot.order_number<<std::endl;
            delete sending_data;
            return 0;
        }


        /***********
         *sends the customer record
         */
        int ReturnRecord(c_rec send_rec){
            char *sending_data = new char[sizeof(send_rec)];
            memcpy((void*)sending_data, (void*)&send_rec, sizeof(send_rec));
            if(check((send(sock_thread, sending_data,sizeof(send_rec), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            };
            //std::cout<<"shipping of order with order number: "<< send_rec.customer_id<<":"<<send_rec.last_order<<std::endl;
            delete sending_data;
            return 0;
        }


        /**********
         * Check if the connection is for customer of client
         */
        int GetConnType(conn_type * get_type){

            char *receiving_data = new char[sizeof(conn_type)];
            if(check((recv(sock_thread, receiving_data, sizeof(conn_type), 0)), "ERROR reading from socket")<0){
                return -1;
            };
            memcpy(get_type, receiving_data, sizeof(conn_type));
            delete receiving_data;
            return 0;
        }

        /**********
         * set the connection for admin
         */
        int set_conn_type_admin(){
            conn_type repl_conn;
            repl_conn.is_customer=0;
            repl_conn.get_commit_req=0;
            
            char *sending_conn_type = new char[sizeof(repl_conn)];   
            memcpy((void*)sending_conn_type, (void*)&repl_conn, sizeof(repl_conn));
            if(check((send(sock_thread, sending_conn_type,sizeof(repl_conn), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            }
            delete sending_conn_type;
            return 0;
        }

        /**********
         * set the connection to fetch the last commited index.
         */
        int get_commited_index(commit_in * get_curr_commited_index){
            conn_type repl_conn;
            repl_conn.is_customer=0;
            repl_conn.get_commit_req=1;
            
            char *sending_conn_type = new char[sizeof(repl_conn)];   
            memcpy((void*)sending_conn_type, (void*)&repl_conn, sizeof(repl_conn));
            if(check((send(sock_thread, sending_conn_type,sizeof(repl_conn), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            }
            delete sending_conn_type;
            char *receiving_data = new char[sizeof(*get_curr_commited_index)];
            if(check((recv(sock_thread, receiving_data, sizeof(*get_curr_commited_index), 0)), "ERROR reading from socket")<0){
                return -1;
            };
            memcpy(get_curr_commited_index, receiving_data, sizeof(*get_curr_commited_index));
            delete receiving_data;
            return 0;
        }

        /************************
         * Send Committed index
         */
        int send_commited_index(commit_in send_curr_commited_index){
            char *sending_commit_in = new char[sizeof(send_curr_commited_index)];   
            memcpy((void*)sending_commit_in, (void*)&send_curr_commited_index, sizeof(send_curr_commited_index));
            if(check((send(sock_thread, sending_commit_in,sizeof(send_curr_commited_index), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            }
            delete sending_commit_in;
            return 0;
        }

        /**********
         * Initiate connection with replication server.
         */
        int ReplicationConnectionInit(std::string ip, int port){
            ClientSocket adminClient;
            int sock_thread= adminClient.Init(ip, port);
            if (sock_thread<0){
                return -1;
            }
            return sock_thread;
        }


        /***********
         *sends replication request
         */
        int SendReplicationRequest(repl_req send_req){
            char *sending_data = new char[sizeof(send_req)];
            memcpy((void*)sending_data, (void*)&send_req, sizeof(send_req));
            if(check((send(sock_thread, sending_data,sizeof(send_req), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            };    
            delete sending_data;
            return 0;
        }



        /***********
         *get replication request
         */
        int GetReplicationRequest(repl_req * recv_req){
            char *receiving_data = new char[sizeof(*recv_req)];
            if(check((recv(sock_thread, receiving_data, sizeof(*recv_req), 0)), "ERROR reading from socket")<0){
                return -1;
            };
            memcpy(recv_req, receiving_data, sizeof(*recv_req));
            delete receiving_data;
            return 0;
        }

       
        /***********
         * Get replication Ack
         */
        int GetReplicationAck(repl_ack * is_ack){
            char *receiving_data = new char[sizeof(*is_ack)];
            if(check((recv(sock_thread, receiving_data, sizeof(*is_ack), 0)), "ERROR reading from socket")<0){
                return -1;
            }; 
            memcpy(is_ack, receiving_data, sizeof(*is_ack));
            //close(sock_thread);
            delete receiving_data;
            return 0;
        }



        /***********
         *send replication Ack
         */
        int ReplicationAck(int if_success){
            repl_ack is_ack;
            is_ack.is_success = if_success;
            char *sending_data = new char[sizeof(is_ack)];
            memcpy((void*)sending_data, (void*)&is_ack, sizeof(is_ack));
            if(check((send(sock_thread, sending_data,sizeof(is_ack), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
            };
            delete sending_data;
            //close(sock_thread);
            return 0;
        }



        /**********
         * Send Recovery log
         */
        int SendRecoveryLog(std::vector<map_op> &smr_log){
            commit_in recv_commited_index;
            char *receiving_data = new char[sizeof(recv_commited_index)];
            if(check((recv(sock_thread, receiving_data, sizeof(recv_commited_index), 0)), "ERROR reading from socket")<0){
                    return -1;
            }
            memcpy(&recv_commited_index, receiving_data, sizeof(recv_commited_index));
            int from_index = recv_commited_index.committed_index+1;
            delete receiving_data;
            //send logs from from_index onwards.
            //std::cout<<"log size to send:"<<smr_log.size()<<std::endl;
            for (unsigned int i = from_index; i < smr_log.size(); i++){
                char *sending_data = new char[sizeof(map_op)];
                memcpy((void*)sending_data, (void*)&smr_log[i], sizeof(map_op));
                if(check((send(sock_thread, sending_data,sizeof(map_op), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
                return -1;
                delete sending_data;
                }
            }
            return 0;
        }


        /**************
         * Get Recovery log
         */
        int GetRecoveryLog(std::vector<map_op> &smr_log, int my_last_log_index, int remote_last_log_index){
            //std::cout<<"my last indx:"<<my_last_log_index<<std::endl;
            for (int i = my_last_log_index+1; i <= remote_last_log_index; i++){
                map_op temp_op;
                char *receiving_data = new char[sizeof(map_op)];
                if(check((recv(sock_thread, receiving_data, sizeof(map_op), 0)), "ERROR reading from socket")<0){
                    return -1;
                }
                memcpy(&temp_op, receiving_data, sizeof(map_op));
                auto smr_log_index = smr_log.begin() + i;
                smr_log.insert(smr_log_index, temp_op);
                delete receiving_data;
            }
            //std::cout<<"recoverd log size:"<<smr_log.size()<<std::endl;
            //std::cout<<"remote_last_log_index:"<<remote_last_log_index<<std::endl;
            return 0;
        }




};