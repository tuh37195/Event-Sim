#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include"./headers/queue.h"
#include"./headers/config.h"
#include"./headers/component.h"

//function prototypes
void sim_cpu(config *myConfig, component *cpu, component *disk1, component *disk2, int currentTime);
void sim_disk(struct component *disk, component *cpu, int currentTime, int DISK_MIN, int DISK_MAX, int diskNum);
int randNumber(int min, int max);
void output(char *s);
void output_config(config *conf);

//define filenames
#define LOGFILE "./log.txt"
#define CONFIG_FILE "./config.txt"
//define hardware status
#define IDLE 0
#define RUNNING 1
//for writing to log
#define BUFF_SIZE 1024

int main(){
  //get time
  clock_t real_time = clock();

  //get config info from config.txt
  config *myConfig = get_config(CONFIG_FILE);
  //set current time to starting time specified in config
  int currentTime = myConfig->INIT_TIME;

  //for randNumber function
  //if seed not specified
  if (myConfig->SEED == 0){
    //use time
    myConfig->SEED = time(NULL);
  }
  //seed the random number
  srand(myConfig->SEED);

  //used for output
  char string[BUFF_SIZE];

  //track hw utilization
  int cpu_track = 0;
  int disk1_track = 0;
  int disk2_track = 0;

  //set up simulated components
  component *cpu = get_component();
  component *disk1 = get_component();
  component *disk2 = get_component();

  //create job queue
  int job_wait_time = 0;
  //tracks number of jobs
  int job_count = 0;

  //sim start
  output_config(myConfig);
  output("************SIMULATION START************");

  //main loop, each tick on the simulated clock checks the CPU and both disks.
  while (currentTime < myConfig->FIN_TIME){

    //if time for new job
    if (job_wait_time <= currentTime){
      //add one to job counter
      job_count++;
      //add job to cpu queue
      push(cpu, job_count);
      //get wait time for next job
      job_wait_time = currentTime + randNumber(myConfig->ARRIVE_MIN, myConfig->ARRIVE_MAX);
    }

    //if cpu is free and has a job in queue
    if (cpu->WAIT_TIME < currentTime && !Empty(cpu->QUEUE)) {
        //simulates the CPU, gets it's current wait time
        sim_cpu(myConfig, cpu, disk1, disk2, currentTime);
    }

    //if disk 1 is free and has a job in queue
    if (disk1->WAIT_TIME < currentTime && !Empty(disk1->QUEUE)) {
        //simulates disk 1, gets it's current wait time
        sim_disk(disk1, cpu, currentTime, myConfig->DISK1_MIN, myConfig->DISK1_MAX, 1);
    }

    //If disk 2 is free and has a job in queue
    if (disk2->WAIT_TIME < currentTime && !Empty(disk2->QUEUE)) {
        //simulates disk 2, gets it's current wait time
        sim_disk(disk2, cpu, currentTime, myConfig->DISK2_MIN, myConfig->DISK2_MAX, 2);
    }

    //track utilization
    if (cpu->STATUS == RUNNING)
      cpu_track++;
    if (disk1->STATUS == RUNNING)
      disk1_track++;
    if (disk2->STATUS == RUNNING)
	    disk2_track++;

    //track queue size
    cpu->SIZE += cpu->QUEUE->count;
    disk1->SIZE += disk1->QUEUE->count;
    disk2->SIZE += disk2->QUEUE->count;

    //track max jobs at one time
    if (cpu->QUEUE->count > cpu->MOST_JOBS)
      cpu->MOST_JOBS = cpu->QUEUE->count;

    if (disk1->QUEUE->count > disk1->MOST_JOBS)
      disk1->MOST_JOBS = disk1->QUEUE->count;

    if (disk2->QUEUE->count > disk2->MOST_JOBS)
      disk2->MOST_JOBS = disk2->QUEUE->count;


    currentTime++;
  } //end while
  sprintf(string, "%d \t***FIN***", currentTime);
  output(string);

  //get difference in real time
  real_time = clock() - real_time;
  //get total simulation time
  int sim_time = myConfig->FIN_TIME - myConfig->INIT_TIME;
  //outputing stats from simulation
  output("***Stats***");
  output("CPU:");
  sprintf(string, "\tUTILIZATION: %%%.2f", (cpu_track*100.0 / currentTime));
  output(string);
  sprintf(string, "\tAVERAGE SIZE OF QUEUE: %.2f", (double)cpu->SIZE / sim_time);
  output(string);
  sprintf(string, "\tMOST ITEMS IN QUEUE: %lu", cpu->MOST_JOBS);
  output(string);
  sprintf(string, "\tJOBS PROCESSED PER TIC: %.4f", (float)cpu->PROCESSED/sim_time);
  output(string);
  output("DISK 1:");
  sprintf(string, "\tUTILIZATION: %%%.2f", (disk1_track*100.0/currentTime));
  output(string);
  sprintf(string, "\tAVERAGE SIZE OF QUEUE: %.2f", (double)disk1->SIZE / sim_time);
  output(string);
  sprintf(string, "\tMOST ITEMS IN QUEUE: %lu", disk1->MOST_JOBS);
  output(string);
  sprintf(string, "\tJOBS PROCESSED PER TIC: %.4f", (float)disk1->PROCESSED/sim_time);
  output(string);
  output("DISK 2:");
  sprintf(string, "\tUTILIZATION: %%%.2f", (disk2_track*100.0/currentTime));
  output(string);
  sprintf(string, "\tAVERAGE SIZE OF QUEUE: %.2f", (double)disk2->SIZE / sim_time);
  output(string);
  sprintf(string, "\tMOST ITEMS IN QUEUE: %lu", disk2->MOST_JOBS);
  output(string);
  sprintf(string, "\tJOBS PROCESSED PER TIC: %.4f", (float)disk2->PROCESSED/sim_time);
  output(string);
  output("SYSTEM:");
  sprintf(string, "\tSIM TIME: %d ticks", sim_time);
  output(string);
  sprintf(string, "\tRUN TIME: %.2fms", (double)real_time*1000/CLOCKS_PER_SEC);
  output(string);
  sprintf(string, "\tJOBS CREATED: %d", job_count);
  output(string);
  sprintf(string, "\tJOBS COMPLETED: %d", cpu->COMPLETED);
  output(string);
  output("************SIMULATION END************");
}


