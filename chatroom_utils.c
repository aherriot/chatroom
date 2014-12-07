/*
*
* Chatroom - a simple linux commandline client/server C program for group chat.
* Author: Andrew Herriot
* License: Public Domain
*
*/


#include "chatroom_utils.h"

#include <string.h>

void trim_newline(char *text)
{
  int len = strlen(text) - 1;
  if (text[len] == '\n')
{
      text[len] = '\0';
  }
}

void clear_stdin_buffer()
{
  int c;
  while((c = getchar()) != '\n' && c != EOF)
    /* discard content*/ ;
}
