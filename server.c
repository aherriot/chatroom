#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>


#include "chatroom_utils.h"

#define MAX_CLIENTS 2

void send_public_message(connection_info connection[], int sender,
              char *username, char *message_text)
{
  message public_message;
  public_message.type = PUBLIC_MESSAGE;
  strncpy(public_message.username, username, 21);
  strncpy(public_message.data, message_text, 256);

  int i = 0;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    if(i != sender && connection[i].socket != 0)
    {
      if(send(connection[i].socket, &public_message, sizeof(public_message), 0) < 0)
      {
          puts("Send failed");
          exit(1);
      }
    }
  }
}

void send_too_full_message(int socket) {

  message too_full_message;
  too_full_message.type = TOO_FULL;

  if(send(socket, &too_full_message, sizeof(too_full_message), 0) < 0)
  {
      puts("Send failed");
      exit(1);
  }

  close(socket);
}

//close all the sockets before exiting
void stop_server(connection_info connection[])
{
  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    //send();
    close(connection[i].socket);
  }
  exit(0);
}


int main(int argc, char *argv[])
{

  puts("Starting server");
  int address_len;

  int main_socket;
  struct sockaddr_in address;

  connection_info connection[MAX_CLIENTS];
  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    connection[i].socket = 0;
  }


  fd_set file_descriptors;

  if (argc != 2)
  {

    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  if((main_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Failed to create socket");
    exit(1);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(atoi(argv[1]));

  if(bind(main_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("Binding failed");
    exit(1);
  }


  if(listen(main_socket, 3) < 0) {
    perror("Listen failed");
    exit(1);
  }

  //Accept and incoming connection
  printf("Waiting for incoming connections...\n");

  address_len = sizeof(address);
  while(true)
  {
    FD_ZERO(&file_descriptors);

    FD_SET(STDIN_FILENO, &file_descriptors);
    FD_SET(main_socket, &file_descriptors);

    int max_fd = main_socket;
    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
      if(connection[i].socket > 0)
      {
        FD_SET(connection[i].socket, &file_descriptors);
        if(connection[i].socket > max_fd)
        {
          max_fd = connection[i].socket;
        }
      }

    }

    if(select(max_fd+1, &file_descriptors, NULL, NULL, NULL) < 0)
    {
      perror("Select Failed");
    }

    if(FD_ISSET(STDIN_FILENO, &file_descriptors))
    {
      char input[255] = {};
      fgets(input, sizeof(input), stdin);
      trim_newline(input);

      if(input[0] == 'q') {
        stop_server(connection);
      }

    }

    if(FD_ISSET(main_socket, &file_descriptors))
    {
      int new_socket;
      new_socket = accept(main_socket, (struct sockaddr*)&address, (socklen_t*)&address_len);

      if (new_socket < 0)
      {
        perror("Accept Failed");
        exit(1);
      }

      for(i = 0; i < MAX_CLIENTS; i++)
      {
        if(connection[i].socket == 0) {
          connection[i].socket = new_socket;
          break;

        } else if (i == MAX_CLIENTS -1) // if we can accept no more clients
        {
          send_too_full_message(new_socket);
        }
      }
    }

    for(i = 0; i < MAX_CLIENTS; i++)
    {
      if(FD_ISSET(connection[i].socket, &file_descriptors))
      {
        int read_size;
        message received_message;

        if((read_size = recv(connection[i].socket, &received_message, sizeof(message), 0)) == 0)
        {
          printf("User %s disconnected.\n", connection[i].username);
          close(connection[i].socket);
          connection[i].socket = 0;

        } else {

          switch(received_message.type)
          {

          case SET_USERNAME:
            strcpy(connection[i].username, received_message.username);
            printf("User: %s\n", connection[i].username);
            break;

          case PUBLIC_MESSAGE:
            send_public_message(connection, i, received_message.username,received_message.data);
            break;

          case PRIVATE_MESSAGE:
            //TODO: Implement private message
            break;

          case GET_USERS:
            strcpy(connection[i].username, received_message.data);
            printf("User: %s\n", connection[i].username);
            break;

          default:
            fprintf(stderr, "Unknown message type received.\n");
            break;
          }

        }
      }
    }
  }

  return 0;
}
