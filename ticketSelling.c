#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define NUM_QUEUE 10
#define ROW_SIZE 10

typedef struct customer {
	int arrival_time;
	int custId;
} customer;


typedef struct customerQ {
	customer *cust;
	int front, rear;
} customerQ;


typedef struct row {
	int next_avail;
	pthread_mutex_t mutex;
} row;

typedef struct pthread_args{
	char *seller_type;
	int row_id;
} pthread_args;

customerQ *custQ;
row tickets[10];

// this is just for now i.e simulation sleeping for
// that many sec. that's it
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
void * sell(pthread_args *pargs)
{

	while (serve_t < 60)
	{
		int served_customer=0;
		//Check if either two or all threads of different type are accessing same row.
		if(htype==mtype || mtype == ltype || htype == ltype || (htype==mtype && mtype == ltype && htype == ltype)){
			printf("they are processing same row now\n");
		}

		if(strcmp(pargs->seller_type, "H") == 0){
            
			float serve_time  = H_serve_time[rand() % 2];

			if(tickets[htype].next_avail>9){
				htype++;
			}

			if(htype_customer == N){
				printf("All Customers served of type H !\n");
				return NULL;
			}
			served_customer = custQ[pargs->row_id].cust[htype_customer++].custId;
			printf("Customer %d of Htype arrived..\n", served_customer);

			printf("Customer %d served of Htype, Seat %d %d sold...\n", served_customer, htype, tickets[htype].next_avail);
			tickets[htype].next_avail++;

			printf("Sleeping thread %s for time %f\n", pargs->seller_type, serve_time);
			sleep(serve_time);
			
		} else if(strcmp(pargs->seller_type, "M") == 0) {
			float serve_time  = M_serve_time[rand() % 3];

			int lock_for_row = mtype;
			// take lock for that perticular row
	 		pthread_mutex_lock(&tickets[lock_for_row].mutex);
			if(tickets[mtype].next_avail>9){
				mtype = m_serve_order[++next_row_m];
			}
			if(mtype_customer == N){
				printf("All Customers served of type M !\n");
				return NULL;
			}
			served_customer = custQ[pargs->row_id].cust[mtype_customer++].custId;
			printf("Customer %d of Mtype arrived..\n", served_customer);

			printf("Customer %d served of Mtype, Seat %d %d sold...\n", served_customer, mtype, tickets[mtype].next_avail);
			tickets[mtype].next_avail++;

			printf("Sleeping thread %s for time %f\n", pargs->seller_type, serve_time);
			sleep(serve_time);

			pthread_mutex_unlock(&tickets[lock_for_row].mutex);

		} else {
			float serve_time  = L_serve_time[rand() % 4];

			int lock_for_row = ltype;
			// take lock for that perticular row
			pthread_mutex_lock(&tickets[lock_for_row].mutex);

			if(tickets[ltype].next_avail>9){
				ltype--;
			}
			if(ltype_customer == N){
				printf("All Customers served of type L !\n");
				return NULL;
			}
			served_customer = custQ[pargs->row_id].cust[ltype_customer++].custId;
			printf("Customer %d of Ltype arrived..\n", served_customer);

			printf("Customer %d served of Ltype, Seat %d %d sold...\n", served_customer, ltype, tickets[ltype].next_avail);
			tickets[ltype].next_avail++;

			printf("Sleeping thread %s for time %f\n", pargs->seller_type, serve_time);
			sleep(serve_time);

			pthread_mutex_unlock(&tickets[lock_for_row].mutex);
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

    // initialize all mutex value
    for(i = 0; i < ROW_SIZE; ++i){
		pthread_mutex_init(&tickets[i].mutex, NULL);
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

	pthread_args pargs[10];

	printf("Please enter the number of customer : ");
	scanf("%d", &N);
	setupQueue(N);

	printCustomerQ(N);

	int i;
	pthread_t tids[10];
	// Create necessary data structures for the simulator.
	// Create buyers list for each seller ticket queue based on the
	// N value within an hour and have them in the seller queue.
	// Create 10 threads representing the 10 sellers.
	
	pargs[0].seller_type = (char *) malloc(strlen("H") + 1);
	memcpy(pargs[0].seller_type, "H", strlen("H"));
	pargs[0].row_id = 0;
	pthread_create(&tids[0], NULL, (void *)sell, &pargs[0]);
	
	for (i = 1; i < 4; i++) {
		pargs[i].seller_type = (char *) malloc(strlen("M") + 1);
		memcpy(pargs[i].seller_type, "H", strlen("H"));
		pargs[i].row_id = i;
		pthread_create(&tids[i], NULL, (void *)sell, &pargs[i]);
	}
	
	
	for (i = 4; i < 10; i++) {
		pargs[i].seller_type = (char *) malloc(strlen("L") + 1);
		memcpy(pargs[i].seller_type, "L", strlen("L"));
		pargs[i].row_id = i;
		pthread_create(&tids[i], NULL, (void *)sell, &pargs[i]);
	}
	
	// wakeup all seller threads
	wakeup_all_seller_threads();
	// wait for all seller threads to exit
	for (i = 0 ; i < 10 ; i++)
		pthread_join(tids[i], NULL);
	// Printout simulation results
	exit(0);
}
