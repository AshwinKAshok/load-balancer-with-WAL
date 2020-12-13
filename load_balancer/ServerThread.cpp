#include <iostream>
#include <memory>
#include <errno.h>
#include <string>
#include "ServerThread.h"
#include "ServerStub.h"

RobotFactory::RobotFactory(std::map<int,std::pair<std::string,int>> worker_nodes_map) {
	current_worker_node = 0;
	master_node_id = 0;
	this->worker_nodes_map = worker_nodes_map;
	worker_node_map_size = worker_nodes_map.size();

	for(int i=0;i<worker_node_map_size;i++) {
		node_health[i] = true;
	}
}

robot_inf RobotFactory::CreateRobot(order_inf get_order, ClientStub* clientStub) {
	
	robot_inf get_robo;
	clientStub->struct_order(get_order, &get_robo);
	return get_robo;
}

void RobotFactory::ReadRecord(order_inf request, ServerStub* serverStub, ClientStub* workerNode) {

	c_rec get_rec;
	workerNode->struct_read_record(request, &get_rec);
	serverStub->struct_return_record(get_rec);
}

ClientStub* RobotFactory::get_master_node() {
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	ul.lock();
	if(!node_health[master_node_id]) {
		for(int i=0;i<worker_node_map_size;i++){
			if(node_health[i]) {
				master_node_id = i;
				ClientStub* new_master = new ClientStub();
				std::pair<std::string, int> worker_node_info;
				worker_node_info = worker_nodes_map[i];
				new_master->Init(worker_node_info.first, worker_node_info.second);
				return new_master;
			}
		}

		return NULL;
	}
	ul.unlock();

	ClientStub* new_master = new ClientStub();
	std::pair<std::string, int> worker_node_info;
	worker_node_info = worker_nodes_map[master_node_id];
	new_master->Init(worker_node_info.first, worker_node_info.second);
	return new_master;
}

void RobotFactory::LoadBalancerThread(std::unique_ptr<ServerSocket> socket, int id) {
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);

	int request_type;
	robot_inf robot;
	CustomerRecord record;
	ServerStub* stub = new ServerStub();
	
	stub->Init(std::move(socket));

	order_inf get_order;
	ClientStub* cs = new ClientStub();
	ClientStub* master_node;

	master_node = get_master_node();
	ul.lock();
	std::pair<std::string, int> worker_node_info;

	if(!node_health[current_worker_node]) {
		for(int i=0; i<worker_node_map_size; i++) {
			if(node_health[i]) {
				worker_node_info = worker_nodes_map[i];
				cs->Init(worker_node_info.first, worker_node_info.second);
				break;
			} else {
				cs = NULL;
			}
		}

		if(cs == NULL) {
			//no active worker node could be selected. Hence send back negative resopnse.
			c_rec get_rec;
			get_rec.customer_id = -1;
			get_rec.last_order = -1;
			stub->struct_return_record(get_rec);
			return;
		}
	} else {
		worker_node_info = worker_nodes_map[current_worker_node++];
		cs->Init(worker_node_info.first, worker_node_info.second);
		if(current_worker_node ==  worker_node_map_size)
			current_worker_node = 0;
	}
	
	ul.unlock();

	while (true) {
		if (stub->msg_peek(get_order) == 0){
			break;
		}

		if(stub->struct_receive_request(&get_order) <0) {
			break;
		}

		request_type = get_order.request_type;
		switch (request_type) {
			case 1:
				
				if(master_node!=NULL) {
					robot = CreateRobot(get_order, master_node);
					stub->struct_ship_Robot(robot);
				} else {
					robot.customer_id = -1;
					robot.admin_id = -1;
					robot.engineer_id = -1;
					robot.request_type = -1;
					robot.order_number = -1;
					stub->struct_ship_Robot(robot);
				}
				break;
			case 2:
				ReadRecord(get_order, stub, cs);
				break;
			
			default:
				std::cout << "Undefined robot type1: "
					<< request_type << std::endl;
		}
	}

	// stub->ServerStubClose();
	cs->ClientStubClose();
	master_node->ClientStubClose();

}

void RobotFactory::HertBeatHandler() {
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	std::map<int, ClientStub*> hb_worker_node_array;

	ul.lock();
	for(int i=0; i<worker_node_map_size; i++) {
		std::pair<std::string, int> worker_node_info;
		worker_node_info = worker_nodes_map[i];
		ClientStub* workern_node_conn = new ClientStub();
		workern_node_conn->Init(worker_node_info.first, worker_node_info.second);
		hb_worker_node_array[i] = workern_node_conn;
		node_health[i] = true;
	}
	ul.unlock();

	order_inf hb_order;
	hb_order.customer_id = 3000;
	hb_order.order_number = 0;
	hb_order.request_type = 2;
	while(true) {
		for(int i=0; i<worker_node_map_size; i++) {
			std::this_thread::sleep_for(std::chrono::microseconds(2000000));
			// std::cout<<"heart beat send: "<<i<<std::endl;
			c_rec get_rec;
			if(hb_worker_node_array[i]->struct_read_record(hb_order, &get_rec)<0) {
				ul.lock();
				node_health[i] = false;
				ul.unlock();
			} else {
				ul.lock();
				node_health[i] = true;
				ul.unlock();				
			}
		}
	}
}



