#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>

#include <stdio.h>

#include <ibaard.h>
#include <ibaard_fs.h>
#include <ibaard_cat.h>
#include <ibaard_log.h>

#include "config.h"

#ifndef POLLRDNORM
#warning Your system stinks.
#define POLLRDNORM 0x0040
#endif

enum status {ST_UNSET, ST_DOWN, ST_UP, ST_WAITUP, ST_ERR};
enum limits {LT_MAXNAME=255};
typedef struct _childstruct cs;
int waitingchilds=0;
char *servicedir;
//char servicedir[255]="/etc/asvm/servic";

struct _childstruct {
    pid_t pid;
    time_t starttime;
    time_t endtime;
    char status;
    char name[LT_MAXNAME];
    char respawn;
    cs *next;
};

static cs *cs_storage=NULL;

void cs_startall(){
  cs *p;
  pid_t pid;
  for (p=cs_storage;p!=NULL;p=p->next){
    if (p->status==ST_WAITUP){
      logmsg(L_DEBUG, "ST", "Checking service '", p->name, "'", NULL);
      if (p->respawn>=3 && time(NULL)-p->endtime<=60) {
        if (p->respawn != 99) {
          p->respawn=99;
          logmsg(L_WARNING, "F", "Disabling service '", p->name, "' because it's respawning too fast", NULL);
        }
        alarm(60);
        continue;
      }
      else if (p->respawn>=3) p->respawn=0;

      if (p->endtime-p->starttime<=10){
        p->respawn++;
      }

      logmsg(L_INFO, "ST", "Starting service '", p->name, "'", NULL);
      pid=fork();
      if (pid==0){
        execlp(cati(servicedir, p->name, "/run", NULL), "run", (char*)NULL);
        exit(-1);
      } else if (pid > 0){
        p->pid=pid;
        p->status=ST_UP;
        p->starttime=time(NULL);
        continue;
      }
    }
  }
}

void cs_killall(){
  cs *p;

  for (p=cs_storage;p!=NULL;p=p->next){
    if (p->status==ST_UP) {
      logmsg(L_INFO, "ST", "Trying to kill service '", p->name, "'", NULL);
      p->status=ST_DOWN;
      kill(p->pid, SIGTERM);
    }
  }
}

int cs_upcount(){
  cs *p;
  int cnt=0;

  for (p=cs_storage;p!=NULL;p=p->next){
    if (p->status==ST_UP) cnt++;
  }

  return cnt;
}

int cs_find(cs key, cs **result){
  cs *p;

  for (p=cs_storage;p!=NULL;p=p->next){
    if (key.pid){
      if (p->pid==key.pid);
      else continue;
    }
    if (key.status){
      if (p->status==key.status);
      else continue;
    }
    if (key.name!=NULL){
      if (strcmp(key.name, p->name)) continue;
    }
    *result=p;
    return 0;
  }
  return -1;
}

char *cs_findnamebypid(pid_t pid){
  cs *p;

  for (p=cs_storage;p!=NULL;p=p->next){
    if (p->pid==pid)
      return p->name;
  }
  return NULL;
}

int cs_findbyname(char *name){
  cs *p;
  for (p=cs_storage;p!=NULL;p=p->next){
    if (!strcmp(p->name, name))
      return 0;
  }
  return -1;
}

void cs_printstat(int fd, cs *key){
  char buf[512];
  switch(key->status){
    case ST_UP:
      snprintf(buf, 512, "+ %s (%i): up %i seconds\n",
               key->name, key->pid, (int)time(NULL)-(int)key->starttime);
      break;
    case ST_WAITUP:
      snprintf(buf, 512, "+ %s (%i): down %i seconds, should be up\n",
               key->name, key->pid, (int)time(NULL)-(int)key->endtime);
      break;
    case ST_DOWN:
      snprintf(buf, 512, "+ %s (%i): disabled since %i seconds\n",
               key->name, key->pid, (int)time(NULL)-(int)key->endtime);
      break;
    default:
      snprintf(buf, 512, "+ %s (%i): %i, start: %i, end: %i\n",
               key->name, key->pid, key->status, (int)time(NULL)-(int)key->starttime, (int)time(NULL)-(int)key->endtime);
  }
  __writefd(fd, buf);
}

void cs_dump(cs *key){
  printf("addr: %p, pid: %i, name: %s, status: %i, stime: %i, etime: %i, next: %p\n", key,
         key->pid, key->name, key->status, (int)key->starttime, (int)key->endtime, key->next);
}

