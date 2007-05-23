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

	while ((c=getopt(argc, argv, "a:c:d:h:i:k:o:p:s:t:u:")) != EOF){
		switch(c){
		case 'a':
			rwcon(cati("A ", optarg, "\n", NULL));
			break;
		case 'c':
			rwcon(cati("C ", optarg, "\n", NULL));
			break;
		case 'd':
			rwcon(cati("D ", optarg, "\n", NULL));
			break;
		case 'h':
			rwcon(cati("H ", optarg, "\n", NULL));
			break;
		case 'i':
			rwcon(cati("I ", optarg, "\n", NULL));
			break;
		case 'k':
			rwcon(cati("K ", optarg, "\n", NULL));
			break;
		case 'o':
			rwcon(cati("O ", optarg, "\n", NULL));
			break;
		case 'p':
			rwcon(cati("P ", optarg, "\n", NULL));
			break;
		case 's':
			rwcon(cati("S ", optarg, "\n", NULL)); 
			break;
		case 't':
			rwcon(cati("T ", optarg, "\n", NULL));
			break;
		case 'u':
			rwcon(cati("U ", optarg, "\n", NULL));
			break;
		}
	}
	return 0;
}
