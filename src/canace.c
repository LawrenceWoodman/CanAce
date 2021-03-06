/* CanAce, a Jupiter ACE emulator
 *
 * Copyright (C) 1994 Ian Collier.
 * xz81 changes (C) 1995-6 Russell Marks.
 * xace changes (C) 1997 Edward Patel.
 * changes (C) 2010-12 Lawrence Woodman.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <SDL.h>
#include <tcl.h>
#include <tk.h>

#include "acescreen.h"
#include "tkwin.h"
#include "tkspooler.h"
#include "tktape.h"
#include "z80.h"
#include "tape.h"
#include "keyboard.h"
#include "spooler.h"

unsigned char mem[65536];
unsigned char *memptr[8] = {
  mem,
  mem+0x2000,
  mem+0x4000,
  mem+0x6000,
  mem+0x8000,
  mem+0xa000,
  mem+0xc000,
  mem+0xe000
};

int memattr[8]={0,1,1,1,1,1,1,1}; /* 8K RAM Banks */

static volatile timedInterrupt = 0;
int warpMode = 0;

/* Prototypes */
void startup(int *argc, char **argv);
void closedown(void);

/* Handle the SIGALRM signal used to sync the emulation speed,
 * check for tk events and refresh the screen.
 */
void
sigint_handler(int signum)
{
  timedInterrupt = 1;
}

/* Handle any Signals to do with quiting the program */
void
sigquit_handler(int signum)
{
  closedown();
  exit(1);
}

/* ints_per_sec   Interrupts per second up to 1000 */
static void
set_itimer(int ints_per_sec)
{
  struct itimerval itv;
  int freq = 1000/ints_per_sec;

  itv.it_interval.tv_sec = 0;
  itv.it_interval.tv_usec = (freq % 1000) * 1000;
  itv.it_value.tv_sec = itv.it_interval.tv_sec;
  itv.it_value.tv_usec = itv.it_interval.tv_usec;
  setitimer(ITIMER_REAL, &itv, NULL);
}

static void
normal_speed(void)
{
  set_itimer(50);
  warpMode = 0;
}

static void
warp_speed(void)
{
  set_itimer(50);
  warpMode = 1;
}

void
handle_cli_args(int argc, char **argv)
{
  int arg_pos = 0;
  char *cli_switch;

  while (arg_pos < argc) {
    cli_switch = argv[arg_pos];
    if (strcasecmp("-s", cli_switch) == 0) {
      if (strcmp("-S", cli_switch) == 0) {
        warp_speed();
      }

      if (++arg_pos < argc) {
        spooler_open(argv[arg_pos]);
      } else {
        fprintf(stderr, "Error: Missing filename for %s arg\n", cli_switch);
      }
    }
    arg_pos++;
  }
}

static void
setup_sighandlers(void)
{
  struct sigaction sa_quit;
  struct sigaction sa_alarm;
  memset(&sa_quit, 0, sizeof(sa_quit));
  memset(&sa_alarm, 0, sizeof(sa_alarm));

  sa_quit.sa_handler = sigquit_handler;
  sa_quit.sa_flags = 0;
  sa_alarm.sa_handler = sigint_handler;
  sa_alarm.sa_flags = SA_RESTART;

  // FIX: Replace the following with a loop through an array
  if (sigaction(SIGINT,  &sa_quit, NULL) < 0) goto error;
  if (sigaction(SIGHUP,  &sa_quit, NULL) < 0) goto error;
  if (sigaction(SIGILL,  &sa_quit, NULL) < 0) goto error;
  if (sigaction(SIGTERM, &sa_quit, NULL) < 0) goto error;
  if (sigaction(SIGQUIT, &sa_quit, NULL) < 0) goto error;
  if (sigaction(SIGSEGV, &sa_quit, NULL) < 0) goto error;
  if (sigaction(SIGALRM, &sa_alarm, NULL) < 0) goto error;
  return;

error:
  perror("sigaction failed");
  exit(1);
}

int
main(int argc, char **argv)
{
  if (!TkWin_init(mem))
    return EXIT_FAILURE;

  if (!AceScreen_init(TkWin_getWindowID(), SCALE, mem+0x2400, mem+0x2C00))
    return EXIT_FAILURE;

  tape_patches(mem);
  memset(mem+8192, 0xff, 57344);

  spooler_init(TkSpooler_observer, keyboard_clear, keyboard_keypress,
               normal_speed);
  setup_sighandlers();
  normal_speed();
  handle_cli_args(argc, argv);
  tape_init(TkTape_observer);
  keyboard_init();

  mainloop();
}

unsigned int
in(int h, int l)
{
  if(l==0xfe) /* keyboard */
    switch(h) {
      case 0xfe: return(keyboard_get_keyport(0));
      case 0xfd: return(keyboard_get_keyport(1));
      case 0xfb: return(keyboard_get_keyport(2));
      case 0xf7: return(keyboard_get_keyport(3));
      case 0xef: return(keyboard_get_keyport(4));
      case 0xdf: return(keyboard_get_keyport(5));
      case 0xbf: return(keyboard_get_keyport(6));
      case 0x7f: return(keyboard_get_keyport(7));
      default:  return(255);
    }
  return(255);
}

unsigned int
out(int h, int l, int a)
{
  return(0);
}

void
do_interrupt(void)
{
  static int count=0;

  count++;
  if (count >= 4) {
    count=0;
    spooler_read();
  }

  if (timedInterrupt) {
    AceScreen_refresh();
    TkWin_checkEvents();
    timedInterrupt = 0;
  }

}

void
closedown(void)
{
  spooler_close();
  tape_detach();
}
