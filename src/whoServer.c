#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netdb.h>      /* gethostbyaddr */
#include <sys/wait.h>   /* sockets */
#include <sys/types.h>  /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include "../structs/summary_list.h"

long int datetoint(const char *s);

#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

void perror_exit(char *message);

int size_of_buf;

typedef struct
{
    //Or whatever information that you need
    int *port;
    int *type; //0 an einai apo to summary_port, 1 apo to query_port
} port_reader;

typedef struct //cycliko buffer
{
    int *data;
    int *type; //0 an einai apo to summary_port, 1 apo to query_port
    int start; //poy tha ginei h eisagwgh
    int end;   //apo pou tha vgainoun stoixeia
    int count;
} c_buffer;

int *worker_port_number_array;
int num_of_workers = 0;
pthread_mutex_t lock;

char workers_host_name[100];
char client_host_name[100];

pthread_mutex_t mtx; //mutex gia diavasma kai grapsimo sto buffer
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
c_buffer buffer;

int run = 1;

void intHandler()
{
    printf("\nsigint\n");
    run = 0;
}

void initialize(c_buffer *buffer, int size_of_buf)
{
    buffer->data = malloc(size_of_buf * sizeof(int));
    buffer->type = malloc(size_of_buf * sizeof(int));
    buffer->start = 0;
    buffer->end = 0;
    buffer->count = 0;
}

void place(c_buffer *buffer, int data, int type) //eisagei ta stoixeia sto c_buffer
{
    pthread_mutex_lock(&mtx);
    while (buffer->count >= size_of_buf)
    {
        printf(">> Found Buffer Full \n");
        pthread_cond_wait(&cond_nonfull, &mtx); //an einai gemato perimenei signal oti kapoio allo thread diavase apo to buffer
    }
    buffer->data[buffer->start] = data;
    buffer->type[buffer->start] = type;

    buffer->start++;
    buffer->count++;
    if (buffer->start == size_of_buf)
    {
        buffer->start = 0;
    }

    pthread_mutex_unlock(&mtx);
}

int obtain(c_buffer *buffer, int *type) //epistrefei to teleytaio stoixeio toy buffer
{
    pthread_mutex_lock(&mtx);
    while (buffer->count <= 0)
    {
        printf(">> Found Buffer Empty \n");
        pthread_cond_wait(&cond_nonempty, &mtx); //an einai adeio to buffer blockarei mexri kapoiow writer na grapsei kapoio file descriptor
    }
    int data = buffer->data[buffer->end];
    // int a=buffer->type[buffer->end];
    (*type) = buffer->type[buffer->end];
    printf("thread %ld read %d with type %d from c_buffer[%d]\n", pthread_self(), data, buffer->type[buffer->end], buffer->end);
    buffer->count--;
    buffer->end++;
    if (buffer->end == size_of_buf)
    {
        buffer->end = 0;
    }
    pthread_mutex_unlock(&mtx);
    return data;
}

