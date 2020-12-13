#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "ServerSocket.h"
#include "ServerThread.h"

int main(int argc, char *argv[]) {
	int port;
	int engineer_cnt = 0;
	ServerSocket socket;
	
	std::unique_ptr<ServerSocket> new_socket;
	std::vector<std::thread> thread_vector;
	std::map<int,std::pair<std::string,int>> worker_node_map;

	port = atoi(argv[1]);
	int num_worker_nodes = atoi(argv[2]);

	int j=3;
	for(int i=1; i<=num_worker_nodes; i++) {
		int worker_node_id = atoi(argv[j++]);
		std::string worker_node_ip = argv[j++];
		int worker_node_port = atoi(argv[j++]);
		worker_node_map[worker_node_id] = std::make_pair(worker_node_ip, worker_node_port);
	}
	RobotFactory factory(worker_node_map);

	if (!socket.Init(port)) {
		std::cout << "Socket initialization failed" << std::endl;
		return 0;
	}

	std::thread load_balancer_thread(&RobotFactory::HertBeatHandler, &factory);

	while ((new_socket = socket.Accept())) {
		std::thread load_balancer_thread(&RobotFactory::LoadBalancerThread, &factory, 
				std::move(new_socket), engineer_cnt++);
		thread_vector.push_back(std::move(load_balancer_thread));
	}
	
	return 0;
}