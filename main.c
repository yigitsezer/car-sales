#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>

//If you are to change any of these, shape CAPACITIES accordingly
#define NUM_OF_BRANDS NUM_OF_DEALERS
#define NUM_OF_SEMS NUM_OF_DEALERS
#define NUM_OF_DEALERS 4
#define NUM_OF_SEGMENTS 3
#define NUM_OF_REPS 2
#define NUM_OF_PRIORITIES 4
#define NUM_OF_RESIDENTS 6

struct rep_info;
struct priority;
struct car;

void* dealer(void* arg);
void* representative(void* arg);
void* resident(void* arg);
void initialize_vars();
void cleanup();

//==================================

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

void* dealer(void* arg) {
    int *id = (int *)arg;
    pthread_t rep_tids[NUM_OF_REPS];
    for (int i = 0; i < NUM_OF_REPS; i++) {
        struct rep_info* new_rep = malloc(sizeof(struct rep_info));
        new_rep->dealer_id = *id;
        new_rep->name = rep_names[NUM_OF_REPS*(*id) + i];
        //printf("new representative %s at %d\n", new_rep->name, new_rep->dealer_id);
        pthread_create(&rep_tids[i], NULL, representative, (void *)new_rep);
    }


    //update prices twice a month
    //sem_wait()


    for (int i = 0; i < 2; i++) {
        pthread_join(rep_tids[i], NULL);
    }
    printf("I'm dealer %d\n", *id);
    sem_post(initialized);
    pthread_exit(NULL);
    return NULL;
}

void* representative(void* arg) {
    //handle customers (sell or not)
    //struct rep_info info = (struct rep_info)arg;
    //char **name = (char **)arg;
    //int *dealer_id = (int *)arg
    printf("Im representative %s at %d\n", ((struct rep_info*)arg)->name, ((struct rep_info*)arg)->dealer_id);
    sem_post(rep_locks[((struct rep_info*)arg)->dealer_id]);
    
    //TODO: I'm not sure as to what to do here
    pthread_exit(NULL);
    return NULL;
}

void* resident(void* arg) {
    char **name = (char **)arg;
    struct priority** priorities = (struct priority**) malloc(sizeof(struct priority*) * NUM_OF_PRIORITIES);
    int money = LOAN_AMOUNTS[rand() % 5]; //5 refers to the number of elements in LOAN_AMOUNTS, consider assigning dynamically

    //initialize priorities
    for (int i = 0; i < NUM_OF_PRIORITIES; i++) {
        struct priority* new_priority = malloc(sizeof(struct priority));
        new_priority->brand = rand() % NUM_OF_BRANDS;
        new_priority->segment = rand() % NUM_OF_SEGMENTS;
        priorities[i] = new_priority;
    }
    
    //debug for resident priorities
    /*
    for (int i = 0; i < 4; i++) {
        //printf("IM %s | priority %d | brand: %d | segment: %d\n", *name ,i+1, priorities[i]->brand, priorities[i]->segment);
    }
    */

    int num_tried = 0;
    int curr_priority_index = 0;
    int curr_priority_brand = 0;
    int curr_priority_segment = 0;
    int can_buy = 0;
    
    for (int i = 0; i < NUM_OF_PRIORITIES * 2; i++) { //2 indicates two tries for each car prioritized, consider assigning dynamically
        can_buy = 0;
        curr_priority_brand = priorities[curr_priority_index]->brand;
        curr_priority_segment = priorities[curr_priority_index]->segment;
        
        
        //sem_wait(mutex); //wait the specific dealer semaphore
        //sem_wait(inv_check);
        printf("Im %s and waiting %d at %d\n", *name , curr_priority_segment, curr_priority_brand);
        sem_wait(rep_locks[curr_priority_brand]); //naming may be confusing, consider changing rep_locks to dealer_locks
        printf("Im %s and waiting %d at %d\n", *name , curr_priority_segment, curr_priority_brand);
        sem_wait(inv_locks[curr_priority_brand]);
        for (int j = 0; j < capacities[curr_priority_brand][curr_priority_segment]; j++) {
            //wait for inventory check
            if (!dealers[curr_priority_brand][curr_priority_segment][j]->is_available || dealers[curr_priority_brand][curr_priority_segment][j]->price > money) {
                //if cant buy
                can_buy = 0;
            } else {
                //if can buy
                can_buy = 1;
                //DEALER ARTTIRIYO
                //resident.money-=car.price
                dealers[curr_priority_brand][curr_priority_segment][j]->is_available = 0; //already reserve this car
                break;
            }
        }

        if (can_buy) {
            printf("I'm %s and I bought my car, see you later suckers!\n", *name);
            sem_post(rep_locks[curr_priority_brand]);
            sem_post(inv_locks[curr_priority_brand]);
            break;
        } else if (!can_buy && num_tried == 0) {
            num_tried++;
            printf("I'm %s and I can't buy the car, this was my first try!\n", *name);
        } else if (!can_buy && num_tried == 1) {
            num_tried = 0;
            curr_priority_index++;
            printf("I'm %s and I'm sure that I am giving up on this car\n", *name);
        }
        printf("Im %s and posting %d at %d\n", *name , curr_priority_segment, curr_priority_brand);
        sem_post(rep_locks[curr_priority_brand]); //naming may be confusing, consider changing rep_locks to dealer_locks
        printf("Im %s and posting %d at %d\n", *name , curr_priority_segment, curr_priority_brand);
        sem_post(inv_locks[curr_priority_brand]);
        //if (!can_buy) {
        //    sleep(rand() % 5);
        //}
        
        //sem_post(inv_check);
        //sem_post(mutex); //post the specific dealer semaphore        
    }
    
    pthread_exit(NULL);
}

