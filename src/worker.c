#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
// #include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>  /* sockets */
#include <sys/socket.h> /* sockets */
#include <netinet/in.h> /* internet sockets */
#include <netdb.h>      /* gethostbyaddr */
#include "../structs/file_list.h"
#include "../structs/summary_list.h"
#include "../structs/records.h"

long int datetoint(const char *s);
int run = 1;
void intHandlerChild() { run = 0; }
int refresh = 0;
int ppid;
void refHandlerChild();
void perror_exit(char *message);

int main(int argc, char *argv[]) //./worker fifo_in fifo_out workload parent_dir buffersize
{
    printf("{%d} ./worker 1.%s 2.%s 3.%s 4.%s 5.%s 6.%s 7.%s\n", getpid(), argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
    signal(SIGINT, intHandlerChild);
    // signal(SIGUSR2, refHandlerChild);

    int port, sock;
    char buf[256];
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr *)&server;
    struct hostent *rem;
    /* Create socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    /* Find server address */
    if ((rem = gethostbyname(argv[6])) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }
    port = atoi(argv[7]);        /*Convert port number to integer*/
    server.sin_family = AF_INET; /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port); /* Server port */

    ppid = getppid();


    char err[35];
    sprintf(err, "./error_reports/error_report.%d", getpid());
    FILE *er = fopen(err, "a+"); //anoigei ena arxeio opou ektypwnei kathe error i kathe success kata thn anagnwsh twn eggrafwn

    int workload = atoi(argv[3]);
    int buffersize = atoi(argv[5]);

    char mid_path[workload][40];
    char countries_served[workload][30];
    long int last_date[workload];
    int ok = 1;
    int in_fd, out_fd;

    struct node *head[workload];
    struct rec *records_head = NULL;

    int k = 0;
    struct summary *sums = NULL;

    for (int i = 0; i < workload; i++) //gia kathe xwra poy tou exei antethei
    {
        head[i] = NULL;
        last_date[i] = 0;
        int chunks = 0;
        char cntr[30];

        in_fd = open(argv[1], O_RDONLY);
        while (in_fd < 0)
        {
            in_fd = open(argv[1], O_RDONLY);
        }
        read(in_fd, &chunks, sizeof(int));
        close(in_fd);

        // printf("{%d} %d -> %d\n", getpid(), workload, chunks);

        for (int c = 0; c < chunks; c++)
        {
            char *read_chunk = malloc(buffersize + 1);
            out_fd = open(argv[2], O_WRONLY);
            while (out_fd < 0)
            {
                out_fd = open(argv[2], O_WRONLY);
            }
            write(out_fd, &ok, sizeof(int));
            close(out_fd);

            in_fd = open(argv[1], O_RDONLY);
            while (in_fd < 0)
            {
                in_fd = open(argv[1], O_RDONLY);
            }
            read(in_fd, read_chunk, buffersize);

            read_chunk[buffersize] = '\0';

            close(in_fd);

            if (c == 0)
            {
                strcpy(cntr, read_chunk);
            }
            else
            {

                strcat(cntr, read_chunk);
            }
            free(read_chunk);
        }

        out_fd = open(argv[2], O_WRONLY);
        while (out_fd < 0)
        {
            out_fd = open(argv[2], O_WRONLY);
        }
        write(out_fd, &ok, sizeof(int));
        close(out_fd);

        strcpy(mid_path[i], argv[4]); //dhmioyrgei to path gia toys fakelous poy tha anoiksei
        strcat(mid_path[i], cntr);
        strcat(mid_path[i], "/");

        strcpy(countries_served[i], cntr);

        printf("%s\n", mid_path[i]);

        DIR *cntr_folder = opendir(mid_path[i]);
        struct dirent *date_folder;

        if (cntr_folder == NULL)
        {
            fprintf(er, "Could not open current directory\n");
            return 0;
        }

        while ((date_folder = readdir(cntr_folder)) != NULL) //bazei se mia lista olous toys ypofakelous-hmeromhnies se ayjousa seira
        {
            if (strcmp(date_folder->d_name, ".") != 0 && strcmp(date_folder->d_name, "..") != 0)
            {
                char full_path[100];
                strcpy(full_path, mid_path[i]);
                strcat(full_path, date_folder->d_name);

                head[i] = insert(head[i], datetoint(date_folder->d_name), full_path);
                if (datetoint(date_folder->d_name) > last_date[i])
                {
                    last_date[i] = datetoint(date_folder->d_name);
                }
            }
        }

        // struct node *sherlock = head[i];

        struct node *ptr = head[i]; //deikths sto head ths listas me tiw hmeromhnies
        // int k = 0;

        char *key;
        char *type;
        char *fn;
        char *ln;
        char *dis;
        int age = 0;
        long int inDate = 0;
        while (ptr != NULL) //oso yparxoyn kai alloi fakeloi
        {

            FILE *infile = fopen(ptr->full_path, "r");

            if (infile != NULL)
            {
                char line[120];
                while (fgets(line, sizeof(line), infile))
                {
                    key = strtok(line, " ");
                    type = strtok(NULL, " ");
                    fn = strtok(NULL, " ");
                    ln = strtok(NULL, " ");
                    dis = strtok(NULL, " ");
                    age = atoi(strtok(NULL, "\n"));

                    if (strcmp(type, "ENTER") == 0)
                    {
                        records_head = rec_ins(records_head, key, fn, ln, age, ptr->int_date, dis, countries_served[i]); //eisagei mia nea rec sth lista eggrafwn
                        fprintf(er, "New Record with ID: %s added\n", key);                                              //typwnei mymnhma sto error_report.(pid)
                        int done = update(sums, age, dis, cntr, ptr->int_date);                                          //psaxnei sta yparxonta sums kai ananevnei ta kroysmata ana kathgoria
                        if (done == 0)                                                                                   //an den vrei sum gia ayth thn arrvstia se ayth thn hmeromhnia epistrefei 0
                        {
                            sums = ins(sums, age, dis, cntr, ptr->int_date); //eisagei ena neo sum sth lista
                            k++;
                        }
                    }
                    else if (strcmp(type, "EXIT") == 0)
                    {

                        int err_code = new_exit(records_head, key, ptr->int_date); //an einai typou exit prospathei na ananewsei ta rec
                        if (err_code == 0)                                         //analoga th timh pou epistrefete ektypwnei sto error_report katallhlo mynhma
                        {
                            fprintf(er, "ERROR not existing record with ID: %s\n", key);
                        }
                        else if (err_code == -1)
                        {
                            fprintf(er, "ERROR no valid exit date for record with ID: %s\n", key);
                        }
                        else
                        {
                            fprintf(er, "Exit date updated for record with ID: %s\n", key);
                        }
                    }
                }

                fclose(infile);
            }
            else
            {

                fprintf(er, "Unable to open file\n");
            }

            ptr = ptr->next;
        }

        closedir(cntr_folder);
    }

    // printf("faultttt\n");

    int sock_read, newsock_read;
    int port_read;
    struct sockaddr_in server_read, client_read;
    socklen_t clientlen_read;
    struct sockaddr *serverptr_read = (struct sockaddr *)&server_read;
    struct sockaddr *clientptr_read = (struct sockaddr *)&client_read;
    struct hostent *rem_read;

    if ((sock_read = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    server_read.sin_family = AF_INET; /* Internet domain */
    server_read.sin_addr.s_addr = htonl(INADDR_ANY);
    server_read.sin_port = 0; /* The given port */
    // printf("faultttt\n");
    /* Bind socket to address */
    if (bind(sock_read, serverptr_read, sizeof(server_read)) < 0)
        perror_exit("bind");
    /* Listen for connections */
    if (listen(sock_read, 5) < 0)
        perror_exit("listen");

    struct sockaddr_in sin_read;
    socklen_t len_read = sizeof(sin_read);
    if (getsockname(sock_read, (struct sockaddr *)&sin_read, &len_read) == -1)
        perror("getsockname");
    else
        port_read = ntohs(sin_read.sin_port);

    printf("I'm worker %d and I'm listening in port %d\n", getpid(), port_read);

    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0)
        perror_exit("connect");
    printf("Connecting to %s port %d\n", argv[6], port);

    write(sock, &port_read, sizeof(int)); //grafei to port sto server

    char host[100];
    memset(host, '\0', sizeof(host));
    gethostname(host, sizeof(host));
    // strcpy(host, argv[6]);
    write(sock, host, sizeof(host)); //grafei to host

    write(sock, &k, sizeof(int));

    struct summary *holmes = sums;
    for (int s = 0; s < k; s++)
    {
        char sample[200]; // = malloc(200 * sizeof(char));
        memset(sample, '\0', sizeof(sample));
        sprintf(sample, "%ld-%ld-%ld/%s/%s/%d/%d/%d/%d\n", ((holmes->int_date) % 360) % 30, ((holmes->int_date) % 360) / 30, (holmes->int_date) / 360, holmes->cntr, holmes->dis, holmes->zero, holmes->twenty, holmes->forty, holmes->sixty);

        write(sock, sample, 200);  //grafei ta summaries


        holmes = holmes->next;
    }

    close(sock);

    int done = 0;

    while (done == 0)
    {
        clientlen_read = sizeof(client_read);
        /* accept connection */
        if ((newsock_read = accept(sock_read, clientptr_read, &clientlen_read)) < 0)
            perror_exit("accept");

        char *buf = malloc(256 * sizeof(char));
        memset(buf, '\0', sizeof(memset));
        read(newsock_read, buf, 256);
        /* Receive 1 char */
        printf("%d read %s", port_read, buf); /* Print received char */

        char *c1 = strtok(buf, " \0");
        // write(newsock_read, &port_read, sizeof(int));
        if (strcmp(c1, "/diseaseFrequency") == 0)
        {
            char *c2 = strtok(NULL, " \0");
            char *c3 = strtok(NULL, " \0");
            char *c4 = strtok(NULL, " \0");
            char *c5 = strtok(NULL, " \n\0");
            if (c5 == NULL)
            {
                long int from = datetoint(c3);
                long int to = datetoint(c4);

                int count = 0;

                // printf("%ld / %ld\n", from, to);
                struct summary *sherlock = sums;
                while (sherlock != NULL)
                {
                    // printf("!!!\n");
                    if (strcmp(sherlock->dis, c2) == 0 && sherlock->int_date >= from && sherlock->int_date <= to)
                    {
                        count = count + (sherlock->zero) + (sherlock->twenty) + (sherlock->forty) + (sherlock->sixty);
                    }
                    sherlock = sherlock->next;
                }

                printf("%d\n", count);
                if (write(newsock_read, &count, sizeof(int)) < 0)
                    perror_exit("write");
            }
            else
            {
                int count = 0;
                long int from = datetoint(c3);
                long int to = datetoint(c4);

                struct summary *sherlock = sums;
                while (sherlock != NULL)
                {
                    // printf("!!!\n");
                    if (strcmp(sherlock->cntr, c5) == 0 && strcmp(sherlock->dis, c2) == 0 && sherlock->int_date >= from && sherlock->int_date <= to)
                    {
                        count = count + (sherlock->zero) + (sherlock->twenty) + (sherlock->forty) + (sherlock->sixty);
                    }
                    sherlock = sherlock->next;
                }

                printf("%d\n", count);
                if (write(newsock_read, &count, sizeof(int)) < 0)
                    perror_exit("write");
            }
        }
        else if (strcmp(c1, "/topk-AgeRanges") == 0)
        {
            char *c2 = strtok(NULL, " \0");
            char *c3 = strtok(NULL, " \0");
            char *c4 = strtok(NULL, " \0");
            char *c5 = strtok(NULL, " \0");
            char *c6 = strtok(NULL, " \n\0");

            long int from = datetoint(c5);
            long int to = datetoint(c6);

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

            int found = 0;
            for (int x = 0; x < workload; x++)
            {
                if (strcmp(c3, countries_served[x]) == 0)
                {
                    found = 1;
                    printf("found\n");
                    struct summary *sherlock = sums;
                    while (sherlock != NULL)
                    {
                        // printf("!!!\n");
                        if (strcmp(sherlock->cntr, c3) == 0 && strcmp(sherlock->dis, c4) == 0 && sherlock->int_date >= from && sherlock->int_date <= to)
                        {
                            zero = zero + (sherlock->zero);
                            twenty = twenty + (sherlock->twenty);
                            forty = forty + (sherlock->forty);
                            sixty = sixty + (sherlock->sixty);
                            total = total + (sherlock->zero) + (sherlock->twenty) + (sherlock->forty) + (sherlock->sixty);
                        }
                        sherlock = sherlock->next;
                    }
                }
            }
            if (write(newsock_read, &found, sizeof(int)) < 0)
                perror_exit("write");

            if (found == 1) //an vrike th xwra grafei ton arithmo twn krousmatwn
            {
                printf("%f %f %f %f\n", zero, twenty, forty, sixty);
                if (write(newsock_read, &zero, sizeof(float)) < 0)
                    perror_exit("write");
                if (write(newsock_read, &twenty, sizeof(float)) < 0)
                    perror_exit("write");
                if (write(newsock_read, &forty, sizeof(float)) < 0)
                    perror_exit("write");
                if (write(newsock_read, &sixty, sizeof(float)) < 0)
                    perror_exit("write");
                if (write(newsock_read, &total, sizeof(float)) < 0)
                    perror_exit("write");
            }
        }
        else if (strcmp(c1, "/searchPatientRecord") == 0)
        {
            char *c2 = strtok(NULL, "  \n\0");
            int found = 0;
            char answer[200];
            memset(answer, '\0', sizeof(answer));
            ////////////////////////////////////

            struct rec *rec_sherlock = records_head;
            while (rec_sherlock != NULL)
            {
                if (strcmp(rec_sherlock->ID, c2) == 0)
                {
                    if (rec_sherlock->outDate == 0)
                    {
                        sprintf(answer, "%s %s %s %s %d %ld-%ld-%ld --", rec_sherlock->ID, rec_sherlock->first_name, rec_sherlock->last_name, rec_sherlock->disease, rec_sherlock->age, ((rec_sherlock->inDate) % 360) % 30, ((rec_sherlock->inDate) % 360) / 30, (rec_sherlock->inDate) / 360);
                    }
                    else
                    {
                        sprintf(answer, "%s %s %s %s %d %ld-%ld-%ld %ld-%ld-%ld", rec_sherlock->ID, rec_sherlock->first_name, rec_sherlock->last_name, rec_sherlock->disease, rec_sherlock->age, ((rec_sherlock->inDate) % 360) % 30, ((rec_sherlock->inDate) % 360) / 30, (rec_sherlock->inDate) / 360, ((rec_sherlock->outDate) % 360) % 30, ((rec_sherlock->outDate) % 360) / 30, (rec_sherlock->outDate) / 360);
                    }
                    found = 1;
                }
                rec_sherlock = rec_sherlock->next;
            }

            if (write(newsock_read, &found, sizeof(int)) < 0)
                perror_exit("write");

            if (found == 1)
            {
                if (write(newsock_read, answer, 200) < 0)
                    perror_exit("write");
            }
        }
        else if (strcmp(c1, "/numPatientAdmissions") == 0)
        {

            char *c2 = strtok(NULL, " \0");
            char *c3 = strtok(NULL, " \0");
            char *c4 = strtok(NULL, " \0");
            char *c5 = strtok(NULL, " \n\0");
            if (c5 == NULL)
            {
                if (write(newsock_read, &workload, sizeof(int)) < 0)
                    perror_exit("write");

                long int from = datetoint(c3);
                long int to = datetoint(c4);
                char answer[200];
                memset(answer, '\0', sizeof(answer));
                for (int x = 0; x < workload; x++)
                {
                    int count = 0;
                    // printf("%ld / %ld\n", from, to);
                    struct rec *sherlock = records_head;
                    while (sherlock != NULL)
                    {
                        if (strcmp(sherlock->country, countries_served[x]) == 0 && strcmp(sherlock->disease, c2) == 0 && sherlock->inDate >= from && sherlock->inDate <= to)
                        {
                            count++;
                        }
                        sherlock = sherlock->next;
                    }
                    printf("%s %d\n", countries_served[x], count);
                    sprintf(answer, "%s %d", countries_served[x], count);
                    if (write(newsock_read, answer, 200) < 0) //gia kathe xwra grafei thn apanthsh
                        perror_exit("write");
                }
            }
            else
            {
                long int from = datetoint(c3);
                long int to = datetoint(c4);
                char answer[200];
                int found = 0;
                memset(answer, '\0', sizeof(answer));
                for (int x = 0; x < workload; x++)
                {
                    if (strcmp(c5, countries_served[x]) == 0)
                    {
                        int count = 0;
                        // printf("%ld / %ld\n", from, to);
                        struct rec *sherlock = records_head;
                        while (sherlock != NULL)
                        {
                            if (strcmp(sherlock->country, countries_served[x]) == 0 && strcmp(sherlock->disease, c2) == 0 && sherlock->inDate >= from && sherlock->inDate <= to)
                            {
                                count++;
                            }
                            sherlock = sherlock->next;
                        }
                        sprintf(answer, "%s %d", countries_served[x], count);
                        printf("%s\n", answer);
                        found = 1;
                    }
                }
                if (write(newsock_read, &found, sizeof(int)) < 0)
                    perror_exit("write");
                if (found == 1) //an vrike th xwra grafei ton arithmo twn krousmatwn
                {
                    if (write(newsock_read, answer, 200) < 0)
                        perror_exit("write");
                }
            }
        }
        else if (strcmp(c1, "/numPatientDischarges") == 0)
        {

            char *c2 = strtok(NULL, " \0");
            char *c3 = strtok(NULL, " \0");
            char *c4 = strtok(NULL, " \0");
            char *c5 = strtok(NULL, " \n\0");
            if (c5 == NULL)
            {
                if (write(newsock_read, &workload, sizeof(int)) < 0)
                    perror_exit("write");

                long int from = datetoint(c3);
                long int to = datetoint(c4);
                char answer[200];
                memset(answer, '\0', sizeof(answer));
                for (int x = 0; x < workload; x++)
                {
                    int count = 0;
                    // printf("%ld / %ld\n", from, to);
                    struct rec *sherlock = records_head;
                    while (sherlock != NULL)
                    {
                        if (strcmp(sherlock->country, countries_served[x]) == 0 && strcmp(sherlock->disease, c2) == 0 && sherlock->outDate >= from && sherlock->outDate <= to)
                        {
                            count++;
                        }
                        sherlock = sherlock->next;
                    }
                    printf("%s %d\n", countries_served[x], count);
                    sprintf(answer, "%s %d", countries_served[x], count);
                    if (write(newsock_read, answer, 200) < 0)
                        perror_exit("write");
                }
            }
            else
            {
                long int from = datetoint(c3);
                long int to = datetoint(c4);
                char answer[200];
                int found = 0;
                memset(answer, '\0', sizeof(answer));
                for (int x = 0; x < workload; x++)
                {
                    if (strcmp(c5, countries_served[x]) == 0)
                    {
                        int count = 0;
                        // printf("%ld / %ld\n", from, to);
                        struct rec *sherlock = records_head;
                        while (sherlock != NULL)
                        {
                            if (strcmp(sherlock->country, countries_served[x]) == 0 && strcmp(sherlock->disease, c2) == 0 && sherlock->outDate >= from && sherlock->outDate <= to)
                            {
                                count++;
                            }
                            sherlock = sherlock->next;
                        }
                        sprintf(answer, "%s %d", countries_served[x], count);
                        printf("%s\n", answer);
                        found = 1;
                    }
                }
                if (write(newsock_read, &found, sizeof(int)) < 0)
                    perror_exit("write");
                if (found == 1) //an vrike th xwra grafei ton arithmo twn krousmatwn
                {
                    if (write(newsock_read, answer, 200) < 0)
                        perror_exit("write");
                }
            }
        }
        else if (strcmp(c1, "/exit") == 0) //an diavasei exit termatizei to loop
        {
            done = 1;
            printf("ooopsie\n"); /* Print received char */
        }
        free(buf);
        close(newsock_read);
    }

    // printf("{%d} term\n", getpid());
    for (int i = 0; i < workload; i++) //apodesmeyei to xwro
    {
        struct node *rec_del = head[i];
        struct node *rec_del_temp = NULL;
        while (rec_del != NULL)
        {
            rec_del_temp = rec_del->next;
            free(rec_del);
            rec_del = rec_del_temp;
            // rec_del = rec_sherlock->next;
        }
    }
    struct rec *rec_del = records_head;
    struct rec *rec_del_temp = NULL;
    while (rec_del != NULL)
    {
        // printf("%s | %s | %s | %s | %s | %d\n",rec_del->ID,rec_del->first_name,rec_del->last_name,rec_del->disease,rec_del->country,rec_del->age);
        rec_del_temp = rec_del->next;
        free(rec_del);
        rec_del = rec_del_temp;
        // rec_del = rec_sherlock->next;
    }
    struct summary *sum_del = sums; //apeleytherwnei th lista me ta sums 
    struct summary *sum_del_temp;
    for (int iam = 0; iam < k; iam++)
    {
        sum_del_temp = sum_del->next;
        free(sum_del);
        sum_del = sum_del_temp;
    }
    fclose(er);
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

void refHandlerChild()
{
    refresh = 1;
}

void perror_exit(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}
