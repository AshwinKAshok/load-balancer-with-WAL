#include "CommitLog.h"
#include <chrono>
#include <numeric>
#include <iostream>

CommitLog::CommitLog(){}

void CommitLog::Init(const char *filename, int is_f_flush, unsigned int batch){
    if (batch > 1){
        batch_size = batch;
    } 
    else{batch_size = 1;
    }
    outfile = fopen (filename, "a+"); 
    if (outfile == NULL) 
    { 
        perror("\nError in opening log file\n"); 
        exit (1); 
    }
    write_counter = 0;
    f_flush = is_f_flush;

}
    
void CommitLog::writelog(map_op log_entry){

    commit_log_batch.push_back(log_entry);
    
    if (commit_log_batch.size() >= batch_size){
        int size_of_batch = sizeof(map_op) * commit_log_batch.size();
        char *log_entry_bytes = new char[size_of_batch];
        memcpy((void *)log_entry_bytes,(void *)&commit_log_batch.front(), size_of_batch);
        auto t1 = std::chrono::high_resolution_clock::now();
        fwrite (log_entry_bytes, size_of_batch, 1, outfile);
        if (f_flush || batch_size > 1){
            fflush (outfile);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();
        performance_stats.push_back(duration);
        write_counter++; 
        if (ferror (outfile)){
            perror("\nError writing to log file\n"); 
            exit (1);
            
        }
        delete log_entry_bytes;
        commit_log_batch.clear();
    }    
}

void CommitLog::close(){
    fclose(outfile);
}

void CommitLog::sync(){
    fflush (outfile);
}

void CommitLog::Recover(std::vector<map_op> &smr_log, const char *filename){
    map_op log_entry;
    char *recovered_entry = new char[sizeof(log_entry)];
    int i = 0;
    while(fread(recovered_entry, sizeof(log_entry), 1, outfile)) {
        memcpy(&log_entry, recovered_entry, sizeof(log_entry));
        auto smr_log_index = smr_log.begin() + i;
        smr_log.insert(smr_log_index, log_entry);
        i++;
    }
    delete recovered_entry;
    return;
}

wal_smr_entry CommitLog::ConstructLog(map_op log_entry){
    const Bytef *crc_bytes = new Bytef[sizeof(log_entry)];
    memcpy((void*)crc_bytes, (void*)&log_entry, sizeof(log_entry));
    wal_smr_entry wal_entry;
    wal_entry.data = log_entry;
    wal_entry.length = sizeof(log_entry);
    wal_entry.cksum = crc32(0, crc_bytes, sizeof(log_entry));
    delete crc_bytes;
    return wal_entry;
}

void CommitLog::writelogwithcrc(map_op log_entry){

    wal_smr_entry wal_entry;
    wal_entry = ConstructLog(log_entry);
    commit_log_w_crc_batch.push_back(wal_entry);

    if (commit_log_w_crc_batch.size() >= batch_size){
        int size_of_batch = sizeof(wal_smr_entry) * commit_log_w_crc_batch.size();
        char *log_entry_bytes = new char[size_of_batch];
        memcpy((void*)log_entry_bytes, (void*)&commit_log_w_crc_batch.front(), size_of_batch);
        auto t1 = std::chrono::high_resolution_clock::now();
        fwrite (log_entry_bytes, size_of_batch, 1, outfile);
        if (f_flush || batch_size > 1){
            fflush (outfile);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();
        performance_stats.push_back(duration);
        commit_log_w_crc_batch.clear();
        if (ferror (outfile)){
        perror("\nError writing to log file\n"); 
        exit (1); 
        delete log_entry_bytes;
        }
    }
}

void CommitLog::Recoverwithcrc(std::vector<map_op> &smr_log, const char *filename){
    map_op log_entry;
    wal_smr_entry wal_entry;
    char *recovered_entry = new char[sizeof(wal_entry)];
    int i = 0; 
    while(fread(recovered_entry, sizeof(wal_entry), 1, outfile)) {
        memcpy(&wal_entry, recovered_entry, sizeof(wal_entry));
        log_entry = wal_entry.data;
        const Bytef *crc_bytes = new Bytef[sizeof(log_entry)];
        memcpy((void*)crc_bytes, (void*)&log_entry, sizeof(log_entry));
        if (wal_entry.cksum == crc32(0, crc_bytes, sizeof(log_entry))){
            auto smr_log_index = smr_log.begin() + i;
            smr_log.insert(smr_log_index, log_entry);
            i++;
        }
        else{
            break;
        }
    }
    delete recovered_entry;
    return;
}

void CommitLog::GetDiskThroughput(){
    long long int time_ns = std::accumulate(performance_stats.begin(), performance_stats.end(), 0LL);
    std::cout<<"Average time taken to write each record(nanoseconds): "<<time_ns/performance_stats.size()<<std::endl;

}


