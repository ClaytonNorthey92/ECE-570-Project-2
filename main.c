#include <stdio.h>
#include <stdlib.h>

#define STATION_COUNT 8
#define INITIAL_CONTENTION_WINDOW 31
#define ACK 14*8 // 14 bytes
#define RTS 20*8 // 20 bytes
#define CTS 14*8 // 14 bytes
#define MAX_PACKET_SIZE 2347
#define SENDING -1
#define DATA_RATE 500 // 500 bits/s <-- very slow, but good for proof of concept
#define TOTAL_TIME 10000

int contention_window_size = INITIAL_CONTENTION_WINDOW;

struct station {
  int backoff_time;
};

struct medium {
  int idle_status; // 1 for idle, 0 for not idle
  int tranmission_time_remaining;
  int station_sending;
  int total_time_transmitting;
  int collision_count;
  int total_bits_sent;
  int total_packets_delivered;
  int packets_per_station [8];
};

struct station * create_stations();
struct medium create_medium();
int is_medium_idle(struct medium medium_to_check);
void decrease_backoff_times(struct station * stations);
void request_to_send(struct station * stations, struct medium * network_medium);
int create_backoff_time();
void pause();
void print_collision(int requesting [], struct station * stations, char * message);
void print_divider();
void print_cannot_request(struct station stations[], int requesting[]);
void print_packets_sent(struct medium network_medium);

int main() {
  struct station * stations = create_stations();
  struct medium network_medium = create_medium();

  int program_time = 0;
  while (program_time < TOTAL_TIME){
    printf("-----\nCurrent Program Time: %d \n", program_time);
    printf("-----\n");
    
    decrease_backoff_times(stations);
    request_to_send(stations, &network_medium);
    
    print_divider();
    //pause();
    program_time++;
  }

  printf("\nTotal Program Time: %d\n", program_time);
  printf("Total Transmission Time: %d\n", network_medium.total_time_transmitting);
  printf("Total bits sent: %d giving a throughput of %f bits/second\n", network_medium.total_bits_sent, ((double)network_medium.total_bits_sent)/program_time);
  printf("%d collisions occured with probability of %f\n", network_medium.collision_count, ((double)network_medium.collision_count)/program_time);
  print_packets_sent(network_medium);
  return 0;
}

struct station * create_stations(){
  static struct station stations [STATION_COUNT];

  for (int i=0;i<STATION_COUNT;i++){
    stations[i].backoff_time = create_backoff_time();
  }

  return stations;
}

struct medium create_medium(){
  struct medium network_medium;
  network_medium.idle_status = 1;
  network_medium.collision_count = 0;
  network_medium.total_time_transmitting = 0;
  network_medium.total_bits_sent = 0;
  network_medium.total_packets_delivered = 0;
  for (int i=0;i<STATION_COUNT;i++){
    network_medium.packets_per_station[i] = 0;
  }
  return network_medium;
}

int is_medium_idle(struct medium medium_to_check){
  return medium_to_check.idle_status;
}

void decrease_backoff_times(struct station * stations){
  for (int i=0;i<STATION_COUNT;i++){
    if (stations[i].backoff_time == SENDING){
      printf("station %d is currently sending\n", i);
    } else {
      printf("decreasing backoff counter for station %d from %d --> %d\n", i, stations[i].backoff_time, stations[i].backoff_time - 1);
      stations[i].backoff_time--;
    }
  }
}

void request_to_send(struct station * stations, struct medium * network_medium){
  int requesting [8] = {0,0,0,0,0,0,0,0};
  int requesting_count = 0;
  int requesting_station; // only use if requesting count == 1
  for (int i=0;i<STATION_COUNT;i++){
    if (stations[i].backoff_time == 0){
      requesting[i] = 1;
      requesting_station = i;
      requesting_count++;
    }
  }
  printf("requesting count %d\n", requesting_count);
  int is_idle = is_medium_idle(*network_medium);
    printf("idle status %d\n", is_idle);
  if (is_idle && requesting_count==1){ // if we can send and only 1 wants to send
    printf("\n !!! station %d won the channel and is able to send !!! \n\n", requesting_station);
    stations[requesting_station].backoff_time = SENDING;
    int total_data_to_send = rand()%MAX_PACKET_SIZE + RTS + CTS + ACK;
    network_medium->total_bits_sent += total_data_to_send;
    int sending_time = total_data_to_send / DATA_RATE;
    if (total_data_to_send % DATA_RATE != 0){
      sending_time ++;
    }
    network_medium->idle_status = 0;
    network_medium->tranmission_time_remaining = sending_time;
    network_medium->station_sending = requesting_station;
    network_medium->packets_per_station[requesting_station] ++;
    network_medium->total_packets_delivered++;
  } else if (is_idle && requesting_count > 1){ // if we can send but more than 1 wants to send
    network_medium->collision_count++;
    print_collision(requesting, stations, "tried to transmit at the same time, they get new backoff counters");
  } else if (is_idle){ // we cannot send
    printf("\n nobody wants to send at the moment\n\n");
  } else if (!is_idle && requesting_count > 0){
    print_cannot_request(stations, requesting);
  }

  is_idle = is_medium_idle(*network_medium);
  if (!is_idle){
    printf("\n --- time remaining in transmision: %d --- \n", network_medium->tranmission_time_remaining);
    network_medium->total_time_transmitting++;
    network_medium->tranmission_time_remaining--;
    if (network_medium->tranmission_time_remaining == 0){
      network_medium->idle_status = 1;
      stations[network_medium->station_sending].backoff_time = create_backoff_time();
    }
  }

}

int create_backoff_time(){
  return (rand()%contention_window_size) + 1;
}

void pause(){
  getchar();
}

void print_collision(int requesting [], struct station * stations, char * message){
    printf("\n !!! there has been a collision, stations: ");
    for (int i=0;i<STATION_COUNT;i++){
      if (requesting[i]){
        printf("%d,", i);
        stations[i].backoff_time = create_backoff_time();
      }
    }
    printf(" %s !!!\n\n", message);
}

void print_divider(){
  printf("\n----------------------------------------------------------------------------------------------------------------\n");
}

void print_cannot_request(struct station stations[], int requesting[]){
      printf("\n !!! stations ");
    for (int i=0;i<STATION_COUNT;i++){
      if (requesting[i]){
        printf("%d,", i);
        stations[i].backoff_time = create_backoff_time();
      }
    }
    printf(" cannot transmit because the medium is not idle, they get a new backoff counter !!!\n\n");
}

void print_packets_sent(struct medium network_medium){
  printf("total packets sent: %d\n", network_medium.total_packets_delivered);
  for (int i=0;i<STATION_COUNT;i++){
    printf("station %d sent %d packets, which is %f%% of total\n", i, network_medium.packets_per_station[i],
           ((double)network_medium.packets_per_station[i])/network_medium.total_packets_delivered*100);
  }
}
