#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define NUM_OF_BRANDS NUM_OF_DEALERS
#define NUM_OF_SEMS NUM_OF_DEALERS
#define NUM_OF_DEALERS 4 //Not subject to change
#define NUM_OF_SEGMENTS 3 //Not subject to change
#define NUM_OF_REPS 2
#define NUM_OF_PRIORITIES 4
#define NUM_OF_RESIDENTS 6

struct rep_info;
struct priority;
struct car;

void* dealer(void* arg);
void* representative(void* arg);
void* resident(void* arg);
void update_prices(int* dealer_id);
void initialize_vars();
void cleanup();
char* get_time();

//==================================
char* resident_names[] = {"Polat alemdar", "Heisenberg", "He-man", "Spaydi", "İskeletor", "Mario", "Rina Sawayama", "Charli XCX", "Sansar Salvo", "Charlie Cunningham"};
int prices[NUM_OF_DEALERS][NUM_OF_SEGMENTS]; //initial prices, refer to struct car->price for the car prices later.
int capacities[NUM_OF_DEALERS][NUM_OF_SEGMENTS] = {
    {2, 2, 2},
    {2, 2, 1},
    {2, 1, 1},
    {1, 1, 1}};
struct car*** dealers[NUM_OF_DEALERS]; //all dealers

char* rep_names[] = {"Yigit0", "Can0", "Yigit1", "Can1", "Yigit2", "Can2", "Yigit3", "Can3"};

int LOAN_AMOUNTS[4] = {200000, 300000, 400000, 500000};

sem_t* rep_locks[NUM_OF_DEALERS]; //value incremented to NUM_OF_REPS when queue is empty
sem_t* inv_locks[NUM_OF_DEALERS]; //max 1 for each dealer when queue is empty
char* rep_lock_names[NUM_OF_DEALERS];
char* inv_lock_names[NUM_OF_DEALERS];

sem_t* initialized; //Indicates that dealers and representatives are initialized
int num_of_residents_done = 0;
//==================================

struct rep_info {
    char* name;
    int dealer_id;
};

struct priority{
    int brand; //Brand1 -> 0th dealer etc.
    int segment; //A->0, B->1, C->2
};

struct car{
    int price;
    int is_available;
};

void update_prices(int* dealer_id) {
    int percentage = (rand() % 41) - 20; //-20, 20
    int new_price;
    int init_price;
    //printf("%s | Dealer %d is updating the prices!\n", get_time(), *dealer_id);
    for (int i = 0; i < NUM_OF_SEGMENTS; i++) {
        init_price = dealers[*dealer_id][i][0]->price;
        new_price = init_price * (100.0 + percentage) / 100;
        //if new prices exceed a 200k-500k interval, set the price on the edges
        if (new_price > 500000 ) {
            new_price = 500000;
        } else if (new_price < 200000) {
            new_price = 200000;
        }
        printf("%s | Dealer %d is updating %c-segment car prices from %d to %d.\n", get_time(), *dealer_id, i+65, init_price, new_price);
        for (int j = 0; j < capacities[*dealer_id][i]; j++) {
            struct car* car = dealers[*dealer_id][i][j];
            car->price = new_price;
        }
    }  
}

void* dealer(void* arg) {
    int *id = (int *)arg;
    pthread_t rep_tids[NUM_OF_REPS];
    printf("Dealer %d is being initialized...\n", *id);
    for (int i = 0; i < NUM_OF_REPS; i++) {
        struct rep_info* new_rep = malloc(sizeof(struct rep_info));
        new_rep->dealer_id = *id;
        new_rep->name = rep_names[NUM_OF_REPS*(*id) + i];
        pthread_create(&rep_tids[i], NULL, representative, (void *)new_rep);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(rep_tids[i], NULL);
    }
    //This printf statement is not dynamic.
    printf("Dealer %d is done initializing. Prices: %c-Segment -> %d, %c-Segment -> %d, %c-Segment -> %d\n", *id, 'A', prices[*id][0], 'B', prices[*id][1], 'C', prices[*id][2]);
    sem_post(initialized);

    int update_time = rand() % 30;
    int update_time_2 = 1 + rand() % (30 - update_time); //TODO: ensure two are not the same
    
    //update prices twice a month (twice in 30 seconds)
    while (num_of_residents_done != NUM_OF_RESIDENTS) {
        sleep(update_time);
        update_prices(id);
        sleep(update_time_2);
        update_prices(id);
        sleep(30 - update_time - update_time_2);
        update_time = rand() % 30;
        update_time_2 = 1 + rand() % (30 - update_time); 
    }

    pthread_exit(NULL);
    return NULL;
}

