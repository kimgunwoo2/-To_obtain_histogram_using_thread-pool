#include<stdio.h>

#include<string.h>

#include<stdlib.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <unistd.h>

#include <sys/time.h>

#include <semaphore.h>

#include <pthread.h>

#include <assert.h>

sem_t workerthread_set;

sem_t buffer_set;

sem_t mainthread_set;

sem_t mutex;

int histogram[256] = {0,};

int pthread_num=0,buffer_size,num_end;

int ttid=1;

int status=0;

int in=0,count=0,out=0;

int *buffer;

float average=0;

int num_start,threadpool_size;

int buffer_full(){

    if(buffer_size==count)

        return 1;

    else

        return 0;

}

int buffer_empty(){

    if(0==count)

        return 1;

    else

        return 0;

}

void push_buffer(int data_num){

   

    sem_wait(&buffer_set);

    buffer[in]=data_num;

    in=(in+1)%buffer_size;

    count++;

    sem_post(&workerthread_set);

    sem_post(&buffer_set);

   

}

int pop_buffer(){

   

    sem_wait(&buffer_set);

    int file_num;

    file_num=buffer[out];

    out=(out+1)%buffer_size;

    count--;

    sem_post(&mainthread_set);

    sem_post(&buffer_set);

    return file_num;

   

 

}

 

void* workerthread(){

    struct timeval start,end;

    char name[20] = "data";

    int bufsize;

    unsigned char buf[256];

    int res;

    float tvalue;

    int file;

    int file_num;

    int tid;

    tid=ttid++;

    res=gettimeofday(&start,NULL);

    assert(res==0);

    

    while(1){

        sem_wait(&mutex);

        pthread_num++;

        sem_post(&mutex);

        sem_wait(&workerthread_set);

        if(status==3)

            break;

        file_num=pop_buffer();

        sem_wait(&mutex);

         pthread_num--;

        sem_post(&mutex);

        sprintf(name,"data%d.bin",file_num);   

        int histogram_data[256] = {0,};

        file = open(name,O_RDONLY);

        while((bufsize=read(file,buf,256))>0){

               for(int i = 0; i < bufsize; i++)

                {

                        histogram_data[buf[i]]++;

                }

                memset(buf, 0, 256);        

     }

        

    sem_wait(&mutex);

       for(int i=0; i<256; i++){

         histogram[i]+=histogram_data[i];      

        }    

        if(file_num==num_end){   

                status=3;

       }

       sem_post(&mutex);

   }

    res=gettimeofday(&end,NULL);

    assert(res==0);

    tvalue=end.tv_sec-start.tv_sec+(end.tv_usec-start.tv_usec)/1000000.0;

    printf("%d thread worked time %.4f ms\n",tid,tvalue);

    sem_wait(&mutex); 

    average+=tvalue; 

    sem_post(&mutex);

    pthread_exit(0);

}

 

 

int main(int argc, char * argv[]){
 struct timeval start,end;
 float tvalue;
 int res;
 res=gettimeofday(&start,NULL);
 assert(res==0);
 pthread_t* thread;

 sem_init(&buffer_set,0,1);

 sem_init(&workerthread_set,0,0);

 sem_init(&mainthread_set,0,0);

 sem_init(&mutex,0,1);

 num_start=atoi(argv[1]);

 num_end=atoi(argv[2]);

if(argc==5){

    threadpool_size=atoi(argv[3]);

    buffer_size=atoi(argv[4]);

}

    else if(argc==3){

    threadpool_size=1;

    buffer_size=1;

}else if(argc==4){

    threadpool_size=atoi(argv[3]);

    buffer_size=1;

}

buffer=(int *)malloc(sizeof(int)*buffer_size);

thread=(pthread_t*)malloc(sizeof(pthread_t)*threadpool_size);

for(int i=0; i<threadpool_size; i++){

   

    pthread_create(&thread[i],NULL,workerthread,NULL);

   

}   

    for(int i=num_start; i<=num_end; i++){

        if(buffer_full()){

            sem_wait(&mainthread_set);

            i--;

        }else{

            push_buffer(i);   

        }   

     }

    while(1){

        if(status==3&&pthread_num==threadpool_size){

            for(int i=0; i<threadpool_size; i++){

                sem_post(&workerthread_set);

            }

            break;

        }

    }

    for(int i=0; i<threadpool_size; i++){

    pthread_join(thread[i],NULL);

}

  

   

    average=average/threadpool_size;

    printf("total thread worked time Average %.4f ms\n",average);

    int file_his;

    file_his= open("histogram.bin",O_RDWR);

    lseek(file_his,(off_t)0,SEEK_SET);   

    write(file_his,histogram,1024);

    close(file_his);

    res=gettimeofday(&end,NULL);

    assert(res==0);

    tvalue=end.tv_sec-start.tv_sec+(end.tv_usec-start.tv_usec)/1000000.0;

    printf("program total time %.4f ms\n",tvalue);

return 0;

       

}