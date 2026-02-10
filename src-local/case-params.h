/**
# Case Parameter Parsing Helper

Simple parser for `key = value` parameter files used by simulation
cases.
*/

#ifndef CASE_PARAMS_H
#define CASE_PARAMS_H

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARAM_KIND_INT 0
#define PARAM_KIND_DOUBLE 1

typedef struct {
  const char * key;
  void * target;
  int kind;
  int required;
  int seen;
} paramEntry;

static char * trimParamWhitespace (char * s) {
  while (*s && isspace((unsigned char) *s))
    s++;

  if (*s == '\0')
    return s;

  char * end = s + strlen(s) - 1;
  while (end >= s && isspace((unsigned char) *end))
    *end-- = '\0';

  return s;
}

static int parseCaseParams (const char * filename, paramEntry * entries,
                            size_t n_entries) {
  FILE * fp = fopen(filename, "r");
  if (!fp) {
    fprintf(ferr, "Could not open params file: %s\n", filename);
    return 0;
  }

  for (size_t i = 0; i < n_entries; i++)
    entries[i].seen = 0;

  char line[512];
  int line_no = 0;

  while (fgets(line, sizeof(line), fp)) {
    line_no++;

    char * text = trimParamWhitespace(line);
    if (*text == '\0' || *text == '#' || *text == ';')
      continue;

    char * hash = strchr(text, '#');
    if (hash)
      *hash = '\0';

    char * slash = strstr(text, "//");
    if (slash)
      *slash = '\0';

    text = trimParamWhitespace(text);
    if (*text == '\0')
      continue;

    char * eq = strchr(text, '=');
    if (!eq) {
      fprintf(ferr, "Malformed line %d in %s\n", line_no, filename);
      fclose(fp);
      return 0;
    }

    *eq = '\0';
    char * key = trimParamWhitespace(text);
    char * value = trimParamWhitespace(eq + 1);

    if (*key == '\0' || *value == '\0') {
      fprintf(ferr, "Invalid key/value at line %d in %s\n", line_no,
              filename);
      fclose(fp);
      return 0;
    }

    int matched = 0;
    for (size_t i = 0; i < n_entries; i++) {
      if (strcmp(key, entries[i].key))
        continue;

      matched = 1;
      entries[i].seen = 1;
      errno = 0;

      if (entries[i].kind == PARAM_KIND_INT) {
        char * end = NULL;
        long parsed = strtol(value, &end, 10);
        if (errno || !end || *end != '\0') {
          fprintf(ferr,
                  "Invalid integer for %s at line %d in %s\n",
                  key, line_no, filename);
          fclose(fp);
          return 0;
        }
        if (parsed < INT_MIN || parsed > INT_MAX) {
          fprintf(ferr,
                  "Integer out of range for %s at line %d"
                  " in %s\n",
                  key, line_no, filename);
          fclose(fp);
          return 0;
        }
        *((int *) entries[i].target) = (int) parsed;
      }
      else {
        char * end = NULL;
        double parsed = strtod(value, &end);
        if (errno || !end || *end != '\0') {
          fprintf(ferr, "Invalid number for %s at line %d in %s\n", key,
                  line_no, filename);
          fclose(fp);
          return 0;
        }
        *((double *) entries[i].target) = parsed;
      }
      break;
    }

    if (!matched)
      fprintf(ferr, "Ignoring unknown key '%s' in %s\n", key, filename);
  }

  fclose(fp);

  for (size_t i = 0; i < n_entries; i++) {
    if (entries[i].required && !entries[i].seen) {
      fprintf(ferr, "Missing required key '%s' in %s\n", entries[i].key,
              filename);
      return 0;
    }
  }

  return 1;
}

#endif
