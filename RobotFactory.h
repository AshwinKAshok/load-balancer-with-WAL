 #ifndef ROBOT_FACTORY_H
 #define ROBOT_FACTORY_H

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
 * Structure of state machine log entry
 */
typedef struct MapOp{
    int opcode ; // operation code : 1 - update value
    int arg1 ; // customer_id to apply the operation
    int arg2 ; // parameter for the operation
}map_op;

/***
 * Structure of customer record
 */
typedef struct cust_record{
    int customer_id ; // copied from the read request; -1 if customer_id is not found in the map
    int last_order ; // copied from the map, -1 if customer_id is not found in the map
}c_rec;


/*****
 * Structure of factory record
 */
typedef struct factory_record{
    int factory_id ; 
    char *factory_ip;
    int factory_port;
    int socket;
}factory_rec;


/****
 * structure to maintain commit
 */
typedef struct smr_last_commit{
    int last_index ; // the last index of the smr_log that has data
    int committed_index ; // the last index of the smr_log where the MapOp of the log entry 
                            // is committed and applied to the customer_record
    int primary_id ; // the production factory id ( server id ). initially set to -1.
    int factory_id ; // the id of the factory . This is assigned via the command line arguments .
}smr_lcommit;

/****
 * structure of replication request
 */
typedef struct repl_request{
    int factory_id;
    int committed_index;
    int last_index;
    map_op mapop_record;
}repl_req;



/****
 * structure of connection type
 */
typedef struct connection_type{
    int is_customer; // 0 if pfa and 1 if customer
    int get_commit_req; // only valid for pfa when is_customer=0; 0 for replication request. 1 for get commit index request.
}conn_type;



/****
 * structure of replication ack
 */
typedef struct replication_ack{
    int is_success; // 0 if no successsful and 1 if success; 2 to initiate recovery
}repl_ack;


/****
 * structure of log
 */
typedef struct wal_smr_log{
	int index;
    unsigned length;
	map_op data;
    unsigned cksum; 
}wal_smr_entry;

/****
 * structure of commited index to share information.
 */
typedef struct commit_index{
	int committed_index;
}commit_in;
#endif