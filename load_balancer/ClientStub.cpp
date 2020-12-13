#include "ClientStub.h"
#include <iostream>
ClientStub::ClientStub() {}

int ClientStub::Init(std::string ip, int port) {
	return socket.Init(ip, port);	
}

RobotInfo ClientStub::Order(CustomerRequest request) {
    RobotInfo info;
    char buffer[32];
    int size;
    request.Marshal(buffer);
    size = request.Size();
    if (socket.Send(buffer, size, 0)) {
        size = info.Size();
        if (socket.Recv(buffer, size, 0)) {
            info.Unmarshal(buffer);
        } 
    }
    return info;
}


CustomerRecord ClientStub::ReadRecord(CustomerRequest request) {
    CustomerRecord cr;
    char buffer[32];
    int size;
    request.Marshal(buffer);
    size = request.Size();
    if (socket.Send(buffer, size, 0)) {
        size = cr.Size();
        if (socket.Recv(buffer, size, 0)) {
            cr.Unmarshal(buffer);
        } 
    }
    return cr;
}



/*******
 * sends an order to the server and receives the robot (or robot information) from the server
 * Once the function is called the order details should be marshalled into a byte stream, which
 * is then sent through a socket connection to the server and then wait for a server's response.
 * Once the server responds back with robot information in a byte stream format, the byte
 * stream is unmarshalled to robot information and returned.
 * */
int  ClientStub::struct_order(order_inf new_order, robot_inf * get_robo){
    char *sending_data = new char[sizeof(new_order)];
    memcpy((void*)sending_data, (void*)&new_order, sizeof(new_order));
    if (check((send(socket.get_sock_fd(),sending_data, sizeof(new_order), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
        return -1;
    };

    char *receiving_data = new char[sizeof(*get_robo)];
    if (check((recv(socket.get_sock_fd(), receiving_data, sizeof(*get_robo), 0)), "ERROR reading from socket") <0){
        return -1;
    };

    memcpy(get_robo, receiving_data, sizeof(*get_robo));

   return 0;
}


/*******
 * This sends a read request to the server and gets the response from the server.
 */
int ClientStub::struct_read_record(order_inf new_order, c_rec* get_rec){
    char *sending_data = new char[sizeof(new_order)];
    memcpy((void*)sending_data, (void*)&new_order, sizeof(new_order));
    if (check((send(this->socket.get_sock_fd(),sending_data, sizeof(new_order), MSG_NOSIGNAL)),"ERROR writing to socket")<0){
        return -1;
    };

    char *receiving_data = new char[sizeof(*get_rec)];
    if (check((recv(this->socket.get_sock_fd(), receiving_data, sizeof(*get_rec), 0)), "ERROR reading from socket") <0){
        return -1;
    };
    memcpy(get_rec, receiving_data, sizeof(*get_rec));

    return 0;
}

 int ClientStub::check(int check_code, const char *error_str){
    if (check_code < 0){
        //std::cout<<"Encountered ERROR...."<<std::endl;
        //perror(error_str);
        //exit(1);
        return -1;
    }
    return 0;
}

void ClientStub::ClientStubClose() {
	this->socket.Close();
}