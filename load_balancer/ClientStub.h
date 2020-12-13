#ifndef __CLIENT_STUB_H__
#define __CLIENT_STUB_H__

#include <string>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "ClientSocket.h"
#include "Messages.h"

class ClientStub {
private:
	ClientSocket socket;
public:
	ClientStub();
	int Init(std::string ip, int port);
	RobotInfo Order(CustomerRequest order);
	CustomerRecord ReadRecord(CustomerRequest request);
	
	int struct_read_record(order_inf new_order, c_rec* get_rec);
	int check(int check_code, const char *error_str);
	int struct_order(order_inf new_order, robot_inf * get_robo);
	void ClientStubClose();
};


#endif // end of #ifndef __CLIENT_STUB_H__