/* option.c $Revision: 1.1 $ $Date: 2009/05/11 09:02:51 $
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

/* disable Warning about deprecated C Standard Function */
#pragma warning(disable : 4996)

/* this file 
   explains the usage
   contains the defaults
   ( not implemented: reads the standard configuration file (if any))
   and processes the commandline
   ( not implemented: reads the secondary configuration file (if any))
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "main.h"
#include "option.h"

#define MAXLINE 4*1024
static char *configfile = NULL;
static char configline[MAXLINE];
static int configlinenr = 0;


#ifdef DEBUG
#define debug(msg) message(1,msg)
#else
#define debug(msg)
#endif

static void warning(char * msg)
{ static char txt[300];
  static char shortline[200];
  strncpy(shortline,configline,199);
  shortline[199]=0;
  if (configfile==NULL) 
    sprintf(txt,"Error in commandline %s: %s",shortline, msg);
  else
    sprintf(txt,"Error in file %s, line %d: %s, %s",configfile, configlinenr,shortline,msg);
  errormsg(txt,0);
}

 
 /* a string to store teporarily an option */

char tmp_option[MAXTMPOPTION]= {0};

void set_option(char **option, char *str)
/* deallocate *option if necessary, allocate if necesarry, and fill with the the given string */
{ unsigned int n;
  if (str==NULL)
  { if (*option!=NULL)
	{ free(*option);
	  *option = NULL;
	}
  }
  else
  {  n = strlen(str);
     if (*option != NULL && strlen(*option) < n)  
	 { free(*option);
       *option = NULL;
	 }
     if (*option == NULL)
	 { *option = malloc(n+1);
	 }
     if (*option == NULL)
	 { fatal_error("Out of Memory for option");
       return;
	 }
     strcpy(*option, str);
  }
}

#define hexdigit(c) ((c)<10? (c)+'0':(c)+'A')

int strtoint(char *arg)
{ int r = 0;
  int sign = 1;
  while(isspace(*arg)) arg++;
  if (*arg=='-') { arg++; sign=-1;}
  if (strncmp(arg,"0x",2)==0 || strncmp(arg,"0X",2)==0) /* hex */
  { arg = arg+2;
	while (isxdigit(*arg))
	{ unsigned int x;
	  if (isdigit(*arg)) x = *arg - '0'; 
	  else if (isupper(*arg)) x = *arg - 'A';
	  else x = *arg -'a';
	  r = (r<<4) + x;
	  arg++;
	}
  }
  else /* decimal */
      while (isdigit(*arg))
	  {unsigned int d;
       d = *arg -'0';
	   r = r*10+d;
	   arg++;
	  }
  return sign*r;
}

int arguments;

static 
int do_option(option_spec *p, unsigned char *arg)
{ unsigned int n;
  debug("processing option:");
  debug(p->longopt);
  switch (p->kind)
  { case str_arg: 
      if (*(p->handler.str)!=NULL)
        free(*(p->handler.str));
      if (arg==NULL)
	  { errormsg("Argument expected",0);
	    return 1;
	  }
	  n = strlen(arg);
	  while (n>0 && isspace(arg[n-1]))
	  { n--; arg[n]=0;}
	  while (n>0 && isspace(arg[0]))
	  { n--; arg++;}
      if (n>0 && arg[0]=='"'){ n--; arg++;}
	  if (n>0 && arg[n-1]=='"'){ n--; arg[n]=0;}
	  if (n>0)
      { *(p->handler.str) =  malloc(n+1);
	    if (*(p->handler.str)==NULL)
			errormsg("Out of memory",0); 
		else
	        strcpy(*(p->handler.str), arg);
	  }
	  else
		*(p->handler.str) = NULL;
      return 1;
    case int_arg:
      if (arg==NULL)
		  errormsg("Argument expected",0);
	  else
          *(p->handler.i)=strtoint(arg);
      return 1;
    case tgl_arg:
      *(p->handler.i)= !(*(p->handler.i));
      return 0;
    case inc_arg:
      (*(p->handler.i))++;
      return 0;
    case on_arg:
      *(p->handler.i)= 1;
      return 0;
    case off_arg:
      *(p->handler.i)= 0;
      return 0;
    case fun_arg:
      debug("calling handler");
      return (p->handler.f)(arg);
    default:
      errormsg("unknown option type",p->kind);
      return 0;
  }
}

