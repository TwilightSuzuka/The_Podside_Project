/* option.h $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
 * This File is part of m3w. 
 *
 *	M3W a mp3 streamer for the www
 *
 *	Copyright (c) 2001, 2002 Martin Ruckert (mailto:ruckertm@acm.org)
 * 
 * m3w is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * m3w is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with m3w; if not, write to the
 * Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <stdio.h>


/* this is what you get: */

/* a string to store teporarily an option */
#define MAXTMPOPTION 1024
extern char tmp_option[MAXTMPOPTION];

extern void set_option(char **option, char *str);
/* deallocate *option if necessary, allocate if necesarry, and fill withe the given string */

extern void parse_commandline(int argc, unsigned char **argv);
/* parse the commandline handling options and arguments 
   short options consist of a "-" followed by the option character
   several short options might be combined -a -b -c is the same as -abc
   in this case only the last of these short oprions might have an argument string
   long options consist of a "--" followed by the option string
   "-" and "--" are not options.
   options may take an optional/required argument that follows the option
   separated by a space.
   in the case of long options only, a single "=" can be used to separate the
   option from the following argument
*/
extern void parse_commandstr(unsigned char *p);
/* like parse_commandline takes all the information from one string */
extern int parse_configfile(char *filename);
/* same for a configuration file (options only) 
   the file with the given filename is opened and read.
   empty/blank lines are ignored
   lines starting with a # character are comments and ignored.
   all other lines should start with a keyword and be followed
   by an optional value. The keywords are exactly the strings valid
   as long options. the semantics of a keyword line is the same as giving
   that same option on the command line with the rest of the line as argument.
   reading of configuration files can be recursive.
   parse_configfile can be used as a handler to handle config-file options
   (see below).
*/
extern int write_configfile(char *filename);
/* writes a configfile that can be read by parse_configfile */

extern void option_defaults(void);
/* extract the defaults from the options table and set them.
   on_args have an implicit default 0 and off_args have an implicit default 1
   all other options need an explicit string as default otherwise no
   default is set. (that is the programm is responsible for setting defaults
   otherwise.)
*/

extern int arguments;
/* number of arguments contained in commandline (so far)*/


/* this is what you must provide: */


extern void fatal_error(char *message);
/* a function to call if something goes wrong (should not return)*/
extern void message(int i, char *msg);
/* a function to call to display messages */
extern void errormsg(char *message, int error_code);
/* a function to call to display errormessages */
extern void do_argument(int pos, char *arg);
/* function to handle comand line arguments, called with
   the position 0,1,2, (not counting options in between)
   and the argument string. For position 0 the argument string
   will be the program name.
   Rationale: starting a program it must be provided with parameters.
   Parameters come in two types: named parameters (also called options)
   and positional parameters (also called arguments)
   Most of this package deals with handling the options, the rest of the
   the commandline are the arguments. For each argument, the function
   do_argument is called with the position number and the argument string.
 
   The special strings "-" and "--" are not considered (empty) options, but
   arguments.

   for example: consider a program with two options -v <arg> and --long
   -v taking an argument string, --long not.
   then the commandline 
      command -v abc def --long hij
   would call do_argument with something like do_argument(0,"/usr/local/bin/command"),
   do_argument(1,"def"),  and do_argument(2,"hij")
   after this the variable arguments would be set to 2.
 */


typedef enum {
  fun_arg, /* call a specified function with an argument */
  str_arg, /* store the argument ar string */
  int_arg, /* store the argument as int */
  tgl_arg, /* toggle the argument, an int, between 0 and 1 */
  on_arg,  /* set the argument, an int, to 1 */
  off_arg, /* set the argument, an int, to 0 */
  inc_arg  /* increment the argument, an int */
} option_type;  /* see below */


typedef
struct {
 char *description;      /* the human readable description of the option */
 char shortopt;          /* the character to indicate this option */
 char *longopt;          /* the name used as long option name */
 char *arg_name;         /* the name of the argument (only for description) */
 option_type kind;       /* the option/argument type */
 char *default_str;      /* string containing the deault value */
 union {                 /* the handler for this option */
   void *v;              /* dummy for internal use */
   char **str;           /* where to store a string */
   int *i;               /* where to store an int */
   int (*f)(char *arg); } handler; /* what function to call */
} option_spec;  /* see below */


extern option_spec options[];
/* table describing the options. terminated with an entry where the
   description is NULL.
   Each entry in the table describes an option
   we best describe it by example:

   option_spec options = {

   {"set input file to <file>",'i',"input","file",str_arg,"in.txt",{&s}},

    This option can be used as ... -i "string" or --input "string"
 
    option_defaults will set the variable s to "in.txt"
    and giving the option on the commandline (or in a configuration file)
    will change s to whatever value was given there.


   {"set priority to <number>",'p',"priority","number",int_arg,"8000",{&i}},

    This option can be used as ... -p 123 or --priority 4567

    option_defaults will set the variable i to 8000
    and giving the option on the commandline (or in a configuration file)
    will change i to whatever value was given there.

   {"turn on verbose mode",'v',"verbose",NULL,on_arg,NULL,{&flag}},

    This option can be used as ... -v or --verbose 

    option_defaults will set the variable flag to 0 (on_arg's are off by 
    default, off_args are on by default, toggle_args and inc_args
    need an explicit default.)
    and giving the option on the commandline (or in a configuration file)
    will change flag to 1 ( off_args are set to 0, toggle_args switch back 
    and forth between 0 and 1, inc_args get incremented each time.)


    {"read configuration from <file>",'f',"config","file",fun_arg,NULL,{parse_configfile}},

    this option can be used as ... -f f.cfg or --config f.cfg
    option_defaults will not do anything, since no default is given (NULL). if
    a default would be present, it would exectute the option as if given on the
    command line (or in a configuration file).
    executing an option of type fun_arg means calling the function given as the handler
    (parse_configfile) in this case with the argument string (if there is one or NULL
    if there is none) the function can do what ever it likes, it should test
    the argument for !=NULL before using it, and it should return 1 if the argument
    was used and 0 if the argument was not used.

   {NULL}
   terminated the table
   }

*/ 

extern void option_usage(HWND hText);