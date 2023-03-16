// includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

// global variables
int REGISTRATION_SIZE = 10; // available registration decks
int RESTROOM_SIZE = 10; // available restrooms
int CAFE_NUMBER = 10; // available cafe cashiers
int GP_NUMBER = 10; // available general practitioners
int PHARMACY_NUMBER = 10; // available pharmacy cashiers
int BLOOD_LAB_NUMBER = 10; // available blood lab assistants

// available operation rooms, surgeons and nurses
int OR_NUMBER = 10;
int SURGEON_NUMBER = 30;
int NURSE_NUMBER = 30;

// maximum amount of surgeons and nurses that can do a surgery.
// a random value is calculated for each operation between 1 and given values.
int SURGEON_LIMIT = 5;
int NURSE_LIMIT = 5;

// amount of patients that will be generated
int PATIENT_NUMBER = 20;

// hospital account
int HOSPITAL_WALLET = 0;

// The WAIT_TIME is the limit for randomly selected time between 1 and the given value 
// that determines how long a patient will wait before each operation to retry to execute.
int WAIT_TIME = 100;
int REGISTRATION_TIME = 100;
int GP_TIME = 200;
int PHARMACY_TIME = 100;
int BLOOD_LAB_TIME = 200;
int SURGERY_TIME = 500;
int CAFE_TIME = 100;
int RESTROOM_TIME = 100;

// The money that will be charged to the patients for given operations. 
// The registration and blood lab costs should be static (not randomly decided) 
// but pharmacy and cafe cost should be randomly generated between 1 and given value below.

// The surgery cost should calculated based on number of doctors and nurses that was required to perform it.
// The formula used for this should be:
// SURGERY_OR_COST + (number of surgeons * SURGERY_SURGEON_COST) + (number of nurses * SURGERY_NURSE_COST)

int REGISTRATION_COST = 100;
int PHARMACY_COST = 200; // Calculated randomly between 1 and given value.
int BLOOD_LAB_COST = 200;
int SURGERY_OR_COST = 200;
int SURGERY_SURGEON_COST = 100;
int SURGERY_NURSE_COST = 50;
int CAFE_COST = 200; // Calculated randomly between 1 and given value.

// The global increase rate of hunger and restroom needs of patients. It will increase randomly between 1 and given rate below.
int HUNGER_INCREASE_RATE = 10;
int RESTROOM_INCREASE_RATE = 10;

// hunger and restroom tresholds
int HUNGER_TRESHOLD = 20;
int RESTROOM_TRESHOLD = 20;

// current global variables
int REGISTRATION_DESKS_IN_USE = 0; 
int RESTROOMS_IN_USE = 0;
int CAFES_IN_USE = 0;
int GPs_IN_USE = 0;
int PHARMACIES_IN_USE = 0;
int BLOOD_LABS_IN_USE = 0;
int ORs_IN_USE = 0;
int SURGEONS_IN_USE = 0;
int NURSES_IN_USE = 0;

// mutexes for global variables
sem_t mutex;

void patient_act(int patient_id, char* work,  int* current_fullness, int capacity, int time_it_takes){
    // utility function
    // reserves rooms/capacity to a patient until their work is over.

    while (*current_fullness >= capacity); // wait until there is an empty spot.

    sem_trywait(&mutex); // lock

    *current_fullness = *current_fullness + 1;
    printf("Patient %d is %s. Capacity: %d/%d\n", patient_id, (char*)work, *current_fullness, capacity);

    usleep(((rand() % time_it_takes) + 1));

    *current_fullness = *current_fullness - 1;
    printf("Patient %d is done %s. Capacity: %d/%d\n", patient_id, (char*)work, *current_fullness, capacity);

    sem_post(&mutex); // unlock
    
}

int waiting(int patient_id, int* hunger, int* restroom_need) {

    // utility function
    // handles waiting part of the algorithm

    int total_cost_this_wait = 0; 

    if (*restroom_need >= RESTROOM_TRESHOLD){
        patient_act(patient_id, "going to restroom", &RESTROOMS_IN_USE, RESTROOM_SIZE, RESTROOM_TIME);
        *restroom_need = 0;
    }
    if (*hunger >= HUNGER_TRESHOLD) {
        patient_act(patient_id, "going to cafe", &CAFES_IN_USE, CAFE_NUMBER, CAFE_TIME);
        total_cost_this_wait += (rand() % CAFE_COST) + 1;
        *hunger = 0;
    }

    printf("Patient %d is waiting.\n", patient_id);

    hunger = hunger + (rand() % HUNGER_INCREASE_RATE) + 1;
    restroom_need = restroom_need + (rand() % RESTROOM_INCREASE_RATE) + 1;

    usleep((rand() % WAIT_TIME) + 1);

    printf("Patient %d is done waiting.\n", patient_id);

    return total_cost_this_wait;

}

