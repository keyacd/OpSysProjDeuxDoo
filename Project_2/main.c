#define _POSIX_SOURCE
#define _SVID_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>

#include "msg.h"
#include "process.h"

// Darien Keyack (661190088) and Corwin Aucoin (661178786)

struct Partition {
  int x;
  int y;
  int size;
};

int remove_from_mem(char id, char** memory){
  int i, j;
  int count = 0;
  for(j = 0; j < 8; j++){
    for(i = 0; i < 32; i++){
      if(memory[i][j] == id){
        memory[i][j] = '.';
        count++;
      }
    }
  }
  return count;
}

int get_open_mem(char** memory){
  int i, j;
  int count = 0;
  for(j = 0; j < 8; j++){
    for(i = 0; i < 32; i++){
      if(memory[i][j] == '.')
        count++;
    }
  }
  return count;
}

int next_fit(int part_num, struct Partition * partitions, char** memory, struct Process proc, int * px, int * py) {
  int zz = 0;
  int tick = 0;
  for (zz = 0; zz < part_num; zz++) {
    if (partitions[zz].size >= proc.memory) {
      int i = partitions[zz].x;
      int j = partitions[zz].y;
      int count = proc.memory;
      while (count > 0) {
        tick = 1;
        memory[i][j] = proc.id;
        i++;
        if (i > 31) {
          i = 0;
          j++;
        }
        count--;
      }
      (*px) = i;
      (*py) = j;
      break;
    }
  }
  
  if (tick == 0) return -1;
  else return 0;
}

int best_fit(int part_num, struct Partition * partitions, char** memory, struct Process proc, int * px, int * py) {
  int zz = 0;
  int size = 999;
  int loc = -1;
  for (zz = 0; zz < part_num; zz++) {
    if (partitions[zz].size - proc.memory < size && partitions[zz].size - proc.memory > -1) {
      loc = zz;
      size = partitions[zz].size - proc.memory;
    }
  }
  if (loc == -1) return -1;  // There's no space that works
  int i = partitions[loc].x;
  int j = partitions[loc].y;
  int count = proc.memory;
  while (count > 0) {
    memory[i][j] = proc.id;
    i++;
    if (i > 31) {
      i = 0;
      j++;
    }
    count--;
  }
  (*px) = i;
  (*py) = j;
  return 0;
}

int worst_fit(int part_num, struct Partition * partitions, char** memory, struct Process proc, int * px, int * py) {
  int zz = 0;
  int size = -1;
  int loc = -1;
  for (zz = 0; zz < part_num; zz++) {
    if (partitions[zz].size - proc.memory > size && partitions[zz].size - proc.memory > -1) {
      loc = zz;
      size = partitions[zz].size - proc.memory;
    }
  }
  if (loc == -1) return -1;  // There's no space that works
  int i = partitions[loc].x;
  int j = partitions[loc].y;
  int count = proc.memory;
  while (count > 0) {
    memory[i][j] = proc.id;
    i++;
    if (i > 31) {
      i = 0;
      j++;
    }
    count--;
  }
  (*px) = i;
  (*py) = j;
  return 0;
}

int first_fit(char** memory, struct Process proc) {
  int freemem = 0;
  int i = 0;
  int j = 0;
  int to_fill = proc.memory;
  for (j = 0; j < 8; j++) {
    for (i = 0; i < 32; i++) {
      if (memory[i][j] == '.') freemem++;
    }
  }
  if (freemem < proc.memory) return -1;
  for (j = 0; j < 8; j++) {
    for (i = 0; i < 32; i++) {
      if ((memory[i][j] == '.') && (freemem > 0) && (to_fill > 0)) {
        memory[i][j] = proc.id;
        freemem--;
        to_fill--;
      }
    }
  }
  return 0;
}

