#include "ServerStub.h"
#include <cstring>
#include <iostream>


ServerStub::ServerStub() {}

void ServerStub::Init(std::unique_ptr<ServerSocket> socket) {
	this->socket = std::move(socket);
}


CustomerRequest ServerStub::ReceiveRequest() {
	char buffer[32];
	CustomerRequest cr;
	if (socket->Recv(buffer, cr.Size(), 0)) {
		cr.Unmarshal(buffer);
	}
	return cr;	
}

int ServerStub::ShipRobot(RobotInfo info) {
	char buffer[32];
	info.Marshal(buffer);
	return socket->Send(buffer, info.Size(), 0);
}

int ServerStub::ReturnRecord(CustomerRecord cr) {
	char buffer[32];
	cr.Marshal(buffer);
	return socket->Send(buffer, cr.Size(), 0);
}

/***********
 *sends the robot information to the client who ordered it. It should marshal the robot information
    * into a byte stream, and send the byte stream through the socket connection to the customer.
    */
int ServerStub::struct_ship_Robot(robot_inf send_robot){
    char *sending_data = new char[sizeof(send_robot)];
    memcpy((void*)sending_data, (void*)&send_robot, sizeof(send_robot));
    if(check((send(this->socket->get_sock_fd(), sending_data,sizeof(send_robot), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
        return -1;
    };
    delete sending_data;
    return 0;
}

/***********
 *sends the customer record
	*/
int ServerStub::struct_return_record(c_rec send_rec){
	char *sending_data = new char[sizeof(send_rec)];
	memcpy((void*)sending_data, (void*)&send_rec, sizeof(send_rec));
	if(check((send(this->socket->get_sock_fd(), sending_data,sizeof(send_rec), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
		return -1;
	};
	delete sending_data;
	return 0;
}

/**********
 * Receives order from the client.
 * The function waits for client order to arrive through the socket connection.
 * Once the order is received through the socket in a byte stream format, the byte stream 
 * should be unmarshalled to an order and should be returned
 */    
int ServerStub::struct_receive_request(order_inf * get_order){
	char *receiving_data = new char[sizeof(*get_order)];
	if (check((recv(this->socket->get_sock_fd(), receiving_data, sizeof(*get_order), 0)), "ERROR reading from socket") <0){
		return -1;
	};

	memcpy(get_order, receiving_data, sizeof(*get_order));
	delete receiving_data;
	return 0;
}



/*********
 * error handling code. Does not exit from program if error occurs. throws error 
 * and continues with the operation
 */
int ServerStub::check(int check_code, const char *error_str){
	if (check_code < 0){
		//std::cout<<"Encountered ERROR...."<<std::endl;
		//perror(error_str);
		//exit(1);
		return -1;
	
	}
	return 0;
}


int ServerStub::msg_peek(order_inf get_order) {
	if(recv(this->socket->get_sock_fd(), &get_order, sizeof(get_order), MSG_PEEK) == 0) {
		this->socket->Close();
		return 0;
	}

	return 1;
	
}

void ServerStub::ServerStubClose() {
	this->socket->Close();
}