//this function simulates the CPU
void sim_cpu(config *myConfig, component *cpu, component *disk1, component *disk2, int currentTime){

  //used for sending jobs to a disk
  int diskNum;
  int temp;

  //used for logging output
  char string[BUFF_SIZE];

  //get current status for CPU
  switch (cpu->STATUS) {

    //if cpu is IDLE
    case IDLE:
      //get new wait time
      cpu->WAIT_TIME = currentTime + randNumber(myConfig->CPU_MIN, myConfig->CPU_MAX);
      //print out arrival message
      sprintf(string,"%d\tJob %d: Arrived at CPU", currentTime, cpu->QUEUE->front->key);
      output(string);
      //update status
      cpu->STATUS = RUNNING;
      break;

    //CPU is running
    case RUNNING:
      //remove job from CPU queue
      temp = pop(cpu);

      //check if job is finished (random % chance based on QUIT_PROB)
      if (myConfig->QUIT_PROB >= randNumber(0, 100)){
        //log message
        sprintf(string, "%d\tJob %d: Finished.", currentTime, temp);
        output(string);
        //add to PROCESSED and COMPLETED counters
        cpu->COMPLETED ++;
        cpu->PROCESSED ++;
        //update cpu status and finish
        cpu->STATUS = IDLE;
        return;
      }

      //send job to DISK with least amount of jobs
      if (disk1->QUEUE->count < disk2->QUEUE->count){
        push(disk1, temp);
        diskNum = 1;
      }
      else if (disk1->QUEUE->count > disk2->QUEUE->count){
        push(disk2, temp);
        diskNum = 2;
      }
      //if same amount of jobs, pick one at random
      else{
        int num = randNumber(0,100);
        if (num>=50){
          push(disk1, temp);
          diskNum = 1;
        }
        else{
          push(disk2, temp);
          diskNum = 2;
        }
      }
      //print message
      sprintf(string,"%d\tJob %d: Sent to DISK %d from CPU", currentTime, temp, diskNum);
      output(string);
      //add to completed Counter
      //set CPU to IDLE
      cpu->PROCESSED ++;
      cpu->STATUS = IDLE;
      break;
  }
}

