#ifndef __SERVERTHREAD_H__
#define __SERVERTHREAD_H__

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <map>
#include <vector>

#include "Messages.h"
#include "ServerSocket.h"
#include "ClientStub.h"
#include "ServerStub.h"

struct ExpertRequest {
	ServerStub* serverStub;
	ClientStub* workerNode;
	order_inf request;
};

class RobotFactory {
private:

	int current_worker_node;
	int master_node_id;
	int worker_node_map_size;
	
	std::mutex erq_lock;
	std::map<int, std::pair<std::string,int>> worker_nodes_map;
	std::map<int, bool> node_health;
	robot_inf CreateRobot(order_inf order, ClientStub* clientStub);
	void ReadRecord(order_inf get_order, ServerStub* serverStub, ClientStub* workerNode);
	ClientStub* get_master_node();

public:
	RobotFactory(std::map<int,std::pair<std::string,int>> worker_nodes_map);

	// Main thread responsible for handling load balancer duty
	void LoadBalancerThread(std::unique_ptr<ServerSocket> socket, int id); //equivalent to Engineer thread
	void HertBeatHandler();
};

#endif // end of #ifndef __SERVERTHREAD_H__
