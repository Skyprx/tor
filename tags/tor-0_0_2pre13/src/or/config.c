/* Copyright 2001,2002,2003 Roger Dingledine, Matej Pfajfar. */
/* See LICENSE for licensing information */
/* $Id$ */

#include "or.h"

/* enumeration of types which option values can take */
#define CONFIG_TYPE_STRING  0
#define CONFIG_TYPE_CHAR    1
#define CONFIG_TYPE_INT     2
#define CONFIG_TYPE_LONG    3
#define CONFIG_TYPE_DOUBLE  4
#define CONFIG_TYPE_BOOL    5

#define CONFIG_LINE_MAXLEN 4096

struct config_line {
  char *key;
  char *value;
  struct config_line *next;
};

static FILE *config_open(const unsigned char *filename);
static int config_close(FILE *f);
static struct config_line *config_get_commandlines(int argc, char **argv);
static struct config_line *config_get_lines(FILE *f);
static void config_free_lines(struct config_line *front);
static int config_compare(struct config_line *c, char *key, int type, void *arg);
static void config_assign(or_options_t *options, struct config_line *list);

/* open configuration file for reading */
static FILE *config_open(const unsigned char *filename) {
  assert(filename);
  if (strspn(filename,CONFIG_LEGAL_FILENAME_CHARACTERS) != strlen(filename)) {
    /* filename has illegal letters */
    return NULL;
  }
  return fopen(filename, "r");
}

/* close configuration file */
static int config_close(FILE *f) {
  assert(f);
  return fclose(f);
}

static struct config_line *config_get_commandlines(int argc, char **argv) {
  struct config_line *new;
  struct config_line *front = NULL;
  char *s;
  int i = 1;

  while(i < argc-1) { 
    if(!strcmp(argv[i],"-f")) {
//      log(LOG_DEBUG,"Commandline: skipping over -f.");
      i+=2; /* this is the config file option. ignore it. */
      continue;
    }

    new = tor_malloc(sizeof(struct config_line));
    s = argv[i];
    while(*s == '-')
      s++;
    new->key = tor_strdup(s);
    new->value = tor_strdup(argv[i+1]);

    log(LOG_DEBUG,"Commandline: parsed keyword '%s', value '%s'",
      new->key, new->value);
    new->next = front;
    front = new;
    i += 2;
  }
  return front;
}

/* parse the config file and strdup into key/value strings. Return list,
 * or NULL if parsing the file failed.
 * Warn and ignore mangled lines. */
static struct config_line *config_get_lines(FILE *f) {
  struct config_line *new;
  struct config_line *front = NULL;
  char line[CONFIG_LINE_MAXLEN];
  int result;
  char *key, *value;

  while( (result=parse_line_from_file(line,sizeof(line),f,&key,&value)) > 0) {
    new = tor_malloc(sizeof(struct config_line));
    new->key = tor_strdup(key);
    new->value = tor_strdup(value);

    new->next = front;
    front = new;
  }
  if(result < 0)
    return NULL;
  return front;
}

static void config_free_lines(struct config_line *front) {
  struct config_line *tmp;

  while(front) {
    tmp = front;
    front = tmp->next;

    free(tmp->key);
    free(tmp->value);
    free(tmp);
  }
}

static int config_compare(struct config_line *c, char *key, int type, void *arg) {
  int i;

  if(strncasecmp(c->key,key,strlen(c->key)))
    return 0;

  /* it's a match. cast and assign. */
  log_fn(LOG_DEBUG,"Recognized keyword '%s' as %s, using value '%s'.",c->key,key,c->value);

  switch(type) {
    case CONFIG_TYPE_INT:   
      *(int *)arg = atoi(c->value);
      break;
    case CONFIG_TYPE_BOOL:
      i = atoi(c->value);
      if (i != 0 && i != 1) {
        log(LOG_WARN, "Boolean keyword '%s' expects 0 or 1", c->key);
        return 0;
      }
      *(int *)arg = i;
      break;
    case CONFIG_TYPE_STRING:
      *(char **)arg = tor_strdup(c->value);
      break;
    case CONFIG_TYPE_DOUBLE:
      *(double *)arg = atof(c->value);
      break;
  }
  return 1;
}