//this function simulates a DISK
void sim_disk(component *disk, component *cpu, int currentTime, int DISK_MIN, int DISK_MAX, int diskNum){
  int temp;
  //for logging output
  char string[BUFF_SIZE];

  //get DISK status
  switch (disk->STATUS) {

    //disk is IDLE
    case IDLE:
      //get new wait time
      disk->WAIT_TIME = currentTime + randNumber(DISK_MIN, DISK_MAX);
      //output arrival message
      //sprintf(string, "%d\tJob %d: Arrived at DISK %d", currentTime, disk->QUEUE->front->key, diskNum);
      //output(string);
      //set disk to RUNNING
      disk->STATUS = RUNNING;

    break;

    //disk is RUNNING
    case RUNNING:
      //remove job from DISK queue
      temp = pop(disk);
      //put job back into job queue
      push(cpu, temp);
      //print message
      sprintf(string, "%d\tJob %d: I/O finished on DISK %d, sent back to job queue", currentTime, temp, diskNum);
      output(string);
      //add one to completion counter
      disk->PROCESSED++;
      //set disk status to idle
      disk->STATUS = IDLE;
      break;
  }
}

//this function returns a random number between a min and max
int randNumber(int min, int max){
  return min+(rand() % (max+1));
}

//outputs text to the console and to a log file
void output(char *s){

  //open log file
  FILE *file = fopen(LOGFILE, "a");

  //if file could not be opened
  if (file == NULL){
    //error message
    puts("Error: Could not open log file");
    exit(-1);
  }

  //print message to console
  printf ("%s\n", s);
  //write to log file
  fprintf(file, "%s\n", s);

  //close file
  fclose(file);
}
//outputs the values in the config file to console and log file
void output_config(config *conf){

  //open log file
  FILE *file = fopen(LOGFILE, "a");

  //if file could not be opened
  if (file == NULL){
    //error message
    puts("Error: Could not open log file");
    exit(-1);
  }

  //print config to console
  printf("INIT_TIME %d\nFIN_TIME %d\n", conf->INIT_TIME, conf-> FIN_TIME);
  printf("ARRIVE_MIN %d\nARRIVE_MAX %d\n", conf->ARRIVE_MIN, conf-> ARRIVE_MAX);
  printf("CPU_MIN %d\nCPU_MAX %d\n", conf->CPU_MIN, conf-> CPU_MAX);
  printf("DISK1_MIN %d\nDISK1_MAX %d\n", conf->DISK1_MIN, conf-> DISK1_MAX);
  printf("DISK2_MIN %d\nDISK2_MAX %d\n", conf->DISK2_MIN, conf-> DISK2_MAX);
  printf("QUIT_PROB %d\nSEED %d\n", conf->QUIT_PROB, conf->SEED);
  //print config to log
  fprintf(file, "INIT_TIME %d\nFIN_TIME %d\n", conf->INIT_TIME, conf-> FIN_TIME);
  fprintf(file, "ARRIVE_MIN %d\nARRIVE_MAX %d\n", conf->ARRIVE_MIN, conf-> ARRIVE_MAX);
  fprintf(file, "CPU_MIN %d\nCPU_MAX %d\n", conf->CPU_MIN, conf-> CPU_MAX);
  fprintf(file, "DISK1_MIN %d\nDISK1_MAX %d\n", conf->DISK1_MIN, conf-> DISK1_MAX);
  fprintf(file, "DISK2_MIN %d\nDISK2_MAX %d\n", conf->DISK2_MIN, conf-> DISK2_MAX);
  fprintf(file, "QUIT_PROB %d\nSEED %d\n", conf->QUIT_PROB, conf->SEED);

  //close file
  fclose(file);
}
