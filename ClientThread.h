#include <chrono>
#include <ctime>
#include <string>

#include"ClientStub.h"
#include "ClientTimer.h"



class ClientThreadClass {

	ClientTimer timer;
    ClientStub client_stub;
public:
	ClientThreadClass();
    
	/******
         * This is the customer function which is spawned as a child thread by the main thread. Each customer has its 
         * own thread and unique customer id associated with it which creates a new socket and initiates connection to 
         * the server with iand process all the orders associated with the secific customer. Once all the robot 
         * information is recieved for the particular customer the thread closes itself and joins the main thread.
        */
	void Client_thread_body(int custid, const char *server_addr_str, int port_addr, int orders, int request_type);
	ClientTimer GetTimer();
};