static void config_assign(or_options_t *options, struct config_line *list) {

  /* iterate through list. for each item convert as appropriate and assign to 'options'. */

  while(list) {
    if(

    /* order matters here! abbreviated arguments use the first match. */

    /* string options */
    config_compare(list, "LogLevel",       CONFIG_TYPE_STRING, &options->LogLevel) ||
    config_compare(list, "LogFile",        CONFIG_TYPE_STRING, &options->LogFile) ||
    config_compare(list, "DebugLogFile",   CONFIG_TYPE_STRING, &options->DebugLogFile) ||
    config_compare(list, "DataDirectory",  CONFIG_TYPE_STRING, &options->DataDirectory) ||
    config_compare(list, "RouterFile",     CONFIG_TYPE_STRING, &options->RouterFile) ||
    config_compare(list, "PidFile",        CONFIG_TYPE_STRING, &options->PidFile) ||
    config_compare(list, "Nickname",       CONFIG_TYPE_STRING, &options->Nickname) ||
    config_compare(list, "Address",        CONFIG_TYPE_STRING, &options->Address) ||
    config_compare(list, "ExitPolicy",     CONFIG_TYPE_STRING, &options->ExitPolicy) ||

    /* int options */
    config_compare(list, "MaxConn",         CONFIG_TYPE_INT, &options->MaxConn) ||
    config_compare(list, "APPort",          CONFIG_TYPE_INT, &options->APPort) ||
    config_compare(list, "ORPort",          CONFIG_TYPE_INT, &options->ORPort) ||
    config_compare(list, "DirPort",         CONFIG_TYPE_INT, &options->DirPort) ||
    config_compare(list, "DirFetchPostPeriod",CONFIG_TYPE_INT, &options->DirFetchPostPeriod) ||
    config_compare(list, "KeepalivePeriod", CONFIG_TYPE_INT, &options->KeepalivePeriod) ||
    config_compare(list, "MaxOnionsPending",CONFIG_TYPE_INT, &options->MaxOnionsPending) ||
    config_compare(list, "NewCircuitPeriod",CONFIG_TYPE_INT, &options->NewCircuitPeriod) ||
    config_compare(list, "TotalBandwidth",  CONFIG_TYPE_INT, &options->TotalBandwidth) ||
    config_compare(list, "NumCpus",         CONFIG_TYPE_INT, &options->NumCpus) ||

    config_compare(list, "OnionRouter",     CONFIG_TYPE_BOOL, &options->OnionRouter) ||
    config_compare(list, "TrafficShaping",  CONFIG_TYPE_BOOL, &options->TrafficShaping) ||
    config_compare(list, "LinkPadding",     CONFIG_TYPE_BOOL, &options->LinkPadding) ||
    config_compare(list, "IgnoreVersion",   CONFIG_TYPE_BOOL, &options->IgnoreVersion) ||
    config_compare(list, "RunAsDaemon",     CONFIG_TYPE_BOOL, &options->RunAsDaemon) ||

    /* float options */
    config_compare(list, "CoinWeight",     CONFIG_TYPE_DOUBLE, &options->CoinWeight)

    ) {
      /* then we're ok. it matched something. */
    } else {
      log_fn(LOG_WARN,"Ignoring unknown keyword '%s'.",list->key);
    }

    list = list->next;
  }  
}

/* return 0 if success, <0 if failure. */
int getconfig(int argc, char **argv, or_options_t *options) {
  struct config_line *cl;
  FILE *cf;
  char *fname;
  int i;
  int result = 0;

/* give reasonable values for each option. Defaults to zero. */
  memset(options,0,sizeof(or_options_t));
  options->LogLevel = "info";
  options->ExitPolicy = "reject 127.0.0.1:*,reject 18.244.0.188:25,accept *:*";
  options->loglevel = LOG_INFO;
  options->PidFile = "tor.pid";
  options->DataDirectory = NULL;
  options->CoinWeight = 0.1;
  options->MaxConn = 900;
  options->DirFetchPostPeriod = 600;
  options->KeepalivePeriod = 300;
  options->MaxOnionsPending = 10;
  options->NewCircuitPeriod = 60; /* once a minute */
  options->TotalBandwidth = 800000; /* at most 800kB/s total sustained incoming */
  options->NumCpus = 1;

/* learn config file name, get config lines, assign them */
  i = 1;
  while(i < argc-1 && strcmp(argv[i],"-f")) {
    i++;
  }
  if(i < argc-1) { /* we found one */
    fname = argv[i+1];
  } else { /* didn't find one, try CONFDIR */
    fname = CONFDIR "/torrc";
  }
  log(LOG_DEBUG,"Opening config file '%s'",fname);

  cf = config_open(fname);
  if(!cf) {
    log(LOG_WARN, "Unable to open configuration file '%s'.",fname);
    return -1;
  }

  cl = config_get_lines(cf);
  if(!cl) return -1;
  config_assign(options,cl);
  config_free_lines(cl);
  config_close(cf);
 
/* go through command-line variables too */
  cl = config_get_commandlines(argc,argv);
  config_assign(options,cl);
  config_free_lines(cl);

/* Validate options */

  if(options->LogLevel) {
    if(!strcmp(options->LogLevel,"err"))
      options->loglevel = LOG_ERR;
    else if(!strcmp(options->LogLevel,"warn"))
      options->loglevel = LOG_WARN;
    else if(!strcmp(options->LogLevel,"info"))
      options->loglevel = LOG_INFO;
    else if(!strcmp(options->LogLevel,"debug"))
      options->loglevel = LOG_DEBUG;
    else {
      log(LOG_WARN,"LogLevel must be one of err|warn|info|debug.");
      result = -1;
    }
  }

  if(options->RouterFile == NULL) {
    log(LOG_WARN,"RouterFile option required, but not found.");
    result = -1;
  }

  if(options->ORPort < 0) {
    log(LOG_WARN,"ORPort option can't be negative.");
    result = -1;
  }

  if(options->OnionRouter && options->ORPort == 0) {
    log(LOG_WARN,"If OnionRouter is set, then ORPort must be positive.");
    result = -1;
  }

  if(options->OnionRouter && options->DataDirectory == NULL) {
    log(LOG_WARN,"DataDirectory option required for OnionRouter, but not found.");
    result = -1;
  }

  if(options->OnionRouter && options->Nickname == NULL) {
    log_fn(LOG_WARN,"Nickname required for OnionRouter, but not found.");
    result = -1;
  }

  if(options->APPort < 0) {
    log(LOG_WARN,"APPort option can't be negative.");
    result = -1;
  }

  if(options->DirPort < 0) {
    log(LOG_WARN,"DirPort option can't be negative.");
    result = -1;
  }

  if(options->APPort > 1 &&
     (options->CoinWeight < 0.0 || options->CoinWeight >= 1.0)) {
    log(LOG_WARN,"CoinWeight option must be >=0.0 and <1.0.");
    result = -1;
  }

  if(options->MaxConn < 1) {
    log(LOG_WARN,"MaxConn option must be a non-zero positive integer.");
    result = -1;
  }

  if(options->MaxConn >= MAXCONNECTIONS) {
    log(LOG_WARN,"MaxConn option must be less than %d.", MAXCONNECTIONS);
    result = -1;
  }

  if(options->DirFetchPostPeriod < 1) {
    log(LOG_WARN,"DirFetchPostPeriod option must be positive.");
    result = -1;
  }

  if(options->KeepalivePeriod < 1) {
    log(LOG_WARN,"KeepalivePeriod option must be positive.");
    result = -1;
  }

  return result;
}

/*
  Local Variables:
  mode:c
  indent-tabs-mode:nil
  c-basic-offset:2
  End:
*/
