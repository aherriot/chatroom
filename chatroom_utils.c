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
