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

void initialize_server(connection_info *server_info, int port)
{
  if((server_info->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Failed to create socket");
    exit(1);
  }

  server_info->address.sin_family = AF_INET;
  server_info->address.sin_addr.s_addr = INADDR_ANY;
  server_info->address.sin_port = htons(port);

  if(bind(server_info->socket, (struct sockaddr *)&server_info->address, sizeof(server_info->address)) < 0)
  {
    perror("Binding failed");
    exit(1);
  }


  if(listen(server_info->socket, 3) < 0) {
    perror("Listen failed");
    exit(1);
  }

  //Accept and incoming connection
  printf("Waiting for incoming connections...\n");
}


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


void handle_client_message(connection_info clients[], int i)
{
  int read_size;
  message received_message;

  if((read_size = recv(clients[i].socket, &received_message, sizeof(message), 0)) == 0)
  {
    printf("User %s disconnected.\n", clients[i].username);
    close(clients[i].socket);
    clients[i].socket = 0;

  } else {

    switch(received_message.type)
    {
      case SET_USERNAME:
        strcpy(clients[i].username, received_message.username);
        printf("User: %s\n", clients[i].username);
      break;

      case PUBLIC_MESSAGE:
        send_public_message(clients, i, received_message.username, received_message.data);
      break;

      case PRIVATE_MESSAGE:
        //TODO: Implement private message
      break;

      case GET_USERS:
        strcpy(clients[i].username, received_message.data);
        printf("User: %s\n", clients[i].username);
      break;

      default:
        fprintf(stderr, "Unknown message type received.\n");
      break;
    }
  }
}

int construct_fd_set(fd_set *set, connection_info *server_info,
                      connection_info clients[])
{
  FD_ZERO(set);
  FD_SET(STDIN_FILENO, set);
  FD_SET(server_info->socket, set);

  int max_fd = server_info->socket;
  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket > 0)
    {
      FD_SET(clients[i].socket, set);
      if(clients[i].socket > max_fd)
      {
        max_fd = clients[i].socket;
      }
    }
  }

  return max_fd;
}

void handle_new_connection(connection_info *server_info, connection_info clients[])
{
  int new_socket;
  int address_len;
  new_socket = accept(server_info->socket, (struct sockaddr*)&server_info->address, (socklen_t*)&address_len);

  if (new_socket < 0)
  {
    perror("Accept Failed");
    exit(1);
  }

  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket == 0) {
      clients[i].socket = new_socket;
      break;

    } else if (i == MAX_CLIENTS -1) // if we can accept no more clients
    {
      send_too_full_message(new_socket);
    }
  }
}

void handle_user_input(connection_info clients[])
{
  char input[255] = {};
  fgets(input, sizeof(input), stdin);
  trim_newline(input);

  if(input[0] == 'q') {
    stop_server(clients);
  }
}

int main(int argc, char *argv[])
{
  puts("Starting server");

  fd_set file_descriptors;

  connection_info server_info;
  connection_info clients[MAX_CLIENTS];

  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    clients[i].socket = 0;
  }

  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  initialize_server(&server_info, atoi(argv[1]));

  while(true)
  {
    int max_fd = construct_fd_set(&file_descriptors, &server_info, clients);

    if(select(max_fd+1, &file_descriptors, NULL, NULL, NULL) < 0)
    {
      perror("Select Failed");
    }

    if(FD_ISSET(STDIN_FILENO, &file_descriptors))
    {
      handle_user_input(clients);
    }

    if(FD_ISSET(server_info.socket, &file_descriptors))
    {
      handle_new_connection(&server_info, clients);
    }

    for(i = 0; i < MAX_CLIENTS; i++)
    {
      if(FD_ISSET(clients[i].socket, &file_descriptors))
      {
        handle_client_message(clients, i);
      }
    }
  }

  return 0;
}
