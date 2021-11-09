/* file: echosrv.c

   Bare-bones single-threaded TCP server. Listens for connections
   on "ephemeral" socket, assigned dynamically by the OS.

   This started out with an example in W. Richard Stevens' book
   "Advanced Programming in the Unix Environment".  I have
   modified it quite a bit, including changes to make use of my
   own re-entrant version of functions in echolib.

   NOTE: See comments starting with "NOTE:" for indications of
   places where code needs to be added to make this multithreaded.
   Remove those comments from your solution before turning it in,
   and replace them by comments explaining the parts you have
   changed.

   Ted Baker
   February 2015

 */

#include "config.h" 
#include "pthread.h"
#include "echolib.h"
#include "checks.h"
#include "Queue.h"
#include "Queue2.h"

#define TIMEOUT 10

Queue2 buffer;
Queue sock_buffer;
pthread_mutex_t lock;
pthread_mutex_t lock2;
pthread_mutex_t lock3;
pthread_mutex_t lock4;
pthread_mutex_t lock5;
pthread_mutex_t lock6;
pthread_mutex_t lock7;
pthread_mutex_t lock8;
pthread_mutex_t lock9;
pthread_cond_t signal2;
pthread_cond_t signal3;
int op = 0;
int flag = 0, cnt2 = 0; //cnt for throughput

// A thread pool that keeps whole threads while the program is running 
typedef struct __threadpool {
	int num_threads;
	pthread_t* pool;
	Queue* container; 
} ThreadPool;

typedef struct _package {
	int thread_num;
	ThreadPool* thread_pool;
} Package;

void serve_connection (int sockfd, Package* package);
void* job(void *arg);
void* acceptorThread(void *args);

void alarmHandler()
{
	printf("알람이 종료되었습니다!(%d 초)\n", TIMEOUT);
	printf("총 request 처리 개수 : %d\n", cnt2);
	return;
}

// Thread pool constructor
// Only acceptor threads contain activated container(Queue) which will be used when they receive data 
ThreadPool* thread_pool_constructor(int num_threads)
{
	ThreadPool* thread_pool = (ThreadPool *)malloc(sizeof(ThreadPool));
	thread_pool->num_threads = num_threads;
	thread_pool->pool = (pthread_t*)malloc(sizeof(pthread_t[num_threads]));
	thread_pool->container = (Queue*)malloc(sizeof(Queue[num_threads / 3]));

	for (int i = 0; i < num_threads; i++)
	{
		if (i % 3 == 0) {
			Package* package = (Package*)malloc(sizeof(Package));
			package->thread_num = i / 3;
			package->thread_pool = thread_pool;
			queueInit(&thread_pool->container[i / 3]);
			pthread_create(&thread_pool->pool[i], NULL, acceptorThread, (void*)package);
		} else {
			pthread_create(&thread_pool->pool[i], NULL, job, (void*)thread_pool);
		}
	}
	return thread_pool;
}

// Thread pool destructor
void thread_pool_destructor(ThreadPool * thread_pool)
{
	flag = 1;
	pthread_cond_signal(&signal2);
	pthread_cond_signal(&signal3);
	
	for (int i = 0; i < thread_pool->num_threads; i++)
	{
		int rc = pthread_join(thread_pool->pool[i], NULL);
		if (rc) {
			printf("Error; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
	}
	free(thread_pool->pool);
}

// The main work that each thread in the thread pool will execute 
int work(Data2* data)
{ 
	int final_data = data->value;
	
	for(int i=2;i<=final_data/2;i++){
		if(final_data%i==0){
			final_data = 0;
			pthread_mutex_lock(&lock8);
			cnt2++;
			pthread_mutex_unlock(&lock8);
			return final_data;
		}
		else{}
	}
	final_data =1;
	pthread_mutex_lock(&lock8);
	cnt2++;
	pthread_mutex_unlock(&lock8);
	return final_data;
}

// The main work which will be used in sequential version
int work2(int data)
{
	int final_data = data;
	cnt2++;
	return final_data;
}

// Each thread in the thread pool(not acceptor thread) will be allocated this job at the beginning
void* job(void *arg)
{
	ThreadPool * thread_pool = (ThreadPool *)arg;
	Data2* data;
	printf("queen\n");
	while (!flag)
	{
		pthread_mutex_lock(&lock);
		if (IsQueue2Empty(&buffer) && !flag) {
			pthread_cond_wait(&signal2, &lock2);
			data = dequeue2(&buffer);
		} else {
			pthread_mutex_lock(&lock4);
			data = dequeue2(&buffer);
			pthread_mutex_unlock(&lock4);
		}
		pthread_mutex_unlock(&lock);
		
		if (!flag) {
			int id = data->thread_num;
			int ans = work(data);
			pthread_mutex_lock(&lock9);
			enqueue(&thread_pool->container[id], ans);
			pthread_mutex_unlock(&lock9);
		}
	}
	return NULL;
}

// Each acceptor thread executes this function at the beginning
void* acceptorThread(void* args)
{
  Package* my_package = (Package*)args;

  Data sockfd;	
  while(!flag)
  {
	pthread_mutex_lock(&lock5);
        if (IsQueueEmpty(&sock_buffer) && !flag ) {	
  		pthread_cond_wait(&signal3, &lock6);
		sockfd = dequeue(&sock_buffer);
	} else {
		pthread_mutex_lock(&lock7);
		sockfd = dequeue(&sock_buffer);
		pthread_mutex_unlock(&lock7);
	}
	pthread_mutex_unlock(&lock5);

	if (!flag) {
		serve_connection(sockfd, my_package);
	}
  }
  return NULL;
}	

void server_handoff (int sockfd, int op, ThreadPool* thread_pool) {
  if (op > 0) {
  	if (IsQueueEmpty(&sock_buffer))
  	{  
  		enqueue(&sock_buffer, sockfd);
  		pthread_cond_signal(&signal3);
  	} else { 
  		pthread_mutex_lock(&lock7);
		enqueue(&sock_buffer, sockfd);
		pthread_mutex_lock(&lock7);
  	}
  } else if (op == 0) {
	Package* parcel = (Package*)malloc(sizeof(Package));
        parcel->thread_num = -1;
	parcel->thread_pool = thread_pool;	
  	serve_connection(sockfd, parcel);
  }
}

/* the main per-connection service loop of the server; assumes
   sockfd is a connected socket */
void serve_connection (int sockfd, Package* package) {
  ssize_t  n, result;
  char line[MAXLINE];
  char output[MAXLINE];
  char temp2[MAXLINE];
  connection_t conn;
  connection_init (&conn);
  conn.sockfd = sockfd;
  int op = package->thread_num, cnt = 0;

  while (! shutting_down) {
    cnt = 0;
    if ((n = readline(&conn, line, MAXLINE)) == 0) goto quit;
    
    if (op > -1) { // pthread mode
    	char *temp = strtok(line, " ");
    	while (temp != NULL) {
		if (isdigit(temp[0]) != 0) {
			Tdata* tdata = (Tdata*)malloc(sizeof(Tdata));
			tdata->thread_num = op;
			tdata->value= atoi(temp);
		
			pthread_mutex_lock(&lock3);
			if (IsQueue2Empty(&buffer))
			{	
				enqueue2(&buffer, tdata);
				pthread_cond_signal(&signal2);
			} else {
				pthread_mutex_lock(&lock4);
				enqueue2(&buffer, tdata);
				pthread_mutex_unlock(&lock4);
			}
			pthread_mutex_unlock(&lock3);
			cnt++;
		}
		temp = strtok(NULL, " ");
    	}	    
    }
    /* connection closed by other end */
    if (shutting_down) goto quit;
    if (n < 0) {
      perror ("readline failed");
      goto quit;
    }
    if (op > -1) { // pthread mode
	output[0] = '\0';
  	temp2[0] = '\0';
	while (cnt--) {    
		printf("kek\n");
		while (IsQueueEmpty(&package->thread_pool->container[op]));    
		printf("gg\n");
		int ans = dequeue(&package->thread_pool->container[op]);
		sprintf(temp2,"%d", ans);
		int tro = strlen(temp2);

		temp2[tro] = ' ';
		temp2[tro+1] = '\0';

		strcat(output, temp2);
		printf("%d\n", cnt);
	}
	int len2 = strlen(output);
	output[len2] = '\n';
	output[len2 + 1] = '\0';
	
	printf("ho\n");
	result = writen (&conn, output, strlen(output));

    	if (result != strlen(output)) {
      		perror ("writen failed");
      		goto quit;
    	}
    } else { // sequential mode
	output[0] = '\0';
  	temp2[0] = '\0';
	char *temp = strtok(line, " ");
    	while (temp != NULL) {
		if (isdigit(temp[0]) != 0) {
			int ans = work2(atoi(temp));
			sprintf(temp2, "%d", ans);
			int tro = strlen(temp2);

			temp2[tro] = ' ';
			temp2[tro + 1] = '\0';
			strcat(output, temp2);
		}
		temp = strtok(NULL, " ");
    	}
	int len2 = strlen(output);
	output[len2] = '\n';
	output[len2 + 1] = '\0';

    	result = writen (&conn, output, strlen(output));
	if (result != strlen(output)) {
      		perror ("writen failed");
     		goto quit;
    	}
    }
    if (shutting_down) goto quit;
  }
quit:
  CHECK (close (conn.sockfd));
}

/* set up socket to use in listening for connections */
void open_listening_socket (int *listenfd) {
  struct sockaddr_in servaddr;
  const int server_port = 0; /* use ephemeral port number */
  socklen_t namelen;
  memset (&servaddr, 0, sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  /* htons translates host byte order to network byte order; ntohs
     translates network byte order to host byte order */
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (server_port);
  /* create the socket */
  CHECK (*listenfd = socket(AF_INET, SOCK_STREAM, 0))
  /* bind it to the ephemeral port number */
  CHECK (bind (*listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)));
  /* extract the ephemeral port number, and put it out */
  namelen = sizeof (servaddr);
  CHECK (getsockname (*listenfd, (struct sockaddr *) &servaddr, &namelen));
  fprintf (stderr, "server using port %d\n", ntohs(servaddr.sin_port));
}

/* handler for SIGINT, the signal conventionally generated by the
   control-C key at a Unix console, to allow us to shut down
   gently rather than having the entire process killed abruptly. */ 
void siginthandler (int sig, siginfo_t *info, void *ignored) {
  shutting_down = 1;
}

void install_siginthandler () {
  struct sigaction act;
  /* get current action for SIGINT */
  CHECK (sigaction (SIGINT, NULL, &act));
  /* add our handler */
  act.sa_sigaction = siginthandler;
  /* update action for SIGINT */
  CHECK (sigaction (SIGINT, &act, NULL));
}

int main (int argc, char **argv) {
  int connfd, listenfd;
  socklen_t clilen;
  struct sockaddr_in cliaddr;

  int c;
  char* opstring;
  signal(SIGALRM, alarmHandler);

  if (argc == 1) {
  	printf("-n 옵션을 사용하여 입력하세요!\n");
	return 0;
  }

  while ((c = getopt(argc, argv, "n:")) != -1) {
  	switch (c)
	{
		case 'n':
			opstring = optarg;
			break;
		case '?':
			printf("Unknown flag : %c\n", optopt);
			break;
	}
  }

  op = atoi(opstring);
  ThreadPool* thread_pool;

  if (op > 0) {
        pthread_mutex_init(&lock, NULL);
  	pthread_mutex_init(&lock2, NULL);
  	pthread_mutex_init(&lock3, NULL);
  	pthread_mutex_init(&lock4, NULL);
  	pthread_mutex_init(&lock5, NULL);
  	pthread_mutex_init(&lock6, NULL);
  	pthread_mutex_init(&lock7, NULL);
	pthread_mutex_init(&lock8, NULL);
	pthread_mutex_init(&lock9, NULL);
  	pthread_cond_init(&signal2, NULL);
	pthread_cond_init(&signal3, NULL);
  	queue2Init(&buffer);
  	queueInit(&sock_buffer);
  	thread_pool = thread_pool_constructor(op);
  }

  install_siginthandler();
  open_listening_socket (&listenfd);
  CHECK (listen (listenfd, 4));
  alarm(TIMEOUT);
  /* allow up to 4 queued connection requests before refusing */
  while (! shutting_down) {
    errno = 0;
    clilen = sizeof (cliaddr); /* length of address can vary, by protocol */
    if ((connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
      if (errno != EINTR) ERR_QUIT ("accept"); 
      /* otherwise try again, unless we are shutting down */
    } else { 
     server_handoff (connfd, op, thread_pool); /* process the connection */
    }
  }
 
  if (op > 0) {  
  	thread_pool_destructor(thread_pool);
  	pthread_mutex_destroy(&lock);
  	pthread_mutex_destroy(&lock2);
  	pthread_mutex_destroy(&lock3);
  	pthread_mutex_destroy(&lock4);
  	pthread_mutex_destroy(&lock5);
  	pthread_mutex_destroy(&lock6);
  	pthread_mutex_destroy(&lock7);
	pthread_mutex_destroy(&lock8);
	pthread_mutex_destroy(&lock9);
  	pthread_cond_destroy(&signal2);
  	pthread_cond_destroy(&signal3);
  }

  CHECK (close (listenfd));
  return 0;
}
