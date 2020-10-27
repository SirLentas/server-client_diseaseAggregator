#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../structs/country_list.h"
#include "../structs/summary_list.h"

long int datetoint(const char *s);
struct summary *sums_head = NULL;
int run = 1;
int num_workers = 0;
int *workers_id;

int refresh_time = 0;
void refHandler();
void intHandler();

int main(int argc, char *argv[])
{

    char folder1[20];
    char serverIP[30];
    char serverPort[20];
    int buffersize;
    int red = 0;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0)
        {
            strcpy(folder1, argv[i + 1]);
            if (argv[i + 1][strlen(argv[i + 1]) - 1] != '/')
                strcat(folder1, "/");
            // printf("input = %s\n", folder1);
            red++;
        }
        else if (strcmp(argv[i], "-w") == 0)
        {

            num_workers = atoi(argv[i + 1]);
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

            strcpy(serverIP, argv[i + 1]);
            red++;
        }
        else if (strcmp(argv[i], "-p") == 0)
        {

            strcpy(serverPort, argv[i + 1]);
            red++;
        }
    }

    if (red != 5)
    {
        printf("./master –w numWorkers -b bufferSize –s serverIP –p serverPort -i input_dir\n");
        return -1;
    }

    int ok = 1;
    int sums_counter = 0;

    int requests = 0;
    int done = 0;
    int failed = 0;

    DIR *inp_folder = opendir(folder1);
    if (inp_folder == NULL)
    {
        printf("Could not open input directory");
        return 0;
    }

    struct dirent *cntr_folder;
    struct country *cntr_head = NULL;
    int cntr_dirs = 0;
    while ((cntr_folder = readdir(inp_folder)) != NULL) //bazei se mia lista allouw tous ypofakeloys me ta onomata xwrwn
    {
        if (strcmp(cntr_folder->d_name, ".") != 0 && strcmp(cntr_folder->d_name, "..") != 0)
        {
            // printf("%s\n", cntr_folder->d_name);
            cntr_head = c_ins(cntr_head, cntr_folder->d_name);
            cntr_dirs++;
        }
    }

    closedir(inp_folder);

    if (cntr_dirs < num_workers) // an dhmioyrghthikan parapanv worker apo oti zhththikan "apoleiontai oi parapanw"
    {
        printf("%d worker(s) fired\n", num_workers - cntr_dirs);
        num_workers = cntr_dirs;
    }

    char worker_in[num_workers][30];
    char worker_out[num_workers][30];
    int workload[num_workers];
    int temp = 0;
    int num_dirs = cntr_dirs;
    int num_workers_temp = num_workers;
    for (int k = 0; k < num_workers; k++) //gia kathe worker vriskei to workload toy se logikh round robin
    {
        temp = num_dirs / num_workers_temp;
        if ((num_dirs % num_workers_temp) > 0)
        {
            temp++;
        }
        workload[k] = temp;
        num_dirs = num_dirs - temp;
        num_workers_temp--;
    }
    int f_des_in[num_workers];
    int f_des_out[num_workers];

    workers_id = malloc(num_workers * sizeof(int)); //kanei malloc to pinaka poy krataei ta worker ids
    char workers_cntr[num_workers][10];
    pid_t workerpid;

    struct country *cntr_sherlock = cntr_head;

    for (int i = 0; i < num_workers; i++)
    {
        sprintf(worker_in[i], "./worker%d-in", i);
        sprintf(worker_out[i], "./worker%d-out", i);
        if ((workerpid = fork()) != 0)
        {
            remove(worker_in[i]); //diagrafei tyxon pipes poy yparxoyn me to idio onoma
            remove(worker_out[i]);
            mkfifo(worker_in[i], 0666); //dhmiourgei ta nea pipes
            mkfifo(worker_out[i], 0666);
            workers_id[i] = workerpid; //apothikeuei to worker_id
        }
        else
        {
            char wrkld[8];
            sprintf(wrkld, "%d", workload[i]);
            char bf[8];
            sprintf(bf, "%d", buffersize);

            char *arg[8];
            arg[0] = "./worker";
            arg[1] = worker_in[i];
            arg[2] = worker_out[i];
            arg[3] = malloc(8 * sizeof(char));
            strcpy(arg[3], wrkld);
            arg[4] = malloc(30 * sizeof(char));
            strcpy(arg[4], folder1);
            arg[5] = bf;
            arg[6] = malloc(100 * sizeof(char));
            strcpy(arg[6], serverIP);
            arg[7] = malloc(20 * sizeof(char));
            strcpy(arg[7], serverPort);
            // arg[6] = argv[7];
            // arg[7] = argv[8];
            arg[8] = NULL;

            // printf("./worker 1.%s 2.%s 3.%s 4.%s 5.%s\n", arg[1], arg[2], arg[3], arg[4], arg[5]);

            execvp(arg[0], arg);
            exit(-1);
        }
    }

    for (int i = 0; i < num_workers; i++) //gia kathe worker
    {
        for (int j = 0; j < workload[i]; j++) //gia kathe country sto worker
        {
            char *str = malloc(sizeof(char) * strlen(cntr_sherlock->cntr) + 1);
            strcpy(str, cntr_sherlock->cntr);

            int length = strlen(cntr_sherlock->cntr);
            int chunks = length / buffersize; //vriskei se posa chunks ua xvrisei th xwra-mynhma pou prepei na grapseis to pipe
            if ((length % buffersize) > 0)
            {
                chunks++;
            }

            f_des_in[i] = open(worker_in[i], O_WRONLY); /////chunk num
            while (f_des_in[i] < 0)
            {
                f_des_in[i] = open(worker_in[i], O_WRONLY);
            }
            write(f_des_in[i], &chunks, sizeof(int)); //grafei enan int gia na jerei to child posa chunks na diabasei
            close(f_des_in[i]);

            int ok = 0;
            for (int c = 0; c < chunks; c++)
            {
                f_des_out[i] = open(worker_out[i], O_RDONLY); //perimenei na diabasei oti to child diavase to prohgoymeno chunk
                while (f_des_out[i] < 0)
                {
                    f_des_out[i] = open(worker_out[i], O_RDONLY);
                }
                read(f_des_out[i], &ok, sizeof(int));
                close(f_des_out[i]);

                char *str2 = malloc(sizeof(char) * buffersize);
                for (int j = 0; j < buffersize; j++)
                {
                    if ((j + c * buffersize) < length)
                        str2[j] = str[j + c * buffersize];
                    else
                    {
                        str2[j] = '\0';
                    }
                }
                f_des_in[i] = open(worker_in[i], O_WRONLY); /////country

                while (f_des_in[i] < 0)
                {
                    f_des_in[i] = open(worker_in[i], O_WRONLY);
                }
                write(f_des_in[i], str2, buffersize); //grafei to chunk
                close(f_des_in[i]);
                free(str2);
            }

            f_des_out[i] = open(worker_out[i], O_RDONLY); //perimenei na diabasei oti to child diavase to prohgoymeno chunk
            while (f_des_out[i] < 0)
            {
                f_des_out[i] = open(worker_out[i], O_RDONLY);
            }
            read(f_des_out[i], &ok, sizeof(int));
            close(f_des_out[i]);

            free(str);

            cntr_sherlock = cntr_sherlock->next;
        }
    }

    cntr_sherlock = cntr_head;
    for (int a = 0; a < num_workers; a++)
    {
        for (int w = 0; w < workload[a]; w++)
        {
            printf("%s %d\n", cntr_sherlock->cntr, workers_id[a]);
            cntr_sherlock = cntr_sherlock->next;
        }
    }

    signal(SIGINT, intHandler); //kai stis 2 periptwseis kanei to run=0 kai zhtaei apo ton xrhsth na dwsei mia teleytaia entolh
    signal(SIGQUIT, intHandler);

    int status; //perimenei ta child na termatisoun
    // printf("1\n");
    for (int i = 0; i < num_workers; i++)
    {
        // kill(workers_id[i], SIGINT);
        while ((waitpid(workers_id[i], &status, WNOHANG)) == 0)
        {
            sleep(0.5);
        }
    }
    // printf("2\n");
    for (int i = 0; i < num_workers; i++) //eidopoiei ta child oti den xreiazontai pleon mesw pipes stelnontas enan int (no_need_work)
    {
        remove(worker_in[i]);
        remove(worker_out[i]);
    }

    struct summary *sum_del = sums_head; //apodesmeysh mnhmhw
    struct summary *sum_del_temp;
    for (int iam = 0; iam < sums_counter; iam++)
    {
        // printf("%ld | %s | %s | %d | %d | %d | %d\n",sum_del->int_date,sum_del->cntr,sum_del->dis,sum_del->zero,sum_del->twenty,sum_del->forty,sum_del->sixty);
        sum_del_temp = sum_del->next;
        free(sum_del);
        sum_del = sum_del_temp;
    }

    struct country *c_del = cntr_head;
    struct country *c_del_temp;
    for (int iam = 0; iam < cntr_dirs; iam++)
    {
        c_del_temp = c_del->next;
        free(c_del);
        c_del = c_del_temp;
    }
    free(workers_id);
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

void intHandler()
{
    // red_code = 1;
    run = 0;
}

void refHandler()
{
    refresh_time = 1;
}