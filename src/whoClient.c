/* inet_str_client.c: Internet stream sockets client */
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>  /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <unistd.h>     /* read, write, close */
#include <netdb.h>      /* gethostbyaddr */
#include <stdlib.h>     /* exit */
#include <string.h>     /* strlen */

void perror_exit(char *message);
int time_to_go = 0;

typedef struct
{
    //Or whatever information that you need
    int *port;
    char ip[256];
    char query[256];
} query_str;

pthread_mutex_t mtx;

void *query_executor(void *args)
{
    query_str *actual_args = args;
    // printf("%ld waiting\n", pthread_self());
    // while(time_to_go==0){};
    pthread_mutex_lock(&mtx);
    printf("-----------------------------------------------------\n");
    printf("%ld will connect to %s:%d to write %s", pthread_self(), actual_args->ip, *(actual_args->port), actual_args->query);

    int port, sock, i;
    char buf[256], buf2[256];
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr *)&server;
    struct hostent *rem;

    /* Create socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    /* Find server address */
    if ((rem = gethostbyname(actual_args->ip)) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }
    port = *(actual_args->port); /*Convert port number to integer*/
    server.sin_family = AF_INET; /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port); /* Server port */
    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0)
        perror_exit("connect");
    printf("Connecting to %s port %d\n", actual_args->ip, port);

    // printf("Give input string: ");
    // fgets(buf, sizeof(buf), stdin); /* Read from stdin*/
    memset(buf, '\0', sizeof(buf));
    strcpy(buf, actual_args->query);

    if (write(sock, buf, 256) < 0)
        perror_exit("write");
    /* receive i-th character transformed */

    char *c1;
    char *c2;
    char *c3;
    char *c4;
    char *c5;
    char *c6;

    c1 = strtok(buf, " \0");
    if (strcmp(c1, "/diseaseFrequency") == 0) 
    {
        int x = 0;
        if (read(sock, &x, sizeof(int)) < 0) //an einai disease frequency diabazei mono 1 int
            perror_exit("read");

        printf("\n>> Query : %s", actual_args->query);

        printf("%d\n", x);
    }
    else if (strcmp(c1, "/topk-AgeRanges") == 0)
    {
        char *c2 = strtok(NULL, " \0");
        int k = atoi(c2);
        if (k > 4)
        {
            k = 4;
        }
        printf("\n>> Query : %s", actual_args->query);
        for (int k2 = 0; k2 < k; k2++)//diavazei k mynhmata apo to server
        {
            char answer[200];
            memset(answer, '\0', sizeof(answer));
            if (read(sock, answer, 200) < 0)
                perror_exit("read");

            printf("%s\n", answer);
        }
    }
    else if (strcmp(c1, "/searchPatientRecord") == 0)
    {
        printf("\n>> Query : %s", actual_args->query);
        char answer[200];
        memset(answer, '\0', sizeof(answer));
        if (read(sock, answer, 200) < 0)//diavazei thn eggrafh
            perror_exit("read");

        printf("%s\n", answer);
    }
    else if (strcmp(c1, "/numPatientAdmissions") == 0)
    {
        printf("\n>> Query : %s", actual_args->query);
        char answer[200];
        memset(answer, '\0', sizeof(answer));
        int x = 0;
        while (x == 0) //oso den exei diavasei mynhma "end of countries" diavazei mynhmata eggrafwn ana xwra
        {
            if (read(sock, answer, 200) < 0)
                perror_exit("read");

            if (strcmp(answer, "end_of_countries") == 0)
            {
                x = 1;
            }
            else
            {
                printf("%s\n", answer);
            }
        }

        // printf("%d\n", x);
    }
    else if (strcmp(c1, "/numPatientDischarges") == 0)
    {
        printf("\n>> Query : %s", actual_args->query);
        char answer[200];
        memset(answer, '\0', sizeof(answer));
        int x = 0;
        while (x == 0)//oso den exei diavasei mynhma "end of countries" diavazei mynhmata eggrafwn ana xwra
        {
            if (read(sock, answer, 200) < 0)
                perror_exit("read");

            if (strcmp(answer, "end_of_countries") == 0)
            {
                x = 1;
            }
            else
            {
                printf("%s\n", answer);
            }
        }

        // printf("%d\n", x);
    }

    printf("-----------------------------------------------------\n\n");

    close(sock); /* Close socket and exit */

    pthread_mutex_unlock(&mtx);
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    int port, i;

    char queryfile[20];
    char serverIP[30];
    char serverPort[20];
    int numThreads;
    int red = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-q") == 0)
        {
            strcpy(queryfile, argv[i + 1]);
            // printf("input = %s\n", folder1);
            red++;
        }
        else if (strcmp(argv[i], "-w") == 0)
        {

            numThreads = atoi(argv[i + 1]);
            // printf("num wo=%d\n", num_workers);
            red++;
        }
        else if (strcmp(argv[i], "-sip") == 0)
        {

            strcpy(serverIP, argv[i + 1]);
            red++;
        }
        else if (strcmp(argv[i], "-sp") == 0)
        {

            strcpy(serverPort, argv[i + 1]);
            red++;
        }
    }

    if (red != 4)
    {
        printf("./whoClient –q queryFile -w numThreads –sp servPort –sip servIP\n");
        return -1;
    }

    char buf[256];
    memset(buf, '\0', sizeof(buf));

    pthread_mutex_init(&mtx, NULL);

    char ip_adr[100];
    memset(ip_adr, '\0', sizeof(ip_adr));
    strcpy(ip_adr, serverIP);
    port = atoi(serverPort); /*Convert port number to integer*/
    int num_of_threads = numThreads;

    FILE *fp = fopen(queryfile, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    int num_of_lines = 0;

    while (fgets(buf, sizeof(buf), fp)) //metraei tis grammes sto input
    {
        num_of_lines++;
    }

    printf("num of lines %d\nnum of threads %d\n", num_of_lines, num_of_threads);

    rewind(fp);

    int num_of_replays = 0;
    num_of_replays = num_of_lines / num_of_threads;
    if ((num_of_lines % num_of_threads) > 0) //briskei poses "omades" tha dhmiourghsei
    {
        num_of_replays++;
    }

    pthread_t query_exec[num_of_threads], exit_ex;

    for (int j = 0; j < num_of_replays; j++)
    {
        pthread_mutex_lock(&mtx); //lockarei ta threads

        query_str *args[num_of_threads];
        int k = 0;
        printf("\n\\\\\\\\ROUND %d////\n\n", j);
        while (j * num_of_threads + k < num_of_lines && k < num_of_threads)
        {
            args[k] = malloc(sizeof *args[k]);
            fgets(buf, sizeof(buf), fp);
            args[k]->port = &port;
            strcpy(args[k]->ip, ip_adr);
            strcpy(args[k]->query, buf);
            pthread_create(&query_exec[k], NULL, query_executor, args[k]);
            printf("%ld created\n",query_exec[k]);
            k++;
        }

        // getchar();
        // time_to_go = 1;
        pthread_mutex_unlock(&mtx); //kanei unlock ta threads pou dhmiourghse 

 
        int k2 = 0;
        while (j * num_of_threads + k2 < num_of_lines && k2 < num_of_threads)
        {
            pthread_join(query_exec[k2], 0);
            free(args[k2]);
            k2++;
        }

        // free(args);
    }

    fclose(fp);
}

void perror_exit(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}