void *consumer(void *ptr) //thread pou diavasei apo to cycliko buffer
{
    while (1)
    {
        int type = 0;
        int *p = &type;
        int socket_ = obtain(&buffer, p); //diabazei to file descriptor apo to buffer
        printf("consumer read: %d of type %d\n", socket_, type);

        // char buf[200];
        if (type == 0) // an diavase file descriptor pou anhkei se worker
        {
            int worker_port;
            read(socket_, &worker_port, sizeof(int)); //diavazei to port poy epikoinwnei o worker

            char host[100];
            memset(host, '\0', sizeof(host)); //diavazei to IP address poy epikoinwnei o worker
            read(socket_, host, 100);

            pthread_mutex_lock(&lock);

            num_of_workers++;
            if (num_of_workers == 1)
            {
                worker_port_number_array = (int *)malloc(sizeof(int) * num_of_workers);
                strcpy(workers_host_name, host);
            }
            else
            {
                worker_port_number_array = (int *)realloc(worker_port_number_array, sizeof(int) * num_of_workers);
            }

            worker_port_number_array[num_of_workers - 1] = worker_port;

            pthread_mutex_unlock(&lock);

            int num_of_sums;
            read(socket_, &num_of_sums, sizeof(int)); // arithmos sum pou tha diavasei

            printf("read port_number %d with IP %s, %d workers connected\n", worker_port, workers_host_name, num_of_workers); /* Print received char */
            for (int i = 0; i < num_of_workers; i++)
            {
                printf("%d ", worker_port_number_array[i]);
            }
            printf("\n");

            for (int i = 0; i < num_of_sums; i++) //diavazei ta sum kai ta emfanizei
            {
                char *buf = malloc(200 * sizeof(char));
                memset(buf, '\0', sizeof(buf));

                read(socket_, buf, 200);
                printf("%s", buf); /* Print received char */

                free(buf);
            }
        }
        else //an type==1 kai diavazei apo ton client
        {

            char buf[256], command[256];
            memset(buf, '\0', sizeof(buf));
            memset(command, '\0', sizeof(command));
            read(socket_, buf, 256);

            strcpy(command, buf); //diavazei to query

            printf("%s", buf); /* Print received char */

            char *c1 = strtok(command, " \0");

            if (strcmp(c1, "/diseaseFrequency") == 0)
            {
                int count = 0;
                for (int j = 0; j < num_of_workers; j++) //anoigei epikoinwnia me tous worker
                {
                    //printf("%d. %d ", j, worker_port_number_array[j]);
                    // write_to_worker(worker_port_number_array[i], buf);
                    int port, sock_read, i;
                    // char query[256];
                    struct sockaddr_in server;
                    struct sockaddr *serverptr = (struct sockaddr *)&server;
                    struct hostent *rem;

                    /* Create socket */
                    if ((sock_read = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                        perror_exit("socket");
                    /* Find server address */
                    if ((rem = gethostbyname(workers_host_name)) == NULL)
                    {
                        herror("gethostbyname");
                        exit(1);
                    }
                    port = worker_port_number_array[j]; /*Convert port number to integer*/
                    server.sin_family = AF_INET;        /* Internet domain */
                    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
                    server.sin_port = htons(port); /* Server port */
                    /* Initiate connection */
                    if (connect(sock_read, serverptr, sizeof(server)) < 0)
                        perror_exit("connect");

                    if (write(sock_read, buf, 256) < 0)
                        perror_exit("write");

                    printf("write to port %d\n", worker_port_number_array[j]);

                    int temp_count;
                    if (read(sock_read, &temp_count, sizeof(int)) < 0) //diavazei poses eggrafes vrhke o kathenaw kai tis athroizei
                        perror_exit("read");

                    count = count + temp_count;

                    close(sock_read);
                }

                if (write(socket_, &count, sizeof(int)) < 0) //grafei sto client ton arithmo ton krousmatwn
                    perror_exit("write");
            }
            else if (strcmp(c1, "/topk-AgeRanges") == 0)
            {
                char *c2 = strtok(NULL, " \0");
                char *c3 = strtok(NULL, " \0");
                char *c4 = strtok(NULL, " \0");
                char *c5 = strtok(NULL, " \0");
                char *c6 = strtok(NULL, " \n\0");
                int k = atoi(c2);
                if (k > 4)
                {
                    k = 4;
                }
                float total = 0;
                float zero = 0;
                float twenty = 0;
                float forty = 0;
                float sixty = 0;

                for (int j = 0; j < num_of_workers; j++) //anoigei epikoinwnia me tous worker
                {
                    //printf("%d. %d ", j, worker_port_number_array[j]);
                    // write_to_worker(worker_port_number_array[i], buf);
                    int port, sock_read, i;
                    // char query[256];
                    struct sockaddr_in server;
                    struct sockaddr *serverptr = (struct sockaddr *)&server;
                    struct hostent *rem;

                    /* Create socket */
                    if ((sock_read = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                        perror_exit("socket");
                    /* Find server address */
                    if ((rem = gethostbyname(workers_host_name)) == NULL)
                    {
                        herror("gethostbyname");
                        exit(1);
                    }
                    port = worker_port_number_array[j]; /*Convert port number to integer*/
                    server.sin_family = AF_INET;        /* Internet domain */
                    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
                    server.sin_port = htons(port); /* Server port */
                    /* Initiate connection */
                    if (connect(sock_read, serverptr, sizeof(server)) < 0)
                        perror_exit("connect");

                    if (write(sock_read, buf, 256) < 0)
                        perror_exit("write");

                    printf("write to port %d\n", worker_port_number_array[j]);

                    int temp;

                    if (read(sock_read, &temp, sizeof(int)) < 0) //diavazei an o worker eksyphretei ayth th xwra
                        perror_exit("read");

                    if (temp == 1) //an nai diabazei kroysmata ana kathgoria kai synoliko arithmo
                    {
                        if (read(sock_read, &zero, sizeof(float)) < 0)
                            perror_exit("read");
                        if (read(sock_read, &twenty, sizeof(float)) < 0)
                            perror_exit("read");
                        if (read(sock_read, &forty, sizeof(float)) < 0)
                            perror_exit("read");
                        if (read(sock_read, &sixty, sizeof(float)) < 0)
                            perror_exit("read");
                        if (read(sock_read, &total, sizeof(float)) < 0)
                            perror_exit("read");
                    }

                    close(sock_read);
                }

                if (total == 0)
                {
                    total = 1;
                }

                for (int t = 0; t < k; t++)
                {
                    float z_per = zero / total;
                    float t_per = twenty / total;
                    float f_per = forty / total;
                    float s_per = sixty / total;

                    char answer[200];
                    memset(answer, '\0', sizeof(answer));

                    if (z_per >= t_per && z_per >= f_per && z_per >= s_per)
                    {
                        sprintf(answer, "0-20: %.1f %%", 100 * z_per);
                        zero = -1;
                    }
                    else if (t_per >= z_per && t_per >= f_per && t_per >= s_per)
                    {
                        sprintf(answer, "20-40: %.1f %%", 100 * t_per);
                        twenty = -1;
                    }
                    else if (f_per >= t_per && f_per >= z_per && f_per >= s_per)
                    {
                        sprintf(answer, "40-60: %.1f %%", 100 * f_per);
                        forty = -1;
                    }
                    else if (s_per >= t_per && s_per >= f_per && s_per >= z_per)
                    {
                        sprintf(answer, "60+: %.1f %%", 100 * s_per);
                        sixty = -1;
                    }

                    printf("%s\n", answer);

                    if (write(socket_, answer, 200) < 0) //grafei k apanthseis sto client, o opoios vash ylopoihshs perimenei toses
                        perror_exit("write");
                }

                // pthread_mutex_unlock(&lock);
            }
            else if (strcmp(c1, "/searchPatientRecord") == 0)
            {
                char answer[200];
                memset(answer, '\0', sizeof(answer));
                int found = 0;

                for (int j = 0; j < num_of_workers; j++)
                {
                    //printf("%d. %d ", j, worker_port_number_array[j]);
                    // write_to_worker(worker_port_number_array[i], buf);
                    int port, sock_read, i;
                    // char query[256];
                    struct sockaddr_in server;
                    struct sockaddr *serverptr = (struct sockaddr *)&server;
                    struct hostent *rem;

                    /* Create socket */
                    if ((sock_read = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                        perror_exit("socket");
                    /* Find server address */
                    if ((rem = gethostbyname(workers_host_name)) == NULL)
                    {
                        herror("gethostbyname");
                        exit(1);
                    }
                    port = worker_port_number_array[j]; /*Convert port number to integer*/
                    server.sin_family = AF_INET;        /* Internet domain */
                    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
                    server.sin_port = htons(port); /* Server port */
                    /* Initiate connection */
                    if (connect(sock_read, serverptr, sizeof(server)) < 0)
                        perror_exit("connect");

                    if (write(sock_read, buf, 256) < 0)
                        perror_exit("write");

                    printf("write to port %d\n", worker_port_number_array[j]);

                    int temp;

                    if (read(sock_read, &temp, sizeof(int)) < 0) //an vrethei tote o worker epistrefei 1, alliws 0
                        perror_exit("read");

                    if (temp == 1) //an einai 1 diavazei thn eggrafh
                    {
                        found = 1;
                        if (read(sock_read, answer, 200) < 0)
                            perror_exit("read");
                    }

                    close(sock_read);
                }

                if (found == 0) //an de brethike h eggrafh stelnei katallhlo mynhma
                    sprintf(answer, "Record Not Found");

                printf("%s\n", answer);

                if (write(socket_, answer, 200) < 0) //grafei thn apanthsh
                    perror_exit("write");
            }
            else if (strcmp(c1, "/numPatientAdmissions") == 0)
            {
                struct answer *ans_to_ret = NULL;
                int num_of_ans = 0;
                char answer[200];
                memset(answer, '\0', sizeof(answer));

                char *c2 = strtok(NULL, " \0");
                char *c3 = strtok(NULL, " \0");
                char *c4 = strtok(NULL, " \0");
                char *c5 = strtok(NULL, " \n\0");
                int found = 0;
                for (int j = 0; j < num_of_workers; j++)
                {
                    //printf("%d. %d ", j, worker_port_number_array[j]);
                    // write_to_worker(worker_port_number_array[i], buf);
                    int port, sock_read, i;
                    // char query[256];
                    struct sockaddr_in server;
                    struct sockaddr *serverptr = (struct sockaddr *)&server;
                    struct hostent *rem;

                    /* Create socket */
                    if ((sock_read = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                        perror_exit("socket");
                    /* Find server address */
                    if ((rem = gethostbyname(workers_host_name)) == NULL)
                    {
                        herror("gethostbyname");
                        exit(1);
                    }
                    port = worker_port_number_array[j]; /*Convert port number to integer*/
                    server.sin_family = AF_INET;        /* Internet domain */
                    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
                    server.sin_port = htons(port); /* Server port */
                    /* Initiate connection */
                    if (connect(sock_read, serverptr, sizeof(server)) < 0)
                        perror_exit("connect");

                    if (write(sock_read, buf, 256) < 0)
                        perror_exit("write");

                    printf("write to port %d\n", worker_port_number_array[j]);

                    if (c5 == NULL)
                    {
                        int temp;

                        found = 1;

                        if (read(sock_read, &temp, sizeof(int)) < 0) //an den dothike xwra diavazei poses xwres eksyphretei o worker
                            perror_exit("read");

                        char answer[200];
                        memset(answer, '\0', sizeof(answer));

                        for (int t = 0; t < temp; t++)
                        {
                            if (read(sock_read, answer, 200) < 0) //diavazei tis apanthseis twn worker kai ta stelnei sto client
                                perror_exit("read");

                            printf("%s\n", answer);
                            if (write(socket_, answer, 200) < 0)
                                perror_exit("write");
                        }

                        num_of_ans = num_of_ans + temp;
                    }
                    else
                    {
                        int temp = 0;
                        if (read(sock_read, &temp, sizeof(int)) < 0) //an o worker eksyphretei to country stelnei 1
                            perror_exit("read");

                        if (temp == 1)
                        {
                            found = 1;

                            if (read(sock_read, answer, 200) < 0) //diabazei thn apanthsh
                                perror_exit("read");

                            printf("%s\n", answer);

                            if (write(socket_, answer, 200) < 0)
                                perror_exit("write");
                        }
                        // if (write(socket_, &num_of_ans, sizeof(int) < 0))
                        //     perror_exit("write");
                    }

                    close(sock_read);
                }
                if (found == 0) //an den brike kanenas worker to country
                {
                    char answer_no_c[200];
                    memset(answer_no_c, '\0', sizeof(answer_no_c));
                    strcpy(answer_no_c, "Country Not Found");
                    if (write(socket_, answer_no_c, 200) < 0)
                        perror_exit("write");
                }
                char answer_end[200];
                memset(answer_end, '\0', sizeof(answer_end));
                strcpy(answer_end, "end_of_countries");
                if (write(socket_, answer_end, 200) < 0) //stelnei mynhma "end of countries" gia na kserei o client oti den exei na emfanisei alla mynhmata
                    perror_exit("write");
            }
            else if (strcmp(c1, "/numPatientDischarges") == 0)
            {
                struct answer *ans_to_ret = NULL;
                int num_of_ans = 0;
                char answer[200];
                memset(answer, '\0', sizeof(answer));

                char *c2 = strtok(NULL, " \0");
                char *c3 = strtok(NULL, " \0");
                char *c4 = strtok(NULL, " \0");
                char *c5 = strtok(NULL, " \n\0");
                int found = 0;
                for (int j = 0; j < num_of_workers; j++)
                {
                    //printf("%d. %d ", j, worker_port_number_array[j]);
                    // write_to_worker(worker_port_number_array[i], buf);
                    int port, sock_read, i;
                    // char query[256];
                    struct sockaddr_in server;
                    struct sockaddr *serverptr = (struct sockaddr *)&server;
                    struct hostent *rem;

                    /* Create socket */
                    if ((sock_read = socket(PF_INET, SOCK_STREAM, 0)) < 0)
                        perror_exit("socket");
                    /* Find server address */
                    if ((rem = gethostbyname(workers_host_name)) == NULL)
                    {
                        herror("gethostbyname");
                        exit(1);
                    }
                    port = worker_port_number_array[j]; /*Convert port number to integer*/
                    server.sin_family = AF_INET;        /* Internet domain */
                    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
                    server.sin_port = htons(port); /* Server port */
                    /* Initiate connection */
                    if (connect(sock_read, serverptr, sizeof(server)) < 0)
                        perror_exit("connect");

                    if (write(sock_read, buf, 256) < 0)
                        perror_exit("write");

                    printf("write to port %d\n", worker_port_number_array[j]);

                    if (c5 == NULL)
                    {
                        int temp;

                        found = 1;

                        if (read(sock_read, &temp, sizeof(int)) < 0)
                            perror_exit("read");

                        char answer[200];
                        memset(answer, '\0', sizeof(answer));

                        for (int t = 0; t < temp; t++)
                        {
                            if (read(sock_read, answer, 200) < 0)
                                perror_exit("read");

                            printf("%s\n", answer);
                            if (write(socket_, answer, 200) < 0)
                                perror_exit("write");
                        }

                        num_of_ans = num_of_ans + temp;
                    }
                    else
                    {
                        int temp = 0;
                        if (read(sock_read, &temp, sizeof(int)) < 0)
                            perror_exit("read");

                        if (temp == 1)
                        {
                            found = 1;

                            if (read(sock_read, answer, 200) < 0)
                                perror_exit("read");

                            printf("%s\n", answer);

                            if (write(socket_, answer, 200) < 0)
                                perror_exit("write");
                        }
                        // if (write(socket_, &num_of_ans, sizeof(int) < 0))
                        //     perror_exit("write");
                    }

                    close(sock_read);
                }
                if (found == 0)
                {
                    char answer_no_c[200];
                    memset(answer_no_c, '\0', sizeof(answer_no_c));
                    strcpy(answer_no_c, "Country Not Found");
                    if (write(socket_, answer_no_c, 200) < 0)
                        perror_exit("write");
                }
                char answer_end[200];
                memset(answer_end, '\0', sizeof(answer_end));
                strcpy(answer_end, "end_of_countries");
                if (write(socket_, answer_end, 200) < 0)
                    perror_exit("write");
            }
        }
        printf("Closing connection.\n");
        close(socket_); //kleinei th syndesh me ton client

        pthread_cond_signal(&cond_nonfull); //stelnei signal oti adeiase thesi sto buffer
        // usleep(500000);
    }
    pthread_exit(0);
}

void *writer(void *args) //diabazei to port apo opou tha diabazei kai otan kanei accept grafei to fd sto buffer
{
    port_reader *actual_args = args;
    int sock, newsock;
    int port = *(actual_args->port);
    int type = *(actual_args->type); //an diabazei apo to query port 'H to summary port
    struct sockaddr_in server, client;
    socklen_t clientlen;
    struct sockaddr *serverptr = (struct sockaddr *)&server;
    struct sockaddr *clientptr = (struct sockaddr *)&client;
    struct hostent *rem;

    int rc;
    int on = 1;
    int max_sd, new_sd;

    struct timeval timeout;
    fd_set master_set, working_set;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    server.sin_family = AF_INET; /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port); /* The given port */

    if (ioctl(sock, FIONBIO, (char *)&on) < 0)
        perror_exit("ioctl");

    /* Bind socket to address */
    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror_exit("bind");
    /* Listen for connections */
    if (listen(sock, 5) < 0)
        perror_exit("listen");
    printf("Listening in port %d\n", port);

    FD_ZERO(&master_set);
    max_sd = sock;
    FD_SET(sock, &master_set);

    while (run)
    {
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        memcpy(&working_set, &master_set, sizeof(master_set));

        rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout); //an den labei mynhma sto socket se 3 deytera synexizei, wste na elegxei an exei lyfthei kapoio sigint 
        if (rc > 0)
        {

            clientlen = sizeof(client);

            if ((newsock = accept(sock, clientptr, &clientlen)) < 0)
                perror_exit("accept");

            place(&buffer, newsock, type);  //eisagei sto buffer

            printf("producer write: %d\n", newsock);

            pthread_cond_signal(&cond_nonempty); //sima oti den einai adeio to buffer
        } 
    }
    // printf("oooooooo\n");
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    char queryPortNumber[20];
    char statisticsPortNumber[20];
    int buffersize;
    int numThreads;
    int red = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-q") == 0)
        {
            strcpy(queryPortNumber, argv[i + 1]);
            // printf("input = %s\n", folder1);
            red++;
        }
        else if (strcmp(argv[i], "-w") == 0)
        {

            numThreads = atoi(argv[i + 1]);
            // printf("num wo=%d\n", num_workers);
            red++;
        }
        else if (strcmp(argv[i], "-b") == 0)
        {

            buffersize = atoi(argv[i + 1]);
            // printf("buffersize = %d\n", buffersize);
            red++;
        }
        else if (strcmp(argv[i], "-s") == 0)
        {

            strcpy(statisticsPortNumber, argv[i + 1]);
            red++;
        }
    }

    if (red != 4)
    {
        printf("./whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize\n");
        return -1;
    }

    int port = atoi(statisticsPortNumber);
    int type = 0;
    int port_q = atoi(queryPortNumber);
    int type_q = 1;

    pthread_t cons[numThreads], writer1, writer2;
    size_of_buf = buffersize;

    // signal(SIGINT,intHandler);
    signal(SIGINT, intHandler);

    initialize(&buffer, size_of_buf);
    pthread_mutex_init(&mtx, 0);
    pthread_mutex_init(&lock, 0);
    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&cond_nonfull, 0);

    for (int i = 0; i < numThreads; i++)        //dhmiourgei numThread pou tha diabazoyn apo to buffer
    {
        pthread_create(&cons[i], 0, consumer, 0);
    }

    port_reader *args = malloc(sizeof *args);
    args->port = &port;
    args->type = &type;
    pthread_create(&writer1, NULL, writer, args); //dhmiourgei ena thread pou tha diabazei apo to summary port

    port_reader *args2 = malloc(sizeof *args2);
    args2->port = &port_q;
    args2->type = &type_q;
    pthread_create(&writer2, NULL, writer, args2); //dhmiourgei ena thread pou tha diabazei apo to query port

    pthread_join(writer1, 0); //perimenie toys writer na epistrepsoun meta apo kapoio sigint
    pthread_join(writer2, 0);
    for (int i = 0; i < numThreads; i++)
    {
        pthread_cancel(cons[i]); //akyrwnei ta numThreads
        // pthread_join(cons[i], 0);
    }

    for (int j = 0; j < num_of_workers; j++)
    {
        // printf("%d. %d ", j, worker_port_number_array[j]);
        // write_to_worker(worker_port_number_array[i], buf);
        int port, sock_read, i;
        // char query[256];
        struct sockaddr_in server;
        struct sockaddr *serverptr = (struct sockaddr *)&server;
        struct hostent *rem;

        /* Create socket */
        if ((sock_read = socket(PF_INET, SOCK_STREAM, 0)) < 0)
            perror_exit("socket");
        /* Find server address */
        if ((rem = gethostbyname(workers_host_name)) == NULL)
        {
            herror("gethostbyname");
            exit(1);
        }
        port = worker_port_number_array[j]; /*Convert port number to integer*/
        server.sin_family = AF_INET;        /* Internet domain */
        memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
        server.sin_port = htons(port); /* Server port */
        /* Initiate connection */
        if (connect(sock_read, serverptr, sizeof(server)) < 0)
            perror_exit("connect");

        char ex_buf[256];

        memset(ex_buf, '\0', sizeof(ex_buf));

        strcpy(ex_buf, "/exit");
        if (write(sock_read, ex_buf, 256) < 0) //grefei se kathe worker ena mynhma /exit gia na kserei oti prepei na termatisei
            perror_exit("write");

        close(sock_read);
    }


    free(args);
    free(args2);
    free(buffer.data);
    free(buffer.type);
    free(worker_port_number_array);
    pthread_cond_destroy(&cond_nonempty);
    pthread_cond_destroy(&cond_nonfull);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&lock);

    return 0;
}

void perror_exit(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

long int datetoint(const char *s) //metatrepei kathe hmeromhnia se enan long int
{
    long int inDate = 0;
    char *temp1 = malloc(strlen(s) + 1);
    strcpy(temp1, s);
    char *temp;

    temp = strtok(temp1, "-");
    inDate = atoi(temp);
    temp = strtok(NULL, "-");
    inDate = inDate + 30 * atoi(temp);
    temp = strtok(NULL, "\0");

    inDate = inDate + 30 * 12 * atoi(temp);

    free(temp1);

    return inDate;
}