void *Patient_Thread(void *patient_id_incoming) {
    int Hunger_Meter = (rand() % 100) + 1; // initial hunger
    int Restroom_Meter = (rand() % 100) + 1; // initial restroom need
    int Needs_Surgery = rand() % 2; // 0 = no need for surgery
    int Needs_Blood_Test = rand() % 2; // 0 = no need for blood test
    int Needs_Medicine = rand() % 2; // 0 = no need for medicine
    int total_cost = 100; // initialize with registration cost

    int patient_id = (int) (intptr_t) patient_id_incoming;

    // wait until a desk opens, then register
    // registering takes time, occupy a desk meanwhile
    patient_act(patient_id, "registering", &REGISTRATION_DESKS_IN_USE, REGISTRATION_SIZE, REGISTRATION_TIME);

    // this patient is registered now, now go to cafe/restroom and wait
    total_cost += waiting(patient_id, &Hunger_Meter, &Restroom_Meter);

    // go to GP for examination
    patient_act(patient_id, "getting examinated in GP", &GPs_IN_USE, GP_NUMBER, GP_TIME);
    total_cost += waiting(patient_id, &Hunger_Meter, &Restroom_Meter);

    // proceed depending on needs_blood_test
    if (Needs_Blood_Test == 1){
        // if the patient needs to give blood
        patient_act(patient_id, "giving blood", &BLOOD_LABS_IN_USE, BLOOD_LAB_NUMBER, BLOOD_LAB_TIME);
        total_cost += waiting(patient_id, &Hunger_Meter, &Restroom_Meter);

        // go to GP for further examination
        patient_act(patient_id, "getting examinated in GP", &GPs_IN_USE, GP_NUMBER, GP_TIME);
        total_cost += waiting(patient_id, &Hunger_Meter, &Restroom_Meter);
        total_cost += 200;
    }

    // proceed depending on needs_surgery
    if (Needs_Surgery == 1){
        
        // get a surgery
        int surgeons_needed = (rand() % SURGEON_LIMIT) + 1;
        int nurses_needed = (rand() % SURGEON_LIMIT) + 1;
        while (ORs_IN_USE >= OR_NUMBER || SURGEONS_IN_USE + surgeons_needed > SURGEON_LIMIT || NURSES_IN_USE + nurses_needed > NURSE_LIMIT); // wait until there is an empty spot.

        sem_trywait(&mutex); // lock

        SURGEONS_IN_USE += surgeons_needed;
        NURSES_IN_USE += nurses_needed;
        ORs_IN_USE++;
        printf("Patient %d is getting a surgery. S:%d/%d - N:%d/%d\n", patient_id, SURGEONS_IN_USE, SURGEON_NUMBER, NURSES_IN_USE, NURSE_NUMBER);

        usleep(((rand() % SURGERY_TIME) + 1));

        SURGEONS_IN_USE -= surgeons_needed;
        NURSES_IN_USE -= nurses_needed;
        ORs_IN_USE--;
        printf("Patient %d is done getting a surgery. ", patient_id);

        sem_post(&mutex); // unlock

        total_cost += waiting(patient_id, &Hunger_Meter, &Restroom_Meter);

        // go to GP for further examination
        patient_act(patient_id, "getting examinated in GP", &GPs_IN_USE, GP_NUMBER, GP_TIME);
        total_cost += waiting(patient_id, &Hunger_Meter, &Restroom_Meter);
        total_cost += 200 + 100 * surgeons_needed + 50 * nurses_needed;
        
    }

    // proceed depending on needs_medicine
    if (Needs_Medicine == 1) {
        // buy medicine
        patient_act(patient_id, "buying medicine", &PHARMACIES_IN_USE, PHARMACY_TIME, GP_TIME);
        total_cost += waiting(patient_id, &Hunger_Meter, &Restroom_Meter);
        total_cost += (rand() % PHARMACY_COST) + 1;
    }

    
    HOSPITAL_WALLET += total_cost;
    printf("Patient %d has left the system. They paid $%d.\n", patient_id, total_cost);


}

int main(void) {
   
    // initalize random
    srand(time(NULL));
    
    // thread array
    pthread_t *thread_ids = malloc(PATIENT_NUMBER*sizeof(pthread_t));

    // initialize sem with mutex
    sem_init(&mutex, 0, 1);

    for (int i = 0; i<=PATIENT_NUMBER; i++){
        pthread_create(&thread_ids[i], NULL, Patient_Thread, (void *) (intptr_t) i);
    }
    
    for (int i = 0; i<=PATIENT_NUMBER; i++){
        pthread_join(thread_ids[i], NULL);
    }


    // free up memory
    sem_destroy(&mutex);
    free(thread_ids);
    

    printf("Wallet result: $%d", HOSPITAL_WALLET);
    printf("\n");

}