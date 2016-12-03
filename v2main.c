#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846
// keep SLOT == 1 for simplicity
#define SLOT 1
#define SIFS SLOT
#define DIFS 2 * SLOT + SIFS
#define DATA_RATE 12
#define BIT_DURATION 1/DATA_RATE
#define INITIAL_CONTENTION_WINDOW 31
#define MAX_CONTENTION_WINDOW 1023
#define RTS_SIZE 20*8*BIT_DURATION
#define CTS_SIZE 14*8*BIT_DURATION
#define ACK_SIZE CTS_SIZE
#define MAX_PACKET_SIZE 100*8*BIT_DURATION
#define BIT 1
#define MIN_STATIONS 1
#define MAX_STATIONS 50
#define PROGRAM_TIME 1000000
#define FILENAME "output_file.txt"


int * get_data_array(int size, int data);
struct program_variables init_program();
struct station * init_stations(int station_count);
int decrease_backoff_counters(struct station * stations, int station_count, int idle_status);
struct channel * init_channel();
int check_and_send_packet(struct station * stations, int station_count, int idle_status);

// variables that exist and change throughout the 
// life of the program
struct program_variables {
    int *rts,
        *cts,
        *ack,
        *packet;
};

struct station {
    int backoff_counter,
        contention_window,
        collisions_in_a_row,
        total_packets_sent;
};

struct channel {
    int idle,
        send_time_remaining;
};

int main(){
    struct program_variables shared = init_program();
    struct channel * medium = init_channel();
    int stations;
    //overwrite file if exists
    FILE * my_file;
    my_file = fopen(FILENAME, "w");
    fprintf(my_file, "0 0 0 0\n");
    fclose(my_file);
    // to get metrics
    for (stations=MIN_STATIONS;stations<MAX_STATIONS;stations++){
        
        int collision_count = 0,
            total_time_sending = 0,
            successful_send_count = 0;
        // initialize stations with a initial contention window
        // and initial backoff counter
        struct station * active_stations = init_stations(stations);

        // run the simulation from time 0 to total time set
        int current_time;
        for (current_time=0;current_time<PROGRAM_TIME;current_time+=SLOT){

            // returns 1 if collision, 0 if no collision
            int collision = decrease_backoff_counters(active_stations, stations, medium->idle);
            if (medium->send_time_remaining){
                medium->send_time_remaining-=SLOT;
                if (!medium->send_time_remaining){
                    medium->idle = 1;
                    int i;
                    for (i=0;i<stations;i++){
                        if (active_stations[i].backoff_counter == -1){
                            active_stations[i].contention_window = INITIAL_CONTENTION_WINDOW;
                            active_stations[i].backoff_counter = SLOT*rand()%active_stations[i].contention_window;
                            active_stations[i].collisions_in_a_row = 0;
                        }
                    }
                }

            }
            if (collision){
                collision_count++;
            } else {
                // if no senders, will return 0, else will set a station to sending and set the medium to no longer idle
                // for the duration of the packet sending
                // NOTE this will NOT get called if there is a collision
                int time_to_send = check_and_send_packet(active_stations, stations, medium->idle);
                total_time_sending += time_to_send;
                if (time_to_send){
                    medium->idle = 0;
                    medium->send_time_remaining = time_to_send;
                    successful_send_count++;
                }
            }
            //getchar();
        }


        // append to file
        printf("\n--- for %d stations ---\n", stations);
        printf("there were %d collisions, %d successful transmissions, %f%% was the total time spent sending\n", collision_count, successful_send_count, ((double)total_time_sending)/PROGRAM_TIME*100);
        int s;
        float mean = 100.0/stations;
        float variance = 0;
        for (s=0;s<stations;s++){
            float percentage_sent = ((double)active_stations[s].total_packets_sent)/successful_send_count*100;
            float this_variance = pow(percentage_sent - mean, 2)/stations;
            variance += this_variance;
            printf("station #%d sent %d packets, which is %f%% of total\n", s, active_stations[s].total_packets_sent, percentage_sent);
        }
        my_file = fopen(FILENAME, "a");
        fprintf(my_file, "%d %f %f %f\n", stations, ((double)total_time_sending/PROGRAM_TIME), ((double)collision_count)/successful_send_count, variance);
        fclose(my_file);
        // free memory that the active_stations pointer
        // was referencing
        free(active_stations);
    }
    system("gnuplot -e \"     set xlabel '# of Stations';\
                              set ylabel 'Ratio';\
                              plot 'output_file.txt' using 2 title 'Throughput' with linespoints,\
                              'output_file.txt' using 3 title 'Collision Probability' with linespoints,\
                              'output_file.txt' using 4 title 'Variance of \% of packets sent' with linespoints;pause -1\"");
    return 0;
}

// defined as double *md in example code
int * get_data_array(int size, int data){
    int i;
    int *data_output = malloc(sizeof(int)*size);
    for (i=0;i<size;i++)
        data_output[i] = data;
    return data_output;
}

struct program_variables init_program(){
    struct program_variables new_program = {
        .rts = get_data_array(RTS_SIZE, BIT),
        .cts = get_data_array(CTS_SIZE, BIT),
        .ack = get_data_array(ACK_SIZE, BIT),
        .packet = get_data_array(MAX_PACKET_SIZE, BIT)
    };
    return new_program;
}

struct station * init_stations (int station_count){
    struct station * stations = (struct station *)malloc(station_count*(sizeof(struct station)));
    int i;
    for(i=0;i<station_count;i++){
        struct station tmp_station = {
            .backoff_counter = SLOT*rand()%INITIAL_CONTENTION_WINDOW + 1,
            .contention_window = INITIAL_CONTENTION_WINDOW,
            .collisions_in_a_row = 0,
            .total_packets_sent = 0
        };
        stations[i] = tmp_station;
    }
    return stations;
}

int decrease_backoff_counters(struct station * stations, int station_count, int idle_status){
    int * requesting_to_send = (int*)calloc(station_count, sizeof(int));
    int number_requesting = 0,
        most_recent_requesting,
        i;
    for(i=0;i<station_count;i++){
        if (stations[i].backoff_counter > 0) // if not currently sending
            stations[i].backoff_counter-=SLOT;
        if (!stations[i].backoff_counter){
            requesting_to_send[i] = 1;
            number_requesting++;
            most_recent_requesting = i;
        }
    }

    // if idle and more than one station wants to send
    int collision_occured = number_requesting > 1 && idle_status;
    
    int cannot_send = number_requesting > 0 && !idle_status;
    // if collision has occurred we need to increase the contention window
    // for the stations that are colliding, we then set new backoff counters
    // for each
    if (collision_occured){
        for (i=0;i<station_count;i++){
            if (requesting_to_send[i]){
                stations[i].contention_window = (stations[i].contention_window + 1) * 2 - 1;
                if (stations[i].contention_window > MAX_CONTENTION_WINDOW)
                    stations[i].contention_window = MAX_CONTENTION_WINDOW;
                stations[i].backoff_counter = SLOT*rand()%stations[i].contention_window;
                stations[i].collisions_in_a_row++;
            }
        }
    }

    if (cannot_send){
        for (i=0;i<station_count;i++){
            if (requesting_to_send[i]){
                stations[i].backoff_counter = SLOT*rand()%stations[i].contention_window;
            }
        }   
    }

    free(requesting_to_send);

    // if more than one requesting, a collision has occured
    return collision_occured;
}

struct channel * init_channel(){
    struct channel * medium = (struct channel *)malloc(sizeof(struct channel));
    medium->idle = 1;
    medium->send_time_remaining = 0;
    return medium;
}

int check_and_send_packet(struct station * stations, int station_count, int idle_status){
    int i,
        time_to_send = 0;
    for(i=0;i<station_count;i++){
        if (!stations[i].backoff_counter){
            if (!idle_status){
                // if channel is not idle, increase backoff counter without setting new contention
                // window size
                stations[i].backoff_counter = SLOT*rand()%stations[i].contention_window;
            } else {
                // -1 means sending
                stations[i].backoff_counter = -1;
                stations[i].total_packets_sent++;
                // determine time it will take to send packet
                time_to_send = DIFS + RTS_SIZE + SIFS + CTS_SIZE + SIFS + rand()%MAX_PACKET_SIZE + SIFS + ACK_SIZE + DIFS;
            }
            break;
        }
    }
    return time_to_send/BIT_DURATION;
}