void cs_dumpall(){
  cs *p;
  for (p=cs_storage;p!=NULL;p=p->next){
    cs_dump(p);
  }
}

int cs_additem(cs *key){
  cs *p;

  if (cs_storage==NULL){
    if ((cs_storage=malloc(sizeof(cs)))==NULL){
      return -1;
    }
    memcpy(cs_storage, key, sizeof(cs));
    cs_storage->starttime=time(NULL);
    cs_storage->next=NULL;
  } else {
    p=cs_storage;
    while (p->next != NULL) p=p->next;

    if ((p->next=malloc(sizeof(cs))) == NULL) {
      logmsg(L_ERROR, "AD", "unable to malloc() memory for new service", NULL);
      return -1;
    }
    key->starttime=time(NULL);
    key->next=NULL;
    memcpy(p->next, key, sizeof(cs));
  }
  return 0;
}

int cs_signal(char *service, int signal){
  cs cskey, *csresult=NULL;

  if (service==NULL) return -1;

  memset(&cskey, 0, sizeof(cs));
  strncpy(cskey.name, service, LT_MAXNAME);
  if (!cs_find(cskey, &csresult)){
    //__writefd(outfifo, cati("+ Service ", cskey.name, " enabled\n", NULL));
    if (csresult->pid!=0) return(kill(csresult->pid, signal));
  }
  return -1;
}

// reads the service directory and creates a cs
// entry for each service, with status 2
int readservices(){
  DIR* dir_ptr;
  struct dirent *temp;
  cs cskey;

  if ((dir_ptr=opendir(servicedir))==NULL){
    perror("Unable to open dir");
    return 0;
  }

  for (temp=readdir(dir_ptr); temp!=NULL; temp=readdir(dir_ptr)){
    if (!strcmp(temp->d_name, ".")) continue;
    if (!strcmp(temp->d_name, "..")) continue;
    if (tf(cati(servicedir, temp->d_name, "/run", NULL))) continue;

    memset(&cskey, 0, sizeof(cs));
    cskey.endtime=time(NULL);
    if (!tf(cati(servicedir, temp->d_name, "/noauto", NULL))) cskey.status=ST_DOWN;
    else cskey.status=ST_WAITUP;
    strncpy(cskey.name, temp->d_name, LT_MAXNAME);
    cs_additem(&cskey);
  }
  return 0; // FIXME
}

int addservice(char* service){
  cs cskey;

  if (!tf(cati(servicedir, service, "/run", NULL))){
    memset(&cskey, 0, sizeof(cs));
    cskey.endtime=time(NULL);
    cskey.status=ST_DOWN;
    strncpy(cskey.name, service, LT_MAXNAME);
    cs_additem(&cskey);
    return 0;
  }
  return -1;
}

void sighandle(int signum){
  switch(signum){
    case SIGCHLD:
      waitingchilds++;
      break;
    case SIGALRM:
      //cs_startall();
      break;
    case SIGTERM:
      cs_killall();
      break;
  }
}

void writestatus(int status, int fd){
  if (status==0)
    __writefd(fd, cati("+\n", NULL));
  else
    __writefd(fd, cati("-\n", NULL));
}