static
int  do_option_long(unsigned char *cmd, unsigned char *arg)
/* returns 1 if argument was used 0 otherwise */
{  int i;
   unsigned int k;
   static char msg[100];
   debug("searching for option:");
   debug(cmd);
   i=0;
   while (1)
   { if (options[i].description==NULL)
      { warning("Unknown Option");
        return 0;
      }
      k = strlen(options[i].longopt);
      if (strlen(cmd)>k && cmd[k] == '=')
      { if (strncmp(cmd,options[i].longopt,k) == 0)
        { do_option(options + i, cmd + k+1);
          return 0;
        }
      }
      else if (strcmp(cmd,options[i].longopt)== 0)
        return  do_option(options + i, arg);
      i++;
    }
}

static
int  do_option_short(unsigned char cmd, unsigned char *arg)
/* returns 1 if argument was used 0 otherwise */
{  int i;
   static unsigned char cmdstr[] = "- ";

   i=0;   
   cmdstr[1] = cmd;
   debug("searching for option:");
   debug(cmdstr);
   
   while (1)
   { if (options[i].description==NULL)
      { warning("Unknown Option");
        return 0;
      }
      if (options[i].shortopt!=0 && cmd==options[i].shortopt)
        return  do_option(options + i, arg);
      i++;
    }
}


void option_defaults(void)
{ option_spec *p;
  int i;
  
  i=0;
  p= &options[i];
  while(p->description!=NULL)
  { if (p->kind == on_arg)
      *(p->handler.i) = 0;
    else if (p->kind == off_arg)
      *(p->handler.i) = 1;
    else if (p->default_str != NULL)
      do_option(p,p->default_str);
    p = &options[++i];
  }
}



int write_configfile(char *filename)
{ FILE *out;
  option_spec *p;

  if (filename==NULL)
  { errormsg("Filename expected",0);
    return 0;
  }
  debug("writing configfile");
  debug(filename);
  out=fopen(filename,"w");
  if (out==NULL) 
	  {  errormsg("Could not write configuration file",0);
	     return 0;
	  }
  fprintf(out,"#m3w configfile: %s\n\n",filename);
  for (p= options; p->description!=NULL; p++)
  switch (p->kind)
  { case fun_arg:
    case tgl_arg: 
      continue;
    case str_arg:
      fprintf(out,"\n#%s\n",p->description);
      fprintf(out,"%s ", p->longopt);
	  if (*(p->handler.str)!=NULL)
        fprintf(out,"%s",*(p->handler.str));
	  fprintf(out,"\n");
	  continue;
    case on_arg:
      fprintf(out,"\n#%s\n",p->description);
	  if (!(*(p->handler.i))) 
		fprintf(out,"#");
	  fprintf(out,"%s\n", p->longopt);
      continue;
    case off_arg:
      fprintf(out,"\n#%s\n",p->description);
	  if (*(p->handler.i)) 
		fprintf(out,"#");
	  fprintf(out,"%s\n", p->longopt);
      continue;
    case inc_arg:
      fprintf(out,"\n#%s\n",p->description);
	  { int i;
	    if (*(p->handler.i)<=0)
		  fprintf(out,"#%s\n", p->longopt);
		else
	      for (i=0; i<*(p->handler.i); i++) 
    	    fprintf(out,"%s\n", p->longopt);
	  }
      continue;
    default:
      fprintf(out,"\n#%s\n",p->description);
      fprintf(out,"%s %d\n", p->longopt, *(p->handler.i));
      continue;
  }
  fclose(out);
  debug("done writing configfile");
  return 1;
}

int parse_configfile(char *filename)
{ FILE *in;

  unsigned char *cmd, *arg, *p;    

  if(filename==NULL)
  { errormsg("Argument expected",0);
    return 0;
  }
  message(0,"reading:");
  message(0,filename);
  in=fopen(filename,"r");
  if (in==NULL)
	return 0;
  configfile = filename;
  configlinenr = 0;
  while(!feof(in))
  {  fgets(configline,MAXLINE,in);
     configlinenr++;
     if(feof(in)) break;

     /* skip spaces */
     p =configline; 
     while(isspace((int)(p[0])))
       p++;

     /* ignore comments and empty lines*/
     if (p[0]=='#' || p[0] == 0) 
       continue;

     /* command found */
     cmd = p; 
     /* convert command to lowercase */
     while(isalnum((int)(*p)))
     { *p = tolower(*p);
       p++;
     }

     /* skip space */
     if (p[0]!=0)
     { p[0]=0; /* terminate command */
       p++;
       while(isspace((int)(p[0])))
         p++;
     }
     arg = p;

	 
     /* here we have cmd and arg pointing to the right places */
     
     do_option_long(cmd,arg);
  }
  fclose(in);
  configfile=NULL;
  configlinenr=0;
  debug("done configfile");
  return 1;
}

