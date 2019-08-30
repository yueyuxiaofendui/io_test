#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <time.h>
#include <iomanip>
#include <sys/epoll.h>
#include <unistd.h> /* for syscall() */ 
#include <sys/syscall.h> /* for __NR_* definitions */ 
#include <linux/aio_abi.h> /* for AIO types and constants */ 
#include <fcntl.h> /* O_RDWR */ 
#include <string.h> /* memset() */ 
#include <inttypes.h> /* uint64_t */ 
#include <math.h>

using namespace std;

#define MAX_THREADS 100
#define MAX_IODEPTH 1024
#define IO_RANDSEED (0xb1899bedUL)
#define RAND64_MAX (-1ULL)
#define RAND32_MAX (-1U)
#define GOLDEN_RATIO_PRIME 0x9e37fffffffc0001UL

#ifndef _OFS_IO_H
#define _OFS_IO_H
extern int unfinished_io;
extern void init_rand_seed(struct rand_state *, uint64_t );
extern uint64_t get_offset(struct aio_data *);
int rand_rw(unsigned int , struct aio_data *);
//extern uint64_t get_offset_me(struct aio_data *);
#endif

struct rand_state {
	uint64_t s1, s2, s3, s4, s5;
};
/**
struct rand88_state{
    unsigned int s1, s2, s3;
};
**/
struct thread_data{
    string disk;
    int direct;
    int iodepth;
    int iodepth_low;
    string rw;
    int rw_ratio;
    unsigned int bs;
    int filesize;
    int iosize;
    int num_threads;
    unsigned int runtime;
    int thread_id;
};

struct custom_iocb
{
    struct iocb iocb;
    int nth_request;
};

struct aio_data
{
    double io_count;
    double time_count;
    int write_count;
    int read_count;

    int numevents;
    uint64_t io_size;
    unsigned int bs_size;
    int rw_value;
    int numevents_low;
    unsigned int run_time;
    uint64_t max_blocks;
    bool is_random;

    string filename;
    string p_rw;

    int efd;
    int fd;
    int epfd;
    aio_context_t ctx;

    void *buf;
    void *aio_buf;

    struct timespec tms;
    struct io_event events[MAX_IODEPTH];
    struct custom_iocb iocbs[MAX_IODEPTH];
    struct iocb *iocbps[MAX_IODEPTH];
    struct custom_iocb *iocbp;
    struct epoll_event epevent;

    struct timespec current_time,last_time;
    struct rand_state random_state;
    struct rand_state rwmix_state;
    //struct rand88_state random_state;
}; 

extern void parse_data(thread_data *);
extern void *thread_main(void *);