int main(int argc, char **argv){
  (void) argc;
  (void) argv;
  int infifo, outfifo;
  cs cskey, *csresult=NULL;
  char *basedir;

  loglevel(4);

  basedir=getenv("ASVM_BASEDIR");
  if (basedir==NULL){
    basedir=malloc(strlen(BASEDIR)+1);
    strcpy(basedir, BASEDIR);
  }

  cat(&servicedir, basedir, "/services/", NULL);

  readservices();

  /*
    - read service directory
    - start all services
    - register signal handlers
    - endless loop
    - restart dying children
    - listen on fifo for commands to enable/disable childs
  */

  infifo=open(cati(basedir, "/in", NULL), O_RDWR);
  if (infifo<0) logmsg(L_DEADLY, "MAIN", "Unable to open in FIFO", NULL);
  outfifo=open(cati(basedir, "/out", NULL), O_RDWR);
  if (outfifo<0) logmsg(L_DEADLY, "MAIN", "Unable to open out FIFO", NULL);

  struct sigaction action;
  action.sa_handler = sighandle;
  action.sa_flags   = SA_RESTART;
  sigemptyset(&action.sa_mask);

  sigaction(SIGCHLD, &action, NULL);
  sigaction(SIGALRM, &action, NULL);
  sigaction(SIGTERM, &action, NULL);

  cs_startall();

  struct  pollfd pfd[0];
  char buf[255], optarg[254];
  int i;
  pfd[0].fd=infifo;
  pfd[0].events=POLLRDNORM;
  for (;;){
    g_waitingchilds:
    if (waitingchilds>0){
      int status;
      pid_t pid;
      cs *p;

      pid=wait(&status);
      waitingchilds--;

      printf("Waiting childs: %i\n", waitingchilds);
      for (p=cs_storage;p!=NULL;p=p->next){
        if (p->pid==pid){
          if (p->status!=ST_DOWN) p->status=ST_WAITUP;
          p->endtime=time(NULL);
          p->pid=0;
        }
      }
      if (waitingchilds>0) goto g_waitingchilds;
    }

    cs_startall();

    buf[0]='\0';
    optarg[0]='\0';
    poll(pfd, 1, -1);

    if (pfd[0].revents & POLLRDNORM)  {
      i=read(pfd[0].fd, buf, 200);
      buf[i]='\0';
      if (i<1) continue;
      if (i>3) {
        strncpy(optarg, buf+2, i-3);
        optarg[i-3]='\0';
      }

      switch(buf[0]){
        case 'A':
          writestatus(cs_signal(optarg, SIGALRM), outfifo);
          break;
        case 'C':
          writestatus(cs_signal(optarg, SIGCONT), outfifo);
          break;
        case 'D':
          if (optarg==NULL) break;
          memset(&cskey, 0, sizeof(cs));
          strncpy(cskey.name, optarg, LT_MAXNAME);
          if (!cs_find(cskey, &csresult)){
            printf("enabling %s\n", cskey.name);
            __writefd(outfifo, cati("+ Service ", cskey.name, " disabled\n", NULL));
            csresult->status=ST_DOWN;
            if (csresult->pid!=0) kill(csresult->pid, SIGTERM);
          } else __writefd(outfifo, cati("- Service ", cskey.name, " not found\n", NULL));
          break;
        case 'H':
          writestatus(cs_signal(optarg, SIGHUP), outfifo);
          break;
        case 'I':
          writestatus(cs_signal(optarg, SIGINT), outfifo);
          break;
        case 'K':
          writestatus(cs_signal(optarg, SIGKILL), outfifo);
          break;
        case 'N':
          // add a new service
          if (optarg==NULL) break;
          memset(&cskey, 0, sizeof(cs));
          strncpy(cskey.name, optarg, LT_MAXNAME);
          if (!cs_find(cskey, &csresult)) {
            __writefd(outfifo, cati("- Service ", optarg, " already exists\n", NULL));
            break;
          }
          if (addservice(optarg)==0)
            __writefd(outfifo, cati("+ Service ", optarg, " added\n", NULL));
          else
            __writefd(outfifo, cati("- Service ", optarg, " could not be added\n", NULL));
          break;
        case 'O':
          writestatus(0, outfifo);
          break;
        case 'P':
          writestatus(cs_signal(optarg, SIGSTOP), outfifo);
          break;
        case 'U': // bring service up
          if (optarg==NULL) break;
          memset(&cskey, 0, sizeof(cs));
          strncpy(cskey.name, optarg, LT_MAXNAME);
          if (!cs_find(cskey, &csresult)){
            __writefd(outfifo, cati("+ Service ", cskey.name, " enabled\n", NULL));
            csresult->status=ST_WAITUP;
          } else __writefd(outfifo, cati("- Service ", cskey.name, " not found\n", NULL));
          break;
        case 'S': // get service status
          if (optarg!=NULL){
            if (!strncmp(optarg, "*", 1)) {
              cs_dumpall();
              writestatus(0, outfifo);
            } else {
              memset(&cskey, 0, sizeof(cs));
              strncpy(cskey.name, optarg, LT_MAXNAME);
              if (!cs_find(cskey, &csresult)) cs_printstat(outfifo, csresult);
              else __writefd(outfifo, cati("- Service ", cskey.name, " not found\n", NULL));
            }
          }
          break;
        case 'T':
          writestatus(cs_signal(optarg, SIGTERM), outfifo);
          break;
        case 'X':
          cs_killall();
          __writefd(outfifo, cati("+ Shutting down asvm.\n", NULL));
          exit(0);
          break;
        default:
          __writefd(outfifo, cati("- Unknown command: ", buf, "\n", NULL));
      } // end switch
    }
  } // end for
}
