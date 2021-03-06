/* file: echocli.c

   Bare-bones TCP client with commmand-line argument to specify
   port number to use to connect to server.  Server hostname is
   specified by environment variable "SERVERHOST".

   This started out with an example in W. Richard Stevens' book
   "Advanced Programming in the Unix Environment".  I have
   modified it quite a bit, including changes to make use of my
   own re-entrant version of functions in echolib.
   
   Ted Baker
   February 2015

 */

#include "config.h"
#include "echolib.h"
#include "checks.h"
#include <sys/time.h>

/* the main service loop of the client; assumes sockfd is a
   connected socket */

int debug_op = 0;

void random_arr(int n, char* sendline){
   int* array = (int*)malloc(sizeof(int)*n);
   sendline[0] = '\0';
   char temp[20];
   srand(time(NULL));
   if(debug_op==0){
      for(int i=0;i<n;i++){
         array[i] = rand()%300+2;
         sprintf(temp, "%d", array[i]);
         strcat(sendline, temp);
         strcat(sendline, " ");
      }
   }
   else{
      for(int i=0;i<n;i++){
         array[i] = i+2;
         sprintf(temp, "%d", array[i]);
         strcat(sendline, temp);
         strcat(sendline, " ");
      }
   }
   sendline[strlen(sendline)-1] = '\n';
}

void
client_work (int sockfd) {
  connection_t conn;
  char *p;
  char sendline[MAXLINE], recvline[MAXLINE];
  char send[MAXLINE];
  connection_init (&conn);
  conn.sockfd = sockfd;
  while ((p = fgets (sendline, sizeof (sendline), stdin))) {
    int num = atoi(sendline);
    random_arr(num, send);
    printf("Send Data : %s\n", send);
    CHECK (writen (&conn, send, strlen (send)));
    if (readline (&conn, recvline, sizeof (recvline)) <= 0)
      ERR_QUIT ("str_cli: server terminated connection prematurely");
    fprintf (stdout, "%s", recvline); /* rely that line contains "/n" */
    fflush (stdout);
  }
  /* null pointer returned by fgets indicates EOF */
}

/* fetch server port number from main program argument list */
int
get_server_port (int argc, char **argv) {
  int val;
  if(debug_op==1){
   return atoi(argv[2]);
  }
  char * endptr;
  if (argc < 2) goto fail;
  errno = 0;
  val = (int) strtol (argv [1], &endptr, 10);
  if (*endptr) goto fail;
  if ((val < 0) || (val > 0xffff)) goto fail;
#ifdef DEBUG
  fprintf (stderr, "port number = %d\n", val);
#endif
  return val;
fail:
   fprintf (stderr, "usage: echosrv [port number]\n");
   exit (-1);
}

/* set up IP address of host, using DNS lookup based on SERVERHOST
   environment variable, and port number provided in main program
   argument list. */
void
set_server_address (struct sockaddr_in *servaddr, int argc, char **argv) {
  struct hostent *hosts;
  char *server;
  const int server_port = get_server_port (argc, argv);
  if ( !(server = getenv ("SERVERHOST"))) {
    QUIT ("usage: SERVERHOST undefined.  Set it to name of server host, and export it.");
  }
  memset (servaddr, 0, sizeof(struct sockaddr_in));
  servaddr->sin_family = AF_INET;
  servaddr->sin_port = htons (server_port);
  if ( !(hosts = gethostbyname (server))) {
    ERR_QUIT ("usage: gethostbyname call failed");
  }
  servaddr->sin_addr = *(struct in_addr *) (hosts->h_addr_list[0]);
}

int
main (int argc, char **argv) {
   int sockfd, c;
   struct sockaddr_in servaddr;
   struct timeval start, stop;
   
   while((c=getopt(argc, argv, "d"))!=-1){
      switch(c){
      case 'd' : 
         debug_op = 1;
         break;
      case '?' :
         printf("Unknown flag : %c\n", optopt);
         break;
       }
   }
   
   /* time how long we have to wait for a connection */
   CHECK (gettimeofday (&start, NULL));
   set_server_address (&servaddr, argc, argv);
   if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    ERR_QUIT ("usage: socket call failed");
   }
   CHECK (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)));
   CHECK (gettimeofday (&stop, NULL));
   fprintf (stderr, "connection wait time = %ld microseconds\n",
            (stop.tv_sec - start.tv_sec)*1000000 + (stop.tv_usec - start.tv_usec));
   client_work (sockfd);
   exit (0);
}
