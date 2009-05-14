#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include <ibaard_log.h>
#include <ibaard_cat.h>
#include <ibaard.h>

#include "config.h"
#include "version.h"

void rwcon(char *action){
  int infifo, outfifo, i;
  char buf[512];
  char *laction=strdup(action);

  infifo=open(cati(BASEDIR, "/in", NULL), O_RDWR);
  if (infifo<0) logmsg(L_DEADLY, "MAIN", "Unable to open in FIFO", NULL);
  outfifo=open(cati(BASEDIR, "/out", NULL), O_RDWR);
  if (outfifo<0) logmsg(L_DEADLY, "MAIN", "Unable to open out FIFO", NULL);

  __writefd(infifo, laction);
  if ((i=read(outfifo, buf, 512))>0){
    buf[i]='\0';
    __write1(buf);
  }
}

void usage(){
  __write2(cati("asvm build ", VERSION, "\n\n",
                "Usage: ",
                NULL));
}

int main(int argc, char **argv){
  int c;

  if (!strcmp(argv[0]+(strlen(argv[0])-6), "svstat")){
    // loop through argv and print status for each service
    return 0;
  }
  if (!strcmp(argv[0]+(strlen(argv[0])-4), "svok")){
    // check service in argv[1] and exit 0 on success, 100 on failure
    return 0;
  }

  while ((c=getopt(argc, argv, "a:c:d:h:i:k:n:o:p:s:t:u:vx")) != EOF){
    switch(c){
      case 'a':
        // send SIGALRM to service
        rwcon(cati("A ", optarg, "\n", NULL));
        break;
      case 'c':
        // send SIGCONT to service
        rwcon(cati("C ", optarg, "\n", NULL));
        break;
      case 'd':
        // disable service
        rwcon(cati("D ", optarg, "\n", NULL));
        break;
      case 'h':
        // send SIGHUP to service
        rwcon(cati("H ", optarg, "\n", NULL));
        break;
      case 'i':
        // send SIGINT to service
        rwcon(cati("I ", optarg, "\n", NULL));
        break;
      case 'k':
        // send SIGKILL to service
        rwcon(cati("K ", optarg, "\n", NULL));
        break;
      case 'n':
        // add a new service
        rwcon(cati("N ", optarg, "\n", NULL));
        break;
      case 'o':
        rwcon(cati("O ", optarg, "\n", NULL));
        break;
      case 'p':
        // send SIGSTOP to service
        rwcon(cati("P ", optarg, "\n", NULL));
        break;
      case 's':
        // get service status
        rwcon(cati("S ", optarg, "\n", NULL)); 
        break;
      case 't':
        // send SIGTERM to service
        rwcon(cati("T ", optarg, "\n", NULL));
        break;
      case 'u':
        // enable service
        rwcon(cati("U ", optarg, "\n", NULL));
        break;
      case 'v':
        // print version number and usage information
        usage();
        break;
      case 'x':
        rwcon(cati("X", optarg, "\n", NULL));
        break;
    }
  }
  return 0;
}
