#include <ofs_io.h>

#define ALIGN_SIZE  4096

using namespace std;

inline int io_setup(unsigned nr, aio_context_t *ctxp) 
{ 
    return syscall(__NR_io_setup, nr, ctxp); 
} 

inline int io_destroy(aio_context_t ctx) 
{ 
	return syscall(__NR_io_destroy, ctx); 
} 

inline int io_submit(aio_context_t ctx, long nr, struct iocb **iocbpp) 
{ 
	return syscall(__NR_io_submit, ctx, nr, iocbpp); 
}

inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr, struct io_event *events, struct timespec *timeout) 
{ 
	return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
} 

inline int eventfd2(unsigned int initval, int flags) 
{
    return syscall(__NR_eventfd2, initval, flags);
}

typedef void io_callback_t(aio_context_t ctx, struct iocb *iocb, long res, long res2);
 
void aio_callback(aio_context_t ctx, struct iocb *iocb, long res, long res2)
{
    struct custom_iocb *iocbp = (struct custom_iocb *)iocb;
    printf("nth_request: %d, request_type: %s, offset: %lld, length: %llu, res: %ld, res2: %ld\n", 
            iocbp->nth_request, (iocb->aio_lio_opcode == IOCB_CMD_PREAD) ? "READ" : "WRITE",
            iocb->aio_offset, iocb->aio_nbytes, res, res2);  
}

/**
int rand_rw(int numratio)
{
    int res = 0;
    if(numratio > (rand()%100))
        res = 1;
    return res;
}
**/

int init_io(struct aio_data *io_u, struct thread_data *data_td)
{
    io_u->numevents = data_td->iodepth;
    io_u->filename = data_td->disk + "/testfile" + to_string(data_td->thread_id);
    io_u->bs_size = (data_td->bs) * 1024;
    io_u->io_size = ((uint64_t)(data_td->iosize) * 1024 * 1024 * 1024);

    io_u->efd = eventfd2(0, O_NONBLOCK | O_CLOEXEC);
    io_u->p_rw = data_td->rw;
    io_u->rw_value = data_td->rw_ratio;
    io_u->numevents_low = data_td->iodepth_low;
    io_u->run_time = data_td->runtime;
    io_u->max_blocks = (uint64_t)((data_td->filesize * 1024 * 1024)/(data_td->bs)); 

    io_u->io_count = 0;
    io_u->time_count = 0;
    io_u->write_count = 0;
    io_u->read_count = 0;

    uint64_t _seed;
    _seed = IO_RANDSEED * data_td->thread_id;
    init_rand_seed(&io_u->random_state, _seed);
    init_rand_seed(&io_u->rwmix_state, _seed + 1);

    if(data_td->rw == "write" || data_td->rw == "read" || data_td->rw == "rw")
    {
        io_u->is_random = false;
    }
    else if(data_td->rw == "randwrite" || data_td->rw == "randread" || data_td->rw == "randrw")
    {
        io_u->is_random = true;
    }
    else
    {
        perror("paragram error ");
        return 0;
    }
    
    if(io_u->efd == -1)
    {
        perror("eventfd2 ");
        return 0;
    }
    
    if(data_td->direct == 1)
    {
        io_u->fd = open(io_u->filename.c_str(), O_RDWR | O_CREAT | O_DIRECT | O_SYNC, 0644);
    }
    else
    {
        io_u->fd = open(io_u->filename.c_str(), O_RDWR | O_CREAT | O_SYNC, 0644);
    }
    if (io_u->fd == -1) {
        perror("open ");
        return 0;
    }
    ftruncate(io_u->fd, (data_td->filesize)*1024*1024*1024);//指定文件大小
    memset(&io_u->ctx, 0, sizeof(io_u->ctx));
    if (io_setup(io_u->numevents, &io_u->ctx)) {
        perror("io_setup ");
        return 0;
    }
    return 1;
}

