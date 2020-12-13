#ifndef __COMMIT_LOG_H__
#define __COMMIT_LOG_H__

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <vector>
#include "RobotFactory.h"
#include <zlib.h>

class CommitLog{

    FILE *outfile;
    std::vector<int> performance_stats;
    int write_counter;
    int f_flush;
    unsigned int batch_size;
    std::vector<map_op> commit_log_batch;
    std::vector<wal_smr_entry> commit_log_w_crc_batch;

    public:
        CommitLog();
        void Init(const char *filename, int is_f_flush, unsigned int batch);
        void writelog(map_op log_entry);
        void writelogwithcrc(map_op log_entry);
        void close();
        void Recover(std::vector<map_op> &smr_log, const char *filename);
        void Recoverwithcrc(std::vector<map_op> &smr_log, const char *filename);
        void GetDiskThroughput();
        void sync();
        wal_smr_entry ConstructLog(map_op log_entry);
};


#endif