int next_get_parts(char **memory, struct Partition *partitions, int px, int py) {
  int i, j, count, parts;
  count = 0;
  parts = 0;
  for (j = 0; j < 8; j++) {
    for (i = 0; i < 32; i++) {
      if(j + i == 0){
        if((px) + (py) != 0){
          i = (px);
          j = (py);
        }
      }
          
      if (memory[i][j] == '.'){
        if(count == 0){
          parts++;
          partitions[parts - 1].x = i;
          partitions[parts - 1].y = j;
        }
        count++;
      } 
      else if (count != 0) {
        partitions[parts - 1].size = count;
        count = 0;
      }
    }
  }
  if(count > 0){
    partitions[parts - 1].size = count;
    count = 0;
  }
  for (j = 0; j < 8; j++) {
    for (i = 0; i < 32; i++) {
      if (memory[i][j] == '.'){
        if(count == 0){
          parts++;
          partitions[parts - 1].x = i;
          partitions[parts - 1].y = j;
          
        }
        count++;
      } 
      else if (count != 0) {
        partitions[parts - 1].size = count;
        count = 0;
      }
    }
  }
  if(count > 0){
    partitions[parts - 1].size = count;
    count = 0;
  }
  return parts;
}
int get_parts(char **memory, struct Partition *partitions, int px, int py) {
  int i, j, count, parts;
  count = 0;
  parts = 0;
  for (j = 0; j < 8; j++) {
    for (i = 0; i < 32; i++) {
      if(j + i == 0){
        if((px) + (py) != 0){
          i = (px);
          j = (py);
        }
      }
          
      if (memory[i][j] == '.'){
        if(count == 0){
          if(i + j != 0){
            if(j == 0){
              if(memory[i - 1][j] == '.'){
                continue;
              }
            }
            else{
              if(memory[i][j - 1] == '.'){
                continue;
              }
            }
          }
          parts++;
          partitions[parts - 1].x = i;
          partitions[parts - 1].y = j;
        }
        count++;
      } 
      else if (count != 0) {
        partitions[parts - 1].size = count;
        count = 0;
      }
    }
  }
  if(count > 0){
    partitions[parts - 1].size = count;
    count = 0;
  }
  for (j = 0; j < 8; j++) {
    for (i = 0; i < 32; i++) {
      if (memory[i][j] == '.'){
        if(count == 0){
          parts++;
          partitions[parts - 1].x = i;
          partitions[parts - 1].y = j;
          
        }
        count++;
      } 
      else if (count != 0) {
        partitions[parts - 1].size = count;
        count = 0;
      }
    }
  }
  if(count > 0){
    partitions[parts - 1].size = count;
    count = 0;
  }
  return parts;
}

int defrag(char **memory){
  int i, j;
  struct Partition ds;// = (struct Partition)malloc(sizeof(struct Partition));
  ds.x = 0;
  ds.y = 0;
  ds.size = 0;
  int count;
  for(j = 0; j < 8; j++){
    for(i = 0; i < 32; i++){
      if(memory[i][j] == '.' && ds.size == 0){
        ds.size++;
        ds.x = i;
        ds.y = j;
      }
      else if(memory[i][j] == '.'){
        ds.size++;
      }
      else if(memory[i][j] != '.' && ds.size != 0){
        memory[ds.x][ds.y] = memory[i][j];
        memory[i][j] = '.';
        j = 0;
        i = 0;
        ds.size = 0;
        count++;
      }
    }
  }
  
  return count;
}