void initialize_vars() {
    //Semaphore names
    //for representatives
    for (int i = 0; i < NUM_OF_SEMS; i++) {
        char* new_name = malloc(8);
        sprintf(new_name, "YIGIT%03d", i % 1000);
        rep_lock_names[i] = new_name;
    }
    //for inventories
    for (int i = 0; i < NUM_OF_SEMS; i++) {
        char* new_name = malloc(8);
        sprintf(new_name, "CANBI%03d", i % 1000);
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
            struct car** list = (struct car**) malloc(sizeof(struct car*) * 4); //instead of 4, there are fixed sizes for these
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
}

int main() {
    //check if constants are not absurdly large

    initialize_vars();
    //debug();
    
    initialized = sem_open("TEST000", O_EXCL|O_CREAT, 0644, 0);
    if (initialized == SEM_FAILED) {
        printf("Semaphores already exists or could not be created, removing the current semaphores...\nPlease restart the program.\n");
        sem_close(initialized);
        sem_unlink("TEST000");
        cleanup();
        exit(0);
    }
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        rep_locks[i] = sem_open(rep_lock_names[i], O_EXCL|O_CREAT, 0644, 0); //add error handling for O_EXCL
        inv_locks[i] = sem_open(inv_lock_names[i], O_EXCL|O_CREAT, 0644, 1); //add error handling for O_EXCL

        if (rep_locks[i] == SEM_FAILED || inv_locks[i] == SEM_FAILED) {
            printf("Semaphores already exists or could not be created, removing the current semaphores...\nPlease restart the program.\n");
            cleanup();
            exit(0);
        }
    }

    /*
    //DEPRECATED
    mutex = sem_open(sem_name, O_EXCL|O_CREAT, 0644, 0); //add error handling for O_EXCL
    inv_check = sem_open(sem_name_2, O_EXCL|O_CREAT, 0644, 1); //add error handling for O_EXCL

    if (mutex == SEM_FAILED || inv_check == SEM_FAILED) {
        printf("Semaphore already exists or could not be created, removing the current semaphore...\nPlease restart the program.\n");
        sem_unlink(sem_name_2);
        sem_unlink(sem_name);
        exit(0);
    }
    */
    //create these two dynamically
    char* resident_names[] = {"Polat alemdar", "Heisenberg", "He-man", "Spaydi", "İskeletor", "Mario"};
    int dealer_ids[] = {0, 1, 2, 3};

    pthread_t res_threads[NUM_OF_RESIDENTS];
    pthread_t dealer_threads[NUM_OF_DEALERS];
    //IMPORTANT: Representative threads are created at dealer()
    
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        pthread_create(&dealer_threads[i], NULL, dealer, &dealer_ids[i]);
    }
    for (int i = 0; i < NUM_OF_DEALERS; i++) {
        sem_wait(initialized);
        printf("done %d\n", i);
    }
    for (int i = 0; i < NUM_OF_RESIDENTS; i++) {
        pthread_create(&res_threads[i], NULL, resident, &resident_names[i]);
    }


    for (int i = 0; i < NUM_OF_DEALERS; i++) { // change dealer count to 4 later
        pthread_join(dealer_threads[i], NULL);
    }
    for (int i = 0; i < NUM_OF_RESIDENTS; i++) {
        pthread_join(res_threads[i], NULL);
    }
    cleanup();
    return 0;

    /*
    //DEPRECATED

    char* resident_names[] = {"Polat alemdar", "Heisenberg", "He-man", "Spaydi", "İskeletor", "Mario"};
    pthread_t tid[6];
    int dealer_ids[] = {0, 1, 2, 3};
    pthread_t dealer_tids[4];

    for (int i = 0; i < 1; i++) { //change dealer count to 4 later
        pthread_create(&dealer_tids[i], NULL, dealer, &dealer_ids[i]);
    }
    for (int i = 0; i < 6; i++) {
        pthread_create(&tid[i], NULL, resident, &resident_names[i]);
    }


    for (int i = 0; i < 1; i++) { // change dealer count to 4 later
        pthread_join(dealer_tids[i], NULL);
    }
    for (int i = 0; i < 6; i++) {
        pthread_join(tid[i], NULL);
    }
    sem_close(mutex);
    sem_unlink(sem_name);
    sem_close(inv_check);
    sem_unlink(sem_name_2);
    //cleanup();
    return 0;
    */
}

/*
TODO:
-Dealer price updates
-Dealer price update priority before residents checking prices
-Put various sleep() commands and create a meaningful flow of time for the case
-Make sure constants are fully done
-More names for residents and more meaningful debug texts
*/
