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

/*
 * TODO:
 * - support pathnames (cut off last part) for better compatibility with
 *   daemontools
 */

int rwcon(char *action){
  int infifo, outfifo, i;
  char buf[512];
  char *laction=strdup(action);
  char *basedir;

  basedir=getenv("ASVM_BASEDIR");
  if (basedir==NULL){
    basedir=malloc(strlen(BASEDIR));
    strcpy(basedir, BASEDIR);
  }

  infifo=open(cati(basedir, "/in", NULL), O_RDWR);
  if (infifo<0) logmsg(L_DEADLY, "MAIN", "Unable to open in FIFO", NULL);
  outfifo=open(cati(basedir, "/out", NULL), O_RDWR);
  if (outfifo<0) logmsg(L_DEADLY, "MAIN", "Unable to open out FIFO", NULL);

  __writefd(infifo, laction);
  if ((i=read(outfifo, buf, 512))>0){
    buf[i]='\0';
    if (i>2)
      __write1(buf+2);
  }
  if (buf[0]=='+') return 0;
  if (buf[0]=='-') return 1;
  return 2;
}

void usage(){
  __write2(cati("asvm build ", VERSION, "\n\n",
                "Usage: \n",
                "-a     send SIGALRM\n",
                "-c     send SIGCONT\n",
                "-d     disable service\n",
                "-h     send SIGHUP\n",
                "-i     send SIGINT\n",
                "-k     send SIGKILL\n",
                "-n     add a new service\n",
                "-o     check asvm status\n",
                "-p     send SIGSTOP\n",
                "-s     print service status\n",
                "-t     send SIGTERM\n",
                "-u     enable service\n",
                "-v     print version number and usage information\n",
                "-x     shut down asvm (and kill all services)\n",
                "\n",
                NULL));
}

int main(int argc, char **argv){
  int c;

  if (!strcmp(argv[0]+(strlen(argv[0])-6), "svstat")){
    // loop through argv and print status for each service
    for (argv++;*argv!=NULL;argv++){
      rwcon(cati("S ", *argv, "\n", NULL));
    }
    return 0;
  }
  if (!strcmp(argv[0]+(strlen(argv[0])-4), "svok")){
    // check if asvm is running and exit 0 on success, 100 on failure
    return 0;
  }

  while ((c=getopt(argc, argv, "a:c:d:h:i:k:n:o:p:s:t:u:vx")) != EOF){
    switch(c){
      case 'a':
        // send SIGALRM to service
        return rwcon(cati("A ", optarg, "\n", NULL));
      case 'c':
        // send SIGCONT to service
        return rwcon(cati("C ", optarg, "\n", NULL));
      case 'd':
        // disable service
        return rwcon(cati("D ", optarg, "\n", NULL));
      case 'h':
        // send SIGHUP to service
        return rwcon(cati("H ", optarg, "\n", NULL));
      case 'i':
        // send SIGINT to service
        return rwcon(cati("I ", optarg, "\n", NULL));
      case 'k':
        // send SIGKILL to service
        return rwcon(cati("K ", optarg, "\n", NULL));
      case 'n':
        // add a new service
        return rwcon(cati("N ", optarg, "\n", NULL));
      case 'o':
        return rwcon(cati("O ", optarg, "\n", NULL));
      case 'p':
        // send SIGSTOP to service
        return rwcon(cati("P ", optarg, "\n", NULL));
      case 's':
        // get service status
        return rwcon(cati("S ", optarg, "\n", NULL));
      case 't':
        // send SIGTERM to service
        return rwcon(cati("T ", optarg, "\n", NULL));
      case 'u':
        // enable service
        return rwcon(cati("U ", optarg, "\n", NULL));
      case 'v':
        // print version number and usage information
        usage();
        break;
      case 'x':
        return rwcon(cati("X", optarg, "\n", NULL));
    }
  }
  return 0;
}