int io_queue(struct aio_data *io_u)
{   
    struct custom_iocb *iocbs = io_u->iocbs;
    struct iocb **iocbps = io_u->iocbps;
    struct custom_iocb *iocbp = io_u->iocbp;
    void *do_buf = io_u->buf;
    void *do_aio_buf = io_u->aio_buf;
    if (posix_memalign(&do_buf, ALIGN_SIZE, io_u->bs_size * io_u->numevents)) {
        perror("posix_memalign");
        return 0;
    }
    int i;
    //printf("buf: %p\n", buf);
    for (i = 0, iocbp = iocbs; i < io_u->numevents; ++i, ++iocbp) {
        do_aio_buf = (void *)((char *)do_buf + (i * (io_u->bs_size)));
        memset(do_aio_buf, 0, io_u->bs_size);
        iocbp->iocb.aio_fildes = io_u->fd;
        if(io_u->p_rw == "read")
        {
            iocbp->iocb.aio_lio_opcode = IOCB_CMD_PREAD;
            io_u->read_count++;
        }
        else if(io_u->p_rw == "write")
        {
            iocbp->iocb.aio_lio_opcode = IOCB_CMD_PWRITE;
            io_u->write_count++;
        }
        else
        {
            if(rand_rw(io_u->rw_value, io_u) == 1)//(read_count < write_count * ((double)data_fd->rw_ratio/(double)(100 - data_fd->rw_ratio)))
            {
                iocbp->iocb.aio_lio_opcode = IOCB_CMD_PREAD;
                io_u->read_count++;
            }
            else
            {
                iocbp->iocb.aio_lio_opcode = IOCB_CMD_PWRITE;
                io_u->write_count++;
            }     
        } 
        iocbp->iocb.aio_buf = (uint64_t)do_aio_buf;
        iocbp->iocb.aio_offset = (io_u->io_count + i) * io_u->bs_size;
        iocbp->iocb.aio_nbytes = io_u->bs_size;
        iocbp->iocb.aio_flags = IOCB_FLAG_RESFD;
        iocbp->iocb.aio_resfd = io_u->efd;
         
        //io_set_callback(&iocbp->iocb, aio_callback);
        iocbp->iocb.aio_data = (__u64)aio_callback;
 
        iocbp->nth_request = io_u->io_count + i + 1;
        iocbps[i] = &iocbp->iocb;
    }
    return 1;
}

int io_rand_queue(struct aio_data *io_u)
{
    struct custom_iocb *iocbs = io_u->iocbs;
    struct iocb **iocbps = io_u->iocbps;
    struct custom_iocb *iocbp = io_u->iocbp;
    void *do_buf = io_u->buf;
    void *do_aio_buf = io_u->aio_buf;
    if (posix_memalign(&do_buf, ALIGN_SIZE, io_u->bs_size * io_u->numevents)) {
        perror("posix_memalign");
        return 0;
    }
    int i;
    //printf("buf: %p\n", buf);

    for (i = 0, iocbp = iocbs; i < io_u->numevents; ++i, ++iocbp) {
        uint64_t offset = get_offset(io_u);
        do_aio_buf = (void *)((char *)do_buf + (i * (io_u->bs_size)));
        memset(do_aio_buf, 0, io_u->bs_size);
        iocbp->iocb.aio_fildes = io_u->fd;
        if(io_u->p_rw == "randread")
        {
            iocbp->iocb.aio_lio_opcode = IOCB_CMD_PREAD;
            io_u->read_count++;
        }
        else if(io_u->p_rw == "randwrite")
        {
            iocbp->iocb.aio_lio_opcode = IOCB_CMD_PWRITE;
            io_u->write_count++;
        }
        else
        {
            if(rand_rw(io_u->rw_value, io_u) == 1)//(read_count < write_count * ((double)data_fd->rw_ratio/(double)(100 - data_fd->rw_ratio)))
            {
                iocbp->iocb.aio_lio_opcode = IOCB_CMD_PREAD;
                io_u->read_count++;
            }
            else
            {
                iocbp->iocb.aio_lio_opcode = IOCB_CMD_PWRITE;
                io_u->write_count++;
            }    
        } 
        iocbp->iocb.aio_buf = (uint64_t)do_aio_buf;
        iocbp->iocb.aio_offset = offset;
        iocbp->iocb.aio_nbytes = io_u->bs_size;
        iocbp->iocb.aio_flags = IOCB_FLAG_RESFD;
        iocbp->iocb.aio_resfd = io_u->efd;
         
        //io_set_callback(&iocbp->iocb, aio_callback);
        iocbp->iocb.aio_data = (__u64)aio_callback;
 
        iocbp->nth_request = io_u->io_count + i + 1;
        iocbps[i] = &iocbp->iocb;
    }
    return 1;
}

