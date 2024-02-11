#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 finish_time;
  u32 remaining_time;
  int time_first_run;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

bool has_finished(struct process* data, u32 size){
  struct process* p = data;
  bool res = false;
  for(int i = 0; i < size; i++){
    if(p->remaining_time != 0){
      res = true;
    }
    p++;
  }
  return res;
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  //If invalid quantum length
  if(quantum_length <= 0){
    printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
    printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

    free(data);
    return 0;
  }
  
  //Loop through processes, init remaining time to burst time, finish time to 0 and time first run to -1
  struct process* p;
  p = data;
  for(int i = 0; i < size; i++){
    p->remaining_time = p->burst_time;
    p->finish_time = 0;
    p->time_first_run = -1;
    p++;
  }
  
  //Start the simulation
  int t = 0;
  int curr_slice = 0;
  int process_count = 0;
  struct process* curr_p = NULL;
  
  while(has_finished(data, size)){
    // printf("Time %i...\n", t);
    //Check if anyone has arrived, if so add them to the Q
    p = data;
    for(int i = 0; i < size; i++){
      if(process_count == size){
        break;
      }
      if(p->arrival_time == t){
        TAILQ_INSERT_TAIL(&list, p, pointers);
        process_count++;
      }
      p++;
    }
    //Check if we have a current process and it has used up its time slice
    if(curr_p != NULL && curr_slice == 0){
      //add it back to the Q and remove it from CPU (set curr_p to null)
      TAILQ_INSERT_TAIL(&list, curr_p, pointers);
      curr_p = NULL;
    }
    //If there is no current process try to get one
    if(curr_p == NULL){
      struct process* np;
      np = TAILQ_FIRST(&list);
      if(np != NULL){
        //Remove that process and make it the current process
        TAILQ_REMOVE(&list, np, pointers);
        // printf("Popped off process %i\n", np->pid);
        curr_p = np;
        //Initialize its time slice to quantum length and record its first run time if it hasnt been run yet
        curr_slice = quantum_length;
        if(curr_p->time_first_run < 0){
          curr_p->time_first_run = t;
        }
      }
    }
    //If there is still no current process increment time, pass
    if(curr_p == NULL){
      // printf("Passing\n");
      t++;
      continue;
    }
    //Run our current process if possible
    if(curr_slice > 0){
      curr_slice--;
      curr_p->remaining_time--;
    }
    //Before we move over to the next iteration/time, check if we'll be finished at this next time, if not we just continue
    if(curr_p->remaining_time == 0){
      // printf("we've finished process %u\n", curr_p->pid);
      //If we will have finished, record finish time, and go ahead and jump off the CPU and also reset curr_slice
      curr_p->finish_time = t+1;
      curr_p = NULL;
      curr_slice = 0;
    }
// DEBUGGING
    // struct process* temp_ptr;
    // printf("Queue\n____________\n");
    // TAILQ_FOREACH(temp_ptr, &list, pointers){
    //   printf("P %i\n", temp_ptr->pid);
    // }
    // printf("____________\n");
//DEBUGGING
    t++;
  }
  p = data;
  for(int i = 0; i < size; i++){
    // printf("Process %i, Burst time: %i, Remaining time: %i, Finish Time: %i Run Time: %i\n",p->pid, p->burst_time, p->remaining_time, p->finish_time, p->time_first_run);
    total_waiting_time += p->finish_time - p->arrival_time - p->burst_time;
    total_response_time += p->time_first_run - p->arrival_time;
    p++;
  }
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