int main(int argc, char * argv[]) {
  // Open input file
  if (argc < 2) msg_error("argc < 2");
  FILE *input = fopen(argv[1], "r");
  if (input == NULL) msg_error("failed to open input file");
  
  // Read number of processes from input file
  char line[256];
  fgets(line, 256, input);
  if (line == NULL) msg_error("input file too short");
  int num_proc = atoi(line);
  if ((num_proc < 1) || (num_proc > 26)) msg_error("invalid number of processes");
  
  // Read in raw processes from input file
  char** proc_raw = (char**) calloc(num_proc, sizeof(char*));
  int i = 0;
  while ((fgets(line, 256, input) != NULL) && (i < num_proc)) {
      if (line[0] != ' ' && line[0] != '#') {
          proc_raw[i] = malloc(256 * sizeof(char));
          strncpy(proc_raw[i], line, 255);
          i++;
      }
  }
  if ((i < num_proc) || (fgets(line, 256, input) != NULL)) msg_error("invalid number of processes");
  fclose(input);
  
  // Create processes from raw data
  struct Process* proc_array = (struct Process*) malloc(num_proc * sizeof(struct Process));
  int j;
  for (i = 0; i < num_proc; i++) {
      proc_array[i].id = proc_raw[i][0];
      proc_array[i].memory = get_seg(2, 256, proc_raw[i], ' ');
      j = 2 + 1 + int_len(proc_array[i].memory);
      proc_array[i].list_size = 0;
      proc_array[i].arrive_times = (int*) calloc(5, sizeof(int));
      proc_array[i].run_times = (int*) calloc(5, sizeof(int));
      proc_array[i].adj_arrive = -1;
      while ((j < 256) && (proc_raw[i][j] != '\n') && (proc_raw[i][j] != '\0') && (proc_array[i].list_size < 5)) {
        proc_array[i].arrive_times[proc_array[i].list_size] = get_seg(j, 256, proc_raw[i], '/');
        j += 1 + int_len(proc_array[i].arrive_times[proc_array[i].list_size]);
        proc_array[i].run_times[proc_array[i].list_size] = get_seg(j, 256, proc_raw[i], ' ');
        j += 1 + int_len(proc_array[i].run_times[proc_array[i].list_size]);
        proc_array[i].list_size += 1;
      }
      free(proc_raw[i]);
  }
  free(proc_raw);
  
  // Initialize memory
  char** memory = (char**) calloc(32, sizeof(char*));
  for (i = 0; i < 32; i++) {
    memory[i] = malloc(8 * sizeof(char));
    for (j = 0; j < 8; j++) memory[i][j] = '.';
  }
  struct Partition partitions[12] = {};
  
  struct Process* holding_q = (struct Process*) malloc(sizeof(struct Process));
  int q_size = 0;
  
  // Contiguous -- Next-Fit
  int t = 0;
  int rt = 0;
  int px = 0;
  int py = 0;
  int defrag_stop = -1;
  int num_left = 0;
  for(i = 0; i < num_proc; i++){
    for(j = 0; j < proc_array[i].list_size; j++)
      num_left++;
  }
  msg_sim_start(t, "Contiguous -- Next-Fit");
  while (num_left > 0) { 
    for (i = 0; i < num_proc; i++) {
      for (j = 0; j < proc_array[i].list_size; j++) {
        if (proc_array[i].arrive_times[j] + proc_array[i].run_times[j] == t || proc_array[i].adj_arrive == t) {
            int temp = remove_from_mem(proc_array[i].id, memory);
            proc_array[i].adj_arrive = -1;
            num_left--;
            if(temp > 0)
              msg_remove(rt, proc_array[i].id, memory);
        }
        if (proc_array[i].arrive_times[j] == t) {
          if( t < defrag_stop){
            proc_array[i].adj_arrive = defrag_stop + proc_array[i].run_times[j];
            q_size++;
            holding_q = (struct Process*) realloc(holding_q, q_size * sizeof(struct Process));
            holding_q[q_size - 1] = proc_array[i];
            
          }
          else if(t == defrag_stop){
            int zz;
            for(zz = 0; zz < q_size; zz++){
              int num_parts = next_get_parts(memory, partitions, px, py);
              int result = next_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
              if (result == -1) {
                if(get_open_mem(memory) >= proc_array[i].memory){
                  defrag_stop = defrag(memory);
                  rt += defrag_stop;
                  defrag_stop += t;
                  num_parts = next_get_parts(memory, partitions, px, py);
                  result = next_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
                  msg_place(rt, proc_array[i].id, memory);
                }
                else
                  msg_skip(rt, proc_array[i].id);
              }
            }
          }
          else{
            msg_arrive(rt, proc_array[i].id, proc_array[i].memory);
            int num_parts = next_get_parts(memory, partitions, px, py);
            int result = next_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
            if (result == -1) {
              if(get_open_mem(memory) >= proc_array[i].memory){
                msg_defrag_start(rt, proc_array[i].id);
                defrag_stop = defrag(memory);
                rt += defrag_stop;
                //TBA: Fetch a list of chars moved
                //msg_defrag_end(rt, defrag_stop, , memory)
                defrag_stop += t;
                num_parts = next_get_parts(memory, partitions, px, py);
                result = next_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
                msg_place(rt, proc_array[i].id, memory);
              }
              else
                msg_skip(rt, proc_array[i].id);
            }
            else msg_place(rt, proc_array[i].id, memory);
          }
        }
      }
    }
    t++;
    rt++;
  }
  msg_sim_end(rt - 1, "Contiguous -- Next-Fit");
  // Contiguous -- Best-Fit
  for (i = 0; i < 8; i++) for (j = 0; j < 32; j++) memory[i][j] = '.';
  t = 0;
  rt = 0;
  px = 0;
  py = 0;
  i = 0;
  j = 0;
  num_left = 0;
  defrag_stop = -1;
  for(i = 0; i < num_proc; i++){
    for(j = 0; j < proc_array[i].list_size; j++){
      //printf("%c : %d / %d\n", proc_array[i].id, proc_array[i].arrive_times[j], proc_array[i].run_times[j]);
      num_left++;
    }
  }
  printf("\n");
  msg_sim_start(t, "Contiguous -- Best-Fit");
  while (num_left > 0) { 
    for (i = 0; i < num_proc; i++) {
      for (j = 0; j < proc_array[i].list_size; j++) {
        if (proc_array[i].arrive_times[j] + proc_array[i].run_times[j] == t || proc_array[i].adj_arrive == t) {
            int temp = remove_from_mem(proc_array[i].id, memory);
            proc_array[i].adj_arrive = -1;
            num_left--;
            if(temp > 0)
              msg_remove(rt, proc_array[i].id, memory);
        }
        if (proc_array[i].arrive_times[j] == t) {
          if( t < defrag_stop){
            proc_array[i].adj_arrive = defrag_stop + proc_array[i].run_times[j];
            q_size++;
            holding_q = (struct Process*) realloc(holding_q, q_size * sizeof(struct Process));
            holding_q[q_size - 1] = proc_array[i];
          }
          else if(t == defrag_stop){
            int zz;
            for(zz = 0; zz < q_size; zz++){
              int num_parts = get_parts(memory, partitions, px, py);
              int result = best_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
              if (result == -1) {
                if(get_open_mem(memory) >= proc_array[i].memory){
                  defrag_stop = defrag(memory);
                  rt += defrag_stop;
                  defrag_stop += t;
                  num_parts = get_parts(memory, partitions, px, py);
                  result = best_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
                  msg_place(rt, proc_array[i].id, memory);
                }
                else
                  msg_skip(rt, proc_array[i].id);
              }
            }
          }
          else{
            msg_arrive(rt, proc_array[i].id, proc_array[i].memory);
            int num_parts = get_parts(memory, partitions, px, py);
            int result = best_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
            if (result == -1) {
              if(get_open_mem(memory) >= proc_array[i].memory){
                defrag_stop = defrag(memory);
                rt += defrag_stop;
                defrag_stop += t;
                num_parts = get_parts(memory, partitions, px, py);
                result = best_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
                msg_place(rt, proc_array[i].id, memory);
              }
              else
                msg_skip(rt, proc_array[i].id);
            }
            else msg_place(rt, proc_array[i].id, memory);
          }
        }
      }
    }
    t++;
    rt++;
  }
  msg_sim_end(rt - 1, "Contiguous -- Best-Fit");
  // Worst-Fit
  for (i = 0; i < 8; i++) for (j = 0; j < 32; j++) memory[i][j] = '.';
  t = 0;
  rt = 0;
  px = 0;
  py = 0;
  i = 0;
  j = 0;
  num_left = 0;
  defrag_stop = -1;
  for(i = 0; i < num_proc; i++){
    for(j = 0; j < proc_array[i].list_size; j++){
      //printf("%c : %d / %d\n", proc_array[i].id, proc_array[i].arrive_times[j], proc_array[i].run_times[j]);
      num_left++;
    }
  }
  printf("\n");
  msg_sim_start(t, "Contiguous -- Worst-Fit");
  while (num_left > 0) { 
    for (i = 0; i < num_proc; i++) {
      for (j = 0; j < proc_array[i].list_size; j++) {
        if (proc_array[i].arrive_times[j] + proc_array[i].run_times[j] == t || proc_array[i].adj_arrive == t) {
            int temp = remove_from_mem(proc_array[i].id, memory);
            proc_array[i].adj_arrive = -1;
            num_left--;
            if(temp > 0)
              msg_remove(rt, proc_array[i].id, memory);
        }
        if (proc_array[i].arrive_times[j] == t) {
          if( t < defrag_stop){
            proc_array[i].adj_arrive = defrag_stop + proc_array[i].run_times[j];
            q_size++;
            holding_q = (struct Process*) realloc(holding_q, q_size * sizeof(struct Process));
            holding_q[q_size - 1] = proc_array[i];
          }
          else if(t == defrag_stop){
            int zz;
            for(zz = 0; zz < q_size; zz++){
              int num_parts = get_parts(memory, partitions, px, py);
              int result = worst_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
              if (result == -1) {
                if(get_open_mem(memory) >= proc_array[i].memory){
                  defrag_stop = defrag(memory);
                  rt += defrag_stop;
                  defrag_stop += t;
                  num_parts = get_parts(memory, partitions, px, py);
                  result = worst_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
                  msg_place(rt, proc_array[i].id, memory);
                }
                else
                  msg_skip(rt, proc_array[i].id);
              }
            }
          }
          else{
            msg_arrive(rt, proc_array[i].id, proc_array[i].memory);
            int num_parts = get_parts(memory, partitions, px, py);
            int result = worst_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
            if (result == -1) {
              if(get_open_mem(memory) >= proc_array[i].memory){
                defrag_stop = defrag(memory);
                rt += defrag_stop;
                defrag_stop += t;
                num_parts = get_parts(memory, partitions, px, py);
                
                result = worst_fit(num_parts, partitions, memory, proc_array[i], &px, &py);
                msg_place(rt, proc_array[i].id, memory);
              }
              else
                msg_skip(rt, proc_array[i].id);
            }
            else msg_place(rt, proc_array[i].id, memory);
          }
        }
      }
    }
    t++;
    rt++;
  }
  msg_sim_end(rt - 1, "Contiguous -- Worst-Fit");
  // Non-contiguous
  for (i = 0; i < 8; i++) for (j = 0; j < 32; j++) memory[i][j] = '.';
  t = 0;
  rt = 0;
  px = 0;
  py = 0;
  i = 0;
  j = 0;
  num_left = 0;
  for(i = 0; i < num_proc; i++){
    for(j = 0; j < proc_array[i].list_size; j++){
      //printf("%c : %d / %d\n", proc_array[i].id, proc_array[i].arrive_times[j], proc_array[i].run_times[j]);
      num_left++;
    }
  }
  printf("\n");
  msg_sim_start(t, "Non-contiguous");
  while (num_left > 0) { 
    for (i = 0; i < num_proc; i++) {
      for (j = 0; j < proc_array[i].list_size; j++) {
        if (proc_array[i].arrive_times[j] + proc_array[i].run_times[j] == t) {
            int temp = remove_from_mem(proc_array[i].id, memory);
            proc_array[i].adj_arrive = -1;
            num_left--;
            if(temp > 0)
              msg_remove(rt, proc_array[i].id, memory);
        }
        if (proc_array[i].arrive_times[j] == t) {
          msg_arrive(rt, proc_array[i].id, proc_array[i].memory);
          int result = first_fit(memory, proc_array[i]);
          if(result != -1)
            msg_place(rt, proc_array[i].id, memory);
          else
            msg_skip(rt, proc_array[i].id);
        }
      }
    }
    t++;
    rt++;
  }
  msg_sim_end(rt - 1, "Non-contiguous");
  
  // Free & exit
  free(proc_array);
  // free memory
  return EXIT_SUCCESS;
}