void* representative(void* arg) {
    printf("Representative %s at dealer %d is being initialized...\n", ((struct rep_info*)arg)->name, ((struct rep_info*)arg)->dealer_id);
    sem_post(rep_locks[((struct rep_info*)arg)->dealer_id]);
    
    //TODO: I'm not sure as to what to do here
    pthread_exit(NULL);
    return NULL;
}

void* resident(void* arg) {
    char **name = (char **)arg;
    struct priority** priorities = (struct priority**) malloc(sizeof(struct priority*) * NUM_OF_PRIORITIES);
    int money = LOAN_AMOUNTS[rand() % 5]; //5 refers to the number of elements in LOAN_AMOUNTS, consider assigning dynamically
    printf("Initializing resident %s...\n", *name);
    //initialize priorities
    for (int i = 0; i < NUM_OF_PRIORITIES; i++) {
        struct priority* new_priority = malloc(sizeof(struct priority));
        new_priority->brand = rand() % NUM_OF_BRANDS;
        new_priority->segment = rand() % NUM_OF_SEGMENTS;
        priorities[i] = new_priority;
    }
    printf("Resident %s is initialized with a loan of %d.\n", *name, money);

    int num_tried = 0;
    int curr_priority_index = 0;
    int curr_priority_brand = 0;
    int curr_priority_segment = 0;
    int can_buy = 0;
    
    for (int i = 0; i < NUM_OF_PRIORITIES * 2; i++) { //2 indicates two tries for each car prioritized, consider assigning dynamically
        can_buy = 0;
        curr_priority_brand = priorities[curr_priority_index]->brand;
        curr_priority_segment = priorities[curr_priority_index]->segment;
        
        
        sleep(3 + rand() % 5); //this is not a good way of doing this
        printf("%s | I'm %s and going to buy %c-segment car at dealer %d.\n", get_time(), *name , curr_priority_segment+65, curr_priority_brand);
        sem_wait(rep_locks[curr_priority_brand]); //naming may be confusing, consider changing rep_locks to dealer_locks
        sem_wait(inv_locks[curr_priority_brand]);
        for (int j = 0; j < capacities[curr_priority_brand][curr_priority_segment]; j++) {
            if (!dealers[curr_priority_brand][curr_priority_segment][j]->is_available || dealers[curr_priority_brand][curr_priority_segment][j]->price > money) {
                can_buy = 0;
            } else {
                can_buy = 1;
                dealers[curr_priority_brand][curr_priority_segment][j]->is_available = 0;
                break;
            }
        }
        sleep(1); //Time spent on buying
        if (can_buy) {
            printf("%s | I'm %s and I bought %c-Segment car at dealer %d!!!\n", get_time(), *name, curr_priority_segment + 65, curr_priority_brand);
            sem_post(rep_locks[curr_priority_brand]);
            sem_post(inv_locks[curr_priority_brand]);
            break;
        } else if (!can_buy && num_tried == 0) {
            num_tried++;
            printf("%s | I'm %s and I'm couldn't buy a %c-segment car at dealer %d, this was my first try.\n", get_time(), *name, curr_priority_segment + 65, curr_priority_brand);
        } else if (!can_buy && num_tried == 1) {
            num_tried = 0;
            curr_priority_index++;
            printf("%s | I'm %s and I'm giving up on %c-segment car at dealer %d :(\n", get_time(), *name, curr_priority_segment + 65, curr_priority_brand);
        }
        sem_post(rep_locks[curr_priority_brand]); //naming may be confusing, consider changing rep_locks to dealer_locks
        sem_post(inv_locks[curr_priority_brand]);
        if (!can_buy && i != (NUM_OF_PRIORITIES * 2)) {
            sleep(1 + rand() % 5);
        }  
    }
    num_of_residents_done = num_of_residents_done + 1;
    pthread_exit(NULL);
}

