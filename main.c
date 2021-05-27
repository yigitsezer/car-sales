#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>

char* rep_names[] = {"Yigit0", "Can0", "Yigit1", "Can1", "Yigit2", "Can2", "Yigit3", "Can3"};

int capacities[4][3] = {
    {2, 2, 2},
    {2, 2, 1},
    {2, 1, 1},
    {1, 1, 1}};

sem_t* mutex;
sem_t* inv_check;
//sem_t* semaphores[4];
char* sem_name = "mysemaphore";
char* sem_name_2 = "mysemaphore2";

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

struct car*** dealers[4]; //all dealers
void* representative(void* arg);


void* dealer(void* arg) {
    int *id = (int *)arg;
    printf("I'm dealer %d\n", *id);
    pthread_t rep_tids[2];
    for (int i = 0; i < 2; i++) {
        struct rep_info* new_rep = malloc(sizeof(struct rep_info));
        new_rep->dealer_id = *id;
        new_rep->name = rep_names[2*(*id) + i];
        //printf("new representative %s at %d\n", new_rep->name, new_rep->dealer_id);
        pthread_create(&rep_tids[i], NULL, representative, (void *)new_rep);
    }


    //update prices twice a month
    //sem_wait()



    for (int i = 0; i < 2; i++) {
        pthread_join(rep_tids[i], NULL);
    }
    return 0;
}

void* representative(void* arg) {
    //handle customers (sell or not)
    //struct rep_info info = (struct rep_info)arg;
    //char **name = (char **)arg;
    //int *dealer_id = (int *)arg
    printf("Im representative %s at %d\n", ((struct rep_info*)arg)->name, ((struct rep_info*)arg)->dealer_id);
    sem_post(mutex); //initialized

    return 0;
}

void* resident(void* arg) {
    char **name = (char **)arg;
    struct priority** priorities = (struct priority**) malloc(sizeof(struct priority*) * 4);
    int money = 10000; //random between 200k-500k
    int done = 0;

    for (int i = 0; i < 4; i++) {
        struct priority* new_priority = malloc(sizeof(struct priority));
        new_priority->brand = i;
        new_priority->segment = i%3; //randomize both of these
        priorities[i] = new_priority;
    }
    
    //debug for resident priorities
    for (int i = 0; i < 4; i++) {
        //printf("IM %s | priority %d | brand: %d | segment: %d\n", *name ,i+1, priorities[i]->brand, priorities[i]->segment);
    }
    
    //Go to dealer for according to current prioritized car's brand
    //wait for a representative
    //try to buy
    //if cannot buy for the second time, change priority and try all this again

    int num_tried = 0;
    int curr_priority_index = 0;
    int curr_priority_brand = 0;
    int curr_priority_segment = 0;
    int can_buy = 0;
    
    for (int i = 0; i < 2; i++) { //change this to 8
        can_buy = 0;
        curr_priority_brand = priorities[curr_priority_index]->brand;
        curr_priority_segment = priorities[curr_priority_index]->segment;
        
        
        sem_wait(mutex); //wait the specific dealer semaphore
        sem_wait(inv_check);
        for (int j = 0; j < capacities[curr_priority_brand][curr_priority_segment]; j++) {
            //wait for inventory check
            if (!dealers[curr_priority_brand][curr_priority_segment][j]->is_available) {
                //if cant buy
                can_buy = 0;
            } else if (dealers[curr_priority_brand][curr_priority_segment][j]->price < money) {
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
            sem_post(inv_check);
            sem_post(mutex); //post the specific dealer semaphore
            break;
        } else if (!can_buy && num_tried == 0) {
            num_tried++;
            printf("I'm %s and I can't buy the car, this was my first try!\n", *name);
        } else if (!can_buy && num_tried == 1) {
            num_tried = 0;
            curr_priority_index++;
            printf("I'm %s and I'm sure that I am giving up on this car\n", *name);
        }

        sem_post(inv_check);
        sem_post(mutex); //post the specific dealer semaphore        
    }
    
    

    return NULL;
}

int main() {
    //pthread_t tid; /* the thread identifier */
    //pthread_attr_t attr;

    for (int i = 0; i < 4; i++) {
        struct car*** segments = (struct car***) malloc(sizeof(struct car**) * 3); //one dealer, list of segments
        for (int j = 0; j < 3; j++) {
            struct car** list = (struct car**) malloc(sizeof(struct car*) * 4); //instead of 4, there are fixed sizes for these
            for (int k = 0; k < capacities[i][j]; k++) {
                struct car* new_car = malloc(sizeof(struct car));
                new_car->price = (i+1)*2 + (j+1)*3; //randomize car prices
                new_car->is_available = 1;
                list[k] = new_car;
            }
            segments[j] = list;
        }  
        dealers[i] = segments;
    }

    
    /*
    //debug for dealer inventories
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
    */
    


    //mutex = sem_open(&mutex, 1, 1);
    mutex = sem_open(sem_name, O_EXCL|O_CREAT, 0644, 0); //add error handling for O_EXCL
    inv_check = sem_open(sem_name_2, O_EXCL|O_CREAT, 0644, 1); //add error handling for O_EXCL

    if (mutex == SEM_FAILED || inv_check == SEM_FAILED) {
        printf("Semaphore already exists or could not be created, removing the current semaphore...\nPlease restart the program.\n");
        sem_unlink(sem_name_2);
        sem_unlink(sem_name);
        exit(0);
    }
    

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

    return 0;
}