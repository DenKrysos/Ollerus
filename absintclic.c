/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */

/* absintclic is abbreviation for:
 * 						absint cli ctrl
 * 						abstract-interface command-line-interface controller
 * Ähm, no, better abbreviation:
 * Abstract Interface Controller Command Line Interface via detached Call
 * I.e. it is the external command line interface for the abstract interface controller
 * Call it, when a controller is currently running to pass it a command by for example a system();
 * or just from a different terminal.
 */


#include "ollerus_globalsettings.h"
#include "ollerus_base.h"

//#include "absint.h"
//#include "absint_ctrl.h"
#include "remainder.h"
#include "debug.h"
#include "absint_ctrl_cli.h"







char ansi_escape_use;


char **args;




int main(int argc, char **argv) {
	int err;err=0;
	args=argv;
	SET_ansi_escape_use;

	depr(cliexterntest);
	return err;
}
