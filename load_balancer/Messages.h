#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <string>


/***
 * Structure of order information that is to be sent by customer/client 
 * and processed by thr factory server to create the robot. 
 */
typedef struct order_info{
    int customer_id ; // customer id
    int order_number ; // # of orders issued by this customer so far
    int request_type ; // either 1 - regular order or 2 - read a customer record
}order_inf;

/***
 * Structure of robot that is to be created by the robot factory server and 
 * sent to the customer/client that ordered it. 
 */
typedef struct robot_info{
    int customer_id ; // copied from the order
    int order_number ; // copied from the order
    int request_type ; // either 1 - regular order or 2 - read a customer record
    int engineer_id ; // id of the engineer who created the robot
    int admin_id ; // id of the admin who added the record
    // -1 indicates that it was read request
}robot_inf;

/***
 * Structure of customer record
 */
typedef struct cust_record{
    int customer_id ; // copied from the read request; -1 if customer_id is not found in the map
    int last_order ; // copied from the map, -1 if customer_id is not found in the map
}c_rec;



class CustomerRequest {
private:
	int customer_id;
	int order_number;
	int request_type;

public:
	CustomerRequest();
	void operator = (const CustomerRequest &order) {
		customer_id = order.customer_id;
		order_number = order.order_number;
		request_type = order.request_type;
	}
	void SetOrder(int cid, int order_num, int type);
	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class RobotInfo {
private:
	int customer_id;
	int order_number;
	int request_type;
	int engineer_id;
	int admin_id;

public:
	RobotInfo();
	void operator = (const RobotInfo &info) {
		customer_id = info.customer_id;
		order_number = info.order_number;
		request_type = info.request_type;
		engineer_id = info.engineer_id;
		admin_id = info.admin_id;
	}
	void SetInfo(int cid, int order_num, int type, int engid, int expid);
	void CopyOrder(CustomerRequest order);
	void SetEngineerId(int id);
	void SetExpertId(int id);

	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();
	int GetEngineerId();
	int GetExpertId();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};


class CustomerRecord {
private:
	int customer_id;
	int last_order;

public:
	CustomerRecord();

	int GetCustomerId();
	int GetLastOrder();

	void SetCustomerId(int id);
	void SetLastOrder(int order);

	int Size();
	
	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
};



#endif // #ifndef __MESSAGES_H__