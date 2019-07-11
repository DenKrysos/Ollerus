/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include "ollerus.h"


/*
 * Cfg-File:
 * Contains some runtime-stuff like:
 * Runs this Platform as Server (Access Point) or Client?
 * The Ollerus loads the settings on startup.
 * You can set the stored values directly in the cfg-File or
 * the Ollerus itself is capable of setting some values on runtime.
 *
 * Use of Config-File:
 * Try to open:
 * Doesn't exist? -> createcfgfile (does it with some default-values)
 * Otherwise read through the file and setup the actual instantiation of the process
 * validatecfgfile: could someday be implemented and checks the file for correct syntax
 * and tells you where a error lies (if any)
 * Syntax of cfg-file isn't correct? (You, any program or maybe Ollerus itself damaged the file...):
 * repaircfgfile trys to extract some left valid settings from
 * the damaged file and pastes them in a new one.
 * (Old, damaged file will be overwritten)
 */
/*
 * Config-File Values:
 * server:
 * 0 if this platform runs as client
 * 1 if running as server
 */
/*
 * Initial thoughts of author:
 * Make Ollerus run once (best without commandline prompts)
 * to force him to make a default cfg-file.
 * In this case it detects if your platform runs in infrastructure mode.
 * (With this information it sets the server-value on creating the default cfg-file)
 * Afterwards you could manual change this value in the cfg, if this platform
 * changes its network-type.
 * On every startup Ollerus reads in the cfg-file.
 * If it isn't the first run on platform it gets the server-attribute
 * and runs the keep_txpower_min (or other dependents as well...) as the right instance.
 */
static int validatecfgfile () {

}

static int repaircfgfile () {

}

static int createcfgfile () {

}

static int readcfgfile (char *attribute, char *value) {

}
static int readcfgfile_whole () {

}

static int writecfgfile (char *attribute, char *value) {

}

static int listen_to_console_commands () {

}









	//	pthreads Example
//void *PrintHello(void *threadid)
//{
//   long tid;
//   tid = (long)threadid;
//   printf("Hello World! It's me, thread #%ld!\n", tid);
//   pthread_exit(NULL);
//}
//
//int main (int argc, char *argv[])
//{
//   pthread_t threads[NUM_THREADS];
//   int rc;
//   long t;
//   for(t=0; t<NUM_THREADS; t++){
//      printf("In main: creating thread %ld\n", t);
//      rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
//      if (rc){
//         printf("ERROR; return code from pthread_create() is %d\n", rc);
//         exit(-1);
//      }
//   }
//
//   /* Last thing that main() should do */
//   pthread_exit(NULL);
//}