int do_io_submit(struct aio_data *io_u)
{
    if(io_u->is_random)
    {
        cout<<"do test: "<< io_u->p_rw << endl;
        if(!io_rand_queue(io_u))
        {
            perror("io_rand_queue ");
            return 0;
        }
    }
    else
    {
        cout<<"do test: "<< io_u->p_rw << endl;
        if(!io_queue(io_u))
        {
            perror("io_queue ");
            return 0;
        }
    }  

    clock_gettime(CLOCK_REALTIME, &io_u->last_time);
    if (io_submit(io_u->ctx, io_u->numevents, io_u->iocbps) != io_u->numevents) {
        perror("io_submit ");
        return 0;
    }

    io_u->epfd = epoll_create(1);
    if (io_u->epfd == -1) {
        perror("epoll_create ");
        return 0;
    }
 
    io_u->epevent.events = EPOLLIN | EPOLLET;
    io_u->epevent.data.ptr = NULL;
    if (epoll_ctl(io_u->epfd, EPOLL_CTL_ADD, io_u->efd, &io_u->epevent)) {
        perror("epoll_ctl ");
        return 0;
    }

    return 1;
}

int do_io_getevents(struct aio_data *io_u)
{
    while(1)
    {
        if (epoll_wait(io_u->epfd, &io_u->epevent, 1, -1) != 1) {
            perror("epoll_wait ");
            return 0;
        } 
        int num_io = 0;
        int num_events = io_u->numevents;
        while(1)
        {
            io_u->tms.tv_sec = 0;
            io_u->tms.tv_nsec = 0;
            int ret = io_getevents(io_u->ctx, 1, num_events, io_u->events, &io_u->tms);              
            if (ret > 0) {
                num_io += ret;
            }    
            unfinished_io = (num_events - ret);
            num_events = unfinished_io;
            if(unfinished_io < io_u->numevents_low)
            {
                break;
            }
        }
        clock_gettime(CLOCK_REALTIME, &io_u->current_time);
        io_u->io_count += num_io;
        /**
        if(num_io > 0)
        {
            for (int i = 0; i < num_io; ++i) {
                    ((io_callback_t *)(io_u->events[i].data))(io_u->ctx, (struct iocb *)io_u->events[i].obj, io_u->events[i].res, io_u->events[i].res2);
                }
        }
        **/
        io_u->time_count = ((io_u->current_time.tv_sec - io_u->last_time.tv_sec) + (io_u->current_time.tv_nsec - io_u->last_time.tv_nsec)*pow(10,-9));
        //cout<<"timecount: "<< io_u->time_count << endl;
        //cout<<"num_io: " << num_io <<endl;
        //cout<<"unfinished: "<<unfinished_io<<endl;
        if(io_u->time_count > (io_u->run_time) || (io_u->bs_size * io_u->io_count) > (io_u->io_size))
        {
            break;
        }

        if(io_u->is_random)
        {
            if(!io_rand_queue(io_u))
            {
                perror("io_rand_queue ");
                return 0;
            }
        }
        else
        {
            if(!io_queue(io_u))
            {
                perror("io_queue ");
                return 0;
            }
        }  
           
        if (io_submit(io_u->ctx, num_io, io_u->iocbps) != num_io) {
            perror("io_submit ");
            return 0;
        }
    }
    return 1;
}

void do_print(struct aio_data *io_u)
{
    cout<<"io_count: "<< io_u->io_count << endl;
    cout<<"time_count: " << io_u->time_count <<endl;
    cout<<"IOPS: "<<(io_u->io_count / io_u->time_count) << "     io: " << ((double)(io_u->io_count * (io_u->bs_size / 1024)) / (double)1024) << "MB"<< endl;
    cout<<"bw: "<<(io_u->io_count / io_u->time_count) * (io_u->bs_size / 1024) / (double)1024<< "MB/s     " << endl;
    cout<<"write_count: "<< io_u->write_count << "  read_count: "<< io_u->read_count << endl;
    cout<<"wait for remove testfile ~_~ "<<endl;
    close(io_u->epfd);
    free(io_u->buf);
    io_destroy(io_u->ctx);
    close(io_u->fd);
    close(io_u->efd);
    remove(io_u->filename.c_str());
}

void *thread_main(void *options)
{
    srand((unsigned)time(NULL));
    struct thread_data *data_td = (struct thread_data*)options;
    struct aio_data *io_u = new aio_data; 
    if(!init_io(io_u, data_td))
    {
        perror("init error");
        return (void*)1;
    }

    if(!do_io_submit(io_u))
    {
        perror("do_io_submit error");
        return (void*)2;
    }

    if(!do_io_getevents(io_u))
    {
        perror("do_io_getevents error");
        return (void*)3;
    }
    do_print(io_u);
    delete(io_u);
    return (void*)0;
}