static 
unsigned char *parse_argument(unsigned char **str)
/* makes *str point past the argument and returns the argument */
{ unsigned char *p, *arg;
  p = *str;
  if (*p == '\0') 
    arg = NULL;
  else
  { /* skip spaces */
    while(isspace((int)(p[0])))
      p++;
    if (*p=='\0' || *p=='-')
      arg = NULL;
    else
    { arg = p;
      if (*arg == '"')
      { arg++; p++;
        while(*p!= 0 && *p != '"')
          p++;
      }
      else
      { while(*p!= 0 && !isspace((int)(*p)))
          p++;
      }
      if (*p != 0)
      {*p = 0;
        p++;
      }
    }
  }
  *str = p;
  return arg;
}

void parse_commandstr(unsigned char *p)
{ unsigned char *cmd, *arg; 

  arguments = 0;
  debug("reading commandstr");
  while(*p != 0)
  {  while(isspace((int)(p[0]))) p++; /* skip spaces */
     if (p[0] == 0) /* done? */
       break;
     if (p[0] == '-')
     { p++;
       if (*p == 0 || isspace((int)(*p))) /* single - */
         do_argument(arguments++, "-");
       else if (*p == '-') /* double -- */
       { p++;
         if (*p == 0 || isspace((int)(*p))) /* single -- */
           do_argument(arguments++, "--");
         else  /* long command found */
         { cmd = p; 
           /* convert command to lowercase */
           while(isalnum((int)(*p)))
           { *p = tolower(*p);
             p++;
           }
           if (p[0]!=0)
           { p[0]=0; /* terminate command */
              p++;
           }
           arg = parse_argument(&p);
           if (do_option_long(cmd,arg)==0 && arg != NULL)
             do_argument(arguments++, arg);
         }
       }
       else 
       { unsigned char c;
         while(*p!=0 && !isspace((int)(*p)))
         { c = *p;
           p++;
           if (*p == 0)
 	       do_option_short(c, NULL);
           else if (isspace((int)(*p)))
           { arg = parse_argument(&p);
             if (do_option_short(c, arg)==0 && arg != NULL)
               do_argument(arguments++, arg);
             break;
           }
           else
 	       do_option_short(c, NULL);
         }
       }
     }
     else
       do_argument(arguments++, parse_argument(&p));
  }
  debug("done commandstr");
}

void parse_commandline(int argc, unsigned char *argv[])
{ int i,j;

  debug("parsing commandline");
  arguments = 0;
  for (i=0; i<argc; i++)
  { if (argv[i][0] == '-' && argv[i][1] != 0)
    {  if (argv[i][1] == '-' && argv[i][2] != 0)
	 i = i+ do_option_long(argv[i]+2, argv[i+1]);
       else
       { for (j=1;argv[i][j]!=0 && argv[i][j+1]!=0;j++)
	   do_option_short(argv[i][j], NULL);
	 i = i+ do_option_short(argv[i][j], argv[i+1]);
       }
    }
    else
       do_argument(arguments++, argv[i]);
  }
  debug("done commandline");
}


void option_usage(HWND hText)
{ option_spec *p;
  int i;
  char line[1000];
  i=0;
  p= options;
  SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)   
	          "#The following options can be used on the commandline\r\n"
			  "#or in configuration files.\r\n\r\n");
  while(p->description!=NULL)
  { if (p->shortopt)
    {  sprintf(line,"-%c  ",p->shortopt);
       SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);
    }
	sprintf(line,"--%s",p->longopt);
    SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);
    if (p->arg_name != NULL)
	{  sprintf(line," <%s>", p->arg_name);
	   SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);
	}
    sprintf(line,"\r\n    %s ",p->description);
    SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);

    if (p->default_str != NULL)
	{  sprintf(line,"\t(default = %s)",p->default_str);
       SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);
	}
    sprintf(line,"\r\n\r\n");
    SendMessage(hText,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)line);

    p++;
  }

}