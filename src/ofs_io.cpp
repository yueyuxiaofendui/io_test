#include <ofs_io.h>

using namespace std; 

int unfinished_io = 0;

int main()
{
    printf("begin test--\n");
    thread_data *td= new thread_data;
    parse_data(td);
    printf("wait about %d seconds--\n", td->runtime);
    int num_jobs = td->num_threads;
    pthread_t thread[MAX_THREADS];
    for(int i = 0; i < num_jobs; i++)
    {
        td->thread_id = i + 1;
        int ret;
        ret = pthread_create(&thread[i], NULL, thread_main, td);
        if(ret)
        {
            perror("phread_create error\n");
            break;
        }
    }
    for(int i = 0; i < num_jobs; i++)
    {
        pthread_join(thread[i],NULL);
    }
    return 0;
}