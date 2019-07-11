#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include <string.h>
#include <pthread.h>

#include "ollerus.h"

#define THREADS_HOSTAPD_MAX 3


#define USAGE "usage: %s \n"
#define DOIT 0x01

#ifndef MAPFILE
#define MAPFILE 0 /*für nicht BSD-Systeme*/
#endif

#define MAPSIZE 64

#define HOSTAPDPATH1 "//etc/wlan0.conf"
#define HOSTAPDPATH2 "//etc/wlan2.conf"


extern int errno;
//static volatile sig_atomic_t sflag;
//static sigset_t signal_neu, signal_alt, signal_leer;
//char *mapped;
//void sigfunc1(int);
//void sigfunc2(int);

struct hostapdThreadArgPassing {
	char *band;
	int thread_index;
};

struct bestchan{
	int band24;
	int band5;
	int active_interface;
	char AP_index;
};


void *hostapdup(void *arg){
	int thread_index = (((struct hostapdThreadArgPassing *)arg)->thread_index);
	printf("\t#%d Hostapd: Starting up.", thread_index);
	char *band = ((struct hostapdThreadArgPassing *)arg)->band;
	//Free the arguments passing memory space
	//here we also have to insert the hostapd path because the system call does not support variables
	if((*band)==0)
		system("hostapd -B //etc/wlan0.conf -P //var/run/hostapd.wlan0.pid");
	else
		system("hostapd -B //etc/wlan2.conf -P //var/run/hostapd.wlan2.pid");
	free(arg);
}


int printdat(struct bestchan *setchan,pthread_t *hostapdthread){
	int err;
	char *path="";
	char movflags=0;
	char new[3];

	if (setchan->band24 > 9){
		movflags = movflags | DOIT;
	}
	if(setchan->AP_index==0){
		path = HOSTAPDPATH1;
	}
	else
		path = HOSTAPDPATH2;

	sprintf(new,"%i",setchan->band24);
	char buf[4096];
	int fd, n=0;
	void *resmap;
	void *start;
	struct stat attr;



		if(( fd=open(path, O_RDWR)) < 0){
			fprintf(stderr, "Konnte %s nicht öffnen!!!\n",path);
		}
		if(fstat(fd, &attr) == -1){
			fprintf(stderr,"Fehler bei fstat.......\n");
		}

	/*Achtung jetzt legen wir die Datei die wir mit fd */
	/*spezifiert haben in den Arbeitsspeicher*/
		resmap=mmap(0, attr.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		start=resmap;
		if(resmap == ((caddr_t) -1)){
			fprintf(stderr, "%s: Fehler bei mmap mit der Datei %s\n"
							,strerror(errno), path);
		 }
	 /*Jetzt ist die Datei die mit fd geöffnet wurde im Arbeitsspeicher */

//		if(write(STDOUT_FILENO, resmap, attr.st_size) != attr.st_size)
//			  fprintf(stderr, "Fehler bei write\n");
		char*texp = strstr(resmap,"channel=");
		texp=texp + 8;
		resmap=texp;
		int i=0;
			while(*(texp+i)!='\n'){
				i++;
			}
		if(movflags&DOIT){

			if(i==1){
				int used=  resmap-start;
				printf("%i",used);
				memmove(resmap+1, resmap, attr.st_size-used);
			}
			*texp=new[0];
			*(texp+1)=new[1];
		}
		else{
			if(i==2){
				int used=  resmap+2-start;
				printf("%i",used);
				memmove(resmap+1, resmap+2, attr.st_size-used);
			}
			*texp=new[0];
		}
		close(fd);
//		snprintf(buf,4096,"%s",resmap);
//		printf("%s",buf);



		printf("\nThread argument passing");
		struct hostapdThreadArgPassing *pthreadArgPass;
		pthreadArgPass = malloc(sizeof(struct hostapdThreadArgPassing));
		pthreadArgPass->band=&setchan->AP_index;
		pthread_attr_t tattr;
		/* initialized with default attributes */
		if((err=pthread_attr_init(&tattr)) < 0) {
			perror("could not create thread-attribute");
			return MAIN_ERR_STD;
		}
		if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){
			perror("could not modify thread-attribute");
			if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
				perror("could not destroy thread-attribute");
				return MAIN_ERR_STD;
			}
			return MAIN_ERR_STD;
		}
		pthreadArgPass->thread_index = setchan->AP_index;
		printf("\nThread startet!");
		//Not any semaphore, synchronization, mutex needed for now
		//secure detached-state for the threads over attributed creation
		if( (err=pthread_create(hostapdthread, &tattr, hostapdup, (void*)pthreadArgPass)) < 0) {
			perror("could not create thread");
			if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
				printf("\n\tDEBUG 11\n");
				perror("could not destroy thread-attribute");
				return MAIN_ERR_STD;
			}
			return MAIN_ERR_STD;
		}
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
			perror("could not destroy thread-attribute");
			return MAIN_ERR_STD;
		}
	return 0;
}

int kill_AP (char band){

	char* pidfile="";
	if(band==0){
			pidfile = "//var/run/hostapd.wlan0.pid";
			printf("\n\tAccess Point on interface wlan0 is going to be terminated!\n");
		}
		else{
			pidfile = "//var/run/hostapd.wlan2.pid";
			printf("\n\tAccess Point on interface wlan2 is going to be terminated!\n");
		}
  FILE *f;
  int pid;

  if (!(f=fopen(pidfile,"r")))
    return 0;
  fscanf(f,"%d", &pid);
  fclose(f);
  if(pid!=0)
  kill(pid, SIGTERM);
  return 0;
}
