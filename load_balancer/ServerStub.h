#ifndef __SERVER_STUB_H__
#define __SERVER_STUB_H__

#include <memory>
#include <stdio.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "ServerSocket.h"
#include "Messages.h"

class ServerStub {
private:
	std::unique_ptr<ServerSocket> socket;
public:
	ServerStub();
	void Init(std::unique_ptr<ServerSocket> socket);
	CustomerRequest ReceiveRequest();
	int ShipRobot(RobotInfo info);
	int ReturnRecord(CustomerRecord cr);
	int struct_ship_Robot(robot_inf send_robot);
	int struct_return_record(c_rec send_rec);
	int struct_receive_request(order_inf * get_order);
	int check(int check_code, const char *error_str);
	int msg_peek(order_inf get_order);
	void ServerStubClose();
};

#endif // end of #ifndef __SERVER_STUB_H__