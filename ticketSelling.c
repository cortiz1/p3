#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define NUM_QUEUE 10

typedef struct customer {
	int arrival_time;
	int custId;
} customer;


typedef struct customerQ {
	customer *cust;
	int front, rear;
} customerQ;


typedef struct ticketSell {
	int next_avail;
} ticketSell;

customerQ *custQ;
ticketSell tickets[10];

float H_serve_time[] = {0.1, .2};
float M_serve_time[] = {.2, .3, .4};
float L_serve_time[] = {.4, .5, .6, .7};

int m_serve_order[] = {4,5,3,6,2,7,1,8,0,9};

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int serve_t = 0;
// Number of customer to service 
int N;
static int next_row_m = 0;
static int htype = 0,  htype_served, htype_customer;
static int mtype = 4,  mtype_served, mtype_customer;
static int ltype = 9,  ltype_served, ltype_customer;

// seller thread to serve one time slice (1 minute)
void * sell(char *seller_type)
{

	while (serve_t < 100)
	{
		int served_customer=0;
		//Check if either two or all threads of different type are accessing same row.
		if(htype==mtype || mtype == ltype || htype == ltype || (htype==mtype && mtype == ltype && htype == ltype)){
			printf("they are processing same row now\n");
		}

		if(strcmp(seller_type, "H") == 0){
            
			float serve_time  = H_serve_time[rand() % 2];
			printf("Sleeping thread %s for time %f\n", seller_type, serve_time);
			if(tickets[htype].next_avail>9){
				htype++;
			}

			if(htype_customer == N){
				printf("All Customers served of type H !\n");
				return NULL;
			}
			served_customer = custQ[htype].cust[htype_customer++].custId;
			printf("Customer %d of Htype arrived..\n", served_customer);

			printf("Customer %d served of Htype, Seat %d %d sold...\n", served_customer, htype, tickets[htype].next_avail);
			tickets[htype].next_avail++;
			sleep(serve_time);
			
		} else if(strcmp(seller_type, "M") == 0) {
			float serve_time  = M_serve_time[rand() % 3];
			printf("Sleeping thread %s for time %f\n", seller_type, serve_time);
			if(tickets[mtype].next_avail>9){
				mtype = m_serve_order[++next_row_m];
			}
			if(mtype_customer == N){
				printf("All Customers served of type M !\n");
				return NULL;
			}
			served_customer = custQ[mtype].cust[mtype_customer++].custId;
			printf("Customer %d of Mtype arrived..\n", served_customer);
			
			printf("Customer %d served of Mtype, Seat %d %d sold...\n", served_customer, mtype, tickets[mtype].next_avail);
			tickets[mtype].next_avail++;
			sleep(serve_time);

		} else {
			float serve_time  = L_serve_time[rand() % 4];
			printf("Sleeping thread %s for time %f\n", seller_type, serve_time);
			if(tickets[ltype].next_avail>9){
				ltype--;
			}
			if(ltype_customer == N){
				printf("All Customers served of type L !\n");
				return NULL;
			}
			served_customer = custQ[ltype].cust[ltype_customer++].custId;
			printf("Customer %d of Ltype arrived..\n", served_customer);
			
			printf("Customer %d served of Ltype, Seat %d %d sold...\n", served_customer, ltype, tickets[ltype].next_avail);
			tickets[ltype].next_avail++;
			sleep(serve_time);
		}

		serve_t++;
	}
	return NULL;
	// thread exits
}

void wakeup_all_seller_threads()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
}


int compare_arrival_times(const void * a, const void * b)
{
    customer *c1 = (customer *)a;
    customer *c2 = (customer *)b;
    int r = c1->arrival_time - c2->arrival_time;
    if (r>0) return 1;
    if (r<0) return -1;
    return r;
}

void setupQueue(int N)
{
	int i, j;
	custQ = (customerQ *) malloc(sizeof (customerQ) * NUM_QUEUE);

	srand(time(NULL));
	for(i = 0; i < NUM_QUEUE; ++i)
    {

    	custQ[i].cust = (customer *) malloc(sizeof(customer) * N);

    	for (j = 0; j < N; ++j)
    	{
    		int arrival_time = rand() % 60;
    		custQ[i].cust[j].custId = j;
    		custQ[i].cust[j].arrival_time = arrival_time;  
    	}

    }

    // sort customer based on arrival times
    for(i = 0; i < NUM_QUEUE; ++i){
    	qsort((void *)custQ[i].cust, N, sizeof(customer), compare_arrival_times);	
    }
    
}

void printCustomerQ(int N)
{
	int i, j;

	for(i = 0; i < NUM_QUEUE; ++i)
    {

    	for (j = 0; j < N; ++j)
    	{
    		
    		printf(" %d:%d\t|", custQ[i].cust[j].custId, custQ[i].cust[j].arrival_time);
    	}
    	printf("\n----------------------------------------------");
    	printf("\n");
    }	
}

int main(int argc, char *argv[])
{


	printf("Please enter the number of customer : ");
	scanf("%d", &N);
	setupQueue(N);

	//printCustomerQ(N);

	int i;
	pthread_t tids[10];
	char *seller_type = (char *) malloc(3);
	// Create necessary data structures for the simulator.
	// Create buyers list for each seller ticket queue based on the
	// N value within an hour and have them in the seller queue.
	// Create 10 threads representing the 10 sellers.
	
	seller_type = "H";
	pthread_create(&tids[0], NULL, (void *)sell, seller_type);
	
	seller_type = "M";
	for (i = 1; i < 4; i++)
		pthread_create(&tids[i], NULL, (void *)sell, seller_type);
	
	seller_type = "L";
	for (i = 4; i < 10; i++)
		pthread_create(&tids[i], NULL, (void *)sell, seller_type);
	
	// wakeup all seller threads
	wakeup_all_seller_threads();
	// wait for all seller threads to exit
	for (i = 0 ; i < 10 ; i++)
		pthread_join(tids[i], NULL);
	// Printout simulation results
	exit(0);
}
