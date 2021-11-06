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
/* not needed now, but will be needed in multi-threaded version */
#include "pthread.h"
#include "echolib.h"
#include "checks.h"
#include "Queue.h"

Queue buffer;
Queue sock_buffer;
pthread_mutex_t lock;
pthread_mutex_t lock2;
pthread_mutex_t lock3;
pthread_mutex_t lock4;
pthread_mutex_t lock5;
pthread_mutex_t lock6;
pthread_mutex_t lock7;
pthread_cond_t signal2;
pthread_cond_t signal3;
int op = 0;
int flag = 0;
typedef struct __threadpool {
	int num_threads;
	pthread_t* pool;
} ThreadPool;

void serve_connection (int sockfd);
void* job(void *arg);
void* acceptorThread(void *args);

ThreadPool* thread_pool_constructor(int num_threads)
{
	ThreadPool* thread_pool = (ThreadPool *)malloc(sizeof(ThreadPool));
	thread_pool->num_threads = num_threads;
	thread_pool->pool = (pthread_t*)malloc(sizeof(pthread_t[num_threads]));

	for (int i = 0; i < num_threads; i++)
	{
		if (i % 3 == 0) {
			pthread_create(&thread_pool->pool[i], NULL, acceptorThread, NULL);
		} else {
			pthread_create(&thread_pool->pool[i], NULL, job, NULL);
		}
	}
	return thread_pool;
}

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

int work(Data data)
{
	return data;
}

void* job(void *arg)
{
	Data data;
	while (!flag)
	{
		pthread_mutex_lock(&lock);
		if (IsQueueEmpty(&buffer)) {
			pthread_cond_wait(&signal2, &lock2);
			data = dequeue(&buffer);
		} else {
			pthread_mutex_lock(&lock4);
			data = dequeue(&buffer);
			pthread_mutex_unlock(&lock4);
		}
		pthread_mutex_unlock(&lock);
		
		if (!flag) {
			int ans = work(data);
			printf("answer : %d\n", ans);
		}
	}
	return NULL;
}

void* acceptorThread(void* args)
{
  Data sockfd;	
  while(!flag)
  {
	pthread_mutex_lock(&lock5);
        if (IsQueueEmpty(&sock_buffer)) {	
  		pthread_cond_wait(&signal3, &lock6);
		sockfd = dequeue(&sock_buffer);
	} else {
		pthread_mutex_lock(&lock7);
		sockfd = dequeue(&sock_buffer);
		pthread_mutex_unlock(&lock7);
	}
	pthread_mutex_unlock(&lock5);

	serve_connection(sockfd);
  }
  return NULL;
}	

void server_handoff (int sockfd) {
  if (IsQueueEmpty(&sock_buffer))
  {  
  	enqueue(&sock_buffer, sockfd);
  	pthread_cond_signal(&signal3);
  } else { 
  	pthread_mutex_lock(&lock7);
	enqueue(&sock_buffer, sockfd);
	pthread_mutex_lock(&lock7);
  }
}

/* the main per-connection service loop of the server; assumes
   sockfd is a connected socket */
void serve_connection (int sockfd) {
  ssize_t  n, result;
  char line[MAXLINE];
  connection_t conn;
  connection_init (&conn);
  conn.sockfd = sockfd;
  while (! shutting_down) {
    if ((n = readline (&conn, line, MAXLINE)) == 0) goto quit;
    // my code
    char *temp = strtok(line, " ");
    while (temp != NULL) {
	pthread_mutex_lock(&lock3);
	if (IsQueueEmpty(&buffer))
	{	
		enqueue(&buffer, atoi(temp));
		pthread_cond_signal(&signal2);
	} else {
		pthread_mutex_lock(&lock4);
		enqueue(&buffer, atoi(temp));
		pthread_mutex_unlock(&lock4);
	}
	pthread_mutex_unlock(&lock3);
	temp = strtok(NULL, " ");
    }    
    /* connection closed by other end */
    if (shutting_down) goto quit;
    if (n < 0) {
      perror ("readline failed");
      goto quit;
    }
    result = writen (&conn, line, n);
    if (shutting_down) goto quit;
    if (result != n) {
      perror ("writen failed");
      goto quit;
    }
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

  /* NOTE: To make this multi-threaded, You may need insert
     additional initialization code here, but you will not need to
     modify anything below here, though you are permitted to
     change anything in this file if you feel it is necessary for
     your design */
  int c;
  char* opstring;

  pthread_mutex_init(&lock, NULL);
  pthread_mutex_init(&lock2, NULL);
  pthread_mutex_init(&lock3, NULL);
  pthread_mutex_init(&lock4, NULL);
  pthread_mutex_init(&lock5, NULL);
  pthread_mutex_init(&lock6, NULL);
  pthread_mutex_init(&lock7, NULL);
  pthread_cond_init(&signal2, NULL);
  pthread_cond_init(&signal3, NULL);  
  
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

  queueInit(&buffer);
  queueInit(&sock_buffer);
  ThreadPool* thread_pool = thread_pool_constructor(op);

  install_siginthandler();
  open_listening_socket (&listenfd);
  CHECK (listen (listenfd, 4));
  /* allow up to 4 queued connection requests before refusing */
  while (! shutting_down) {
    errno = 0;
    clilen = sizeof (cliaddr); /* length of address can vary, by protocol */
    if ((connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
      if (errno != EINTR) ERR_QUIT ("accept"); 
      /* otherwise try again, unless we are shutting down */
    } else { 
     server_handoff (connfd); /* process the connection */
    }
  }
 
  thread_pool_destructor(thread_pool);
  pthread_mutex_destroy(&lock);
  pthread_mutex_destroy(&lock2);
  pthread_mutex_destroy(&lock3);
  pthread_mutex_destroy(&lock4);
  pthread_mutex_destroy(&lock5);
  pthread_mutex_destroy(&lock6);
  pthread_mutex_destroy(&lock7);
  pthread_cond_destroy(&signal2);
  pthread_cond_destroy(&signal3);

  CHECK (close (listenfd));
  return 0;
}