void initialize_vars() {
    //Semaphore names
    //for representatives
    for (int i = 0; i < NUM_OF_SEMS % 1000; i++) {
        char* new_name = malloc(8);
        sprintf(new_name, "YIGIT%03d", i);
        rep_lock_names[i] = new_name;
    }
    //for inventories
    for (int i = 0; i < NUM_OF_SEMS % 1000; i++) {
        char* new_name = malloc(8);
        sprintf(new_name, "CANBI%03d", i);
        inv_lock_names[i] = new_name;
    }

    //initial prices
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        for (int j = 0; j < NUM_OF_SEGMENTS; j++) {
            prices[i][j] = 200000 + rand() % 300000;
        }
    }

    //cars at dealers
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        struct car*** segments = (struct car***) malloc(sizeof(struct car**) * 3); //one dealer, list of segments
        for (int j = 0; j < NUM_OF_SEGMENTS; j++) {
            struct car** list = (struct car**) malloc(sizeof(struct car*) * 4); //TODO: instead of 4, there are fixed sizes for these
            for (int k = 0; k < capacities[i][j]; k++) {
                struct car* new_car = malloc(sizeof(struct car));
                new_car->price = prices[i][j];
                new_car->is_available = 1;
                list[k] = new_car;
            }
            segments[j] = list;
        }  
        dealers[i] = segments;
    }
}

void debug() {
    //prices
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%d ", prices[i][j]);
        }
        printf("\n");
    }
    
    //dealer inventories
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            for (int k = 0; k < capacities[i][j]; k++)
            {
                printf("%d ", dealers[i][j][k]->price);
            }
            printf("\n");
        }
        printf("\n");
    }
}

/* 
 * Clears named semaphores and variables
 */
void cleanup() {
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        sem_close(rep_locks[i]);
        sem_unlink(rep_lock_names[i]);
        sem_close(inv_locks[i]);
        sem_unlink(inv_lock_names[i]);
    }
    sem_close(initialized);
    sem_unlink("TEST000");
}

char* get_time() {
    char* time_str = malloc(8); //HH:MM:SS
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(time_str, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    return time_str;
}

int main() {
    //TODO: Check if constants are not absurdly large
    printf("\n=========================\n");
    initialize_vars();
    //debug();
    
    initialized = sem_open("TEST000", O_EXCL|O_CREAT, 0644, 0);
    if (initialized == SEM_FAILED) {
        printf("Semaphores already exists or could not be created, removing the current semaphores...\nPlease restart the program.\n");
        cleanup();
        exit(0);
    }
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        rep_locks[i] = sem_open(rep_lock_names[i], O_EXCL|O_CREAT, 0644, 0);
        inv_locks[i] = sem_open(inv_lock_names[i], O_EXCL|O_CREAT, 0644, 1);

        if (rep_locks[i] == SEM_FAILED || inv_locks[i] == SEM_FAILED) {
            printf("Semaphores already exists or could not be created, removing the current semaphores...\nPlease restart the program.\n");
            cleanup();
            exit(0);
        }
    }

    pthread_t res_threads[NUM_OF_RESIDENTS];
    pthread_t dealer_threads[NUM_OF_DEALERS];
    //IMPORTANT: Representative threads are created at dealer()
    
    int* new_id = 0;
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        new_id = malloc(sizeof(int));
        *new_id = i;
        pthread_create(&dealer_threads[i], NULL, dealer, new_id); //TODO: assign dealer id depending on "i" here
    }
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        sem_wait(initialized);
    }
    for (int i = 0; i < NUM_OF_RESIDENTS; i++) {
        pthread_create(&res_threads[i], NULL, resident, &resident_names[i % (sizeof(resident_names) / sizeof(*resident_names))]);
    }

    

    for (int i = 0; i < NUM_OF_RESIDENTS; i++) {
        pthread_join(res_threads[i], NULL);
    }
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        //pthread_join(dealer_threads[i], NULL);
        pthread_cancel(dealer_threads[i]);
    }

    cleanup();
    return 0;
}