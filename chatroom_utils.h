#ifndef CHATROOM_UTILS_H_
#define CHATROOM_UTILS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>


//Enum of different messages possible.
typedef enum
{
  CONNECT,
  DISCONNECT,
  GET_USERS,
  SET_USERNAME,
  PUBLIC_MESSAGE,
  PRIVATE_MESSAGE,
  TOO_FULL

} message_type;


//message structure
typedef struct
{
  message_type type;
  char username[21];
  char data[256];

} message;

//structure to hold client connection information
typedef struct connection_info
{
  int socket;
  struct sockaddr_in address;
  char username[20];
} connection_info;


// Removes the trailing newline character from a string.
void trim_newline(char *text);

// discard any remaining data on stdin buffer.
void clear_stdin_buffer();

#endif
