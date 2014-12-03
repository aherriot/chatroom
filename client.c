#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


#include <sys/socket.h>
#include <arpa/inet.h>

#include "chatroom_utils.h"

// get a username from the user.
void get_username(char *username)
{
  while(true)
  {
    printf("Enter a username: ");
    fflush(stdout);
    memset(username, 0, 1000);
    fgets(username, 22, stdin);
    trim_newline(username);

    if(strlen(username) > 20)
    {
      clear_stdin_buffer();

      puts("Username must be 20 characters or less.");
      fflush(stdout);

    } else {
      break;
    }
  }
}

//send local username to the server.
void set_username(connection_info *connection)
{

  message username_message;
  username_message.type = SET_USERNAME;
  strncpy(username_message.username, connection->username, 20);

  if(send(connection->socket, (void*)&username_message, sizeof(username_message), 0) < 0)
  {
    perror("Send failed");
    exit(1);
  }
}

//initialize connection to the server.
void connect_to_server(connection_info *connection, char *address, char *port)
{

  get_username(connection->username);

  //Create socket
  if ((connection->socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP)) < 0)
  {
      perror("Could not create socket");
  }
  puts("Socket created\n");

  connection->address.sin_addr.s_addr = inet_addr(address);
  connection->address.sin_family = AF_INET;
  connection->address.sin_port = htons(atoi(port));

  //Connect to remote server
  if (connect(connection->socket , (struct sockaddr *)&connection->address , sizeof(connection->address)) < 0)
  {
      perror("Connect failed.");
      exit(1);
  }

  set_username(connection);

  puts("Connected\n");
}


void handle_user_input(connection_info *connection)
{
  message public_message;
  public_message.type = PUBLIC_MESSAGE;
  strncpy(public_message.username, connection->username, 21);

  fgets(public_message.data, 256, stdin);
  //clear_stdin_buffer();

  //if there is no input, don't send it.
  if(strlen(public_message.data) == 0) {
    return;
  }

  trim_newline(public_message.data);

  //Send some data
  if(send(connection->socket, &public_message, sizeof(message), 0) < 0)
  {
      perror("Send failed");
      exit(1);
  }
}

void handle_server_message(connection_info *connection)
{
  message received_message;

  //Receive a reply from the server
  ssize_t recv_val = recv(connection->socket, &received_message, sizeof(message), 0);
  if(recv_val < 0)
  {
      perror("recv failed");
      exit(1);

  }
  else if(recv_val == 0)
  {
    close(connection->socket);
    puts("Server disconnected.");
    exit(1);
  }

  switch(received_message.type)
  {

  case SET_USERNAME:
    break;

  case PUBLIC_MESSAGE:
    printf("%s: %s", received_message.username, received_message.data);
    break;

  case TOO_FULL:
    printf("Server chatroom is too full to accept new clients.\n");
    exit(0);

  default:
    fprintf(stderr, "Unknown message type received.\n");
    break;
  }
}

int main(int argc, char *argv[])
{
  connection_info connection;
  fd_set file_descriptors;

  if (argc != 3) {
    fprintf(stderr,"Usage: %s <IP> <port>\n", argv[0]);
    exit(1);
  }

  connect_to_server(&connection, argv[1], argv[2]);

  //keep communicating with server
  while(true)
  {
    FD_ZERO(&file_descriptors);
    FD_SET(STDIN_FILENO, &file_descriptors);
    FD_SET(connection.socket, &file_descriptors);

    if(select(connection.socket+1, &file_descriptors, NULL, NULL, NULL) < 0)
    {
      perror("Select failed.");
      exit(1);
    }

    if(FD_ISSET(STDIN_FILENO, &file_descriptors))
    {
      handle_user_input(&connection);
    }

    if(FD_ISSET(connection.socket, &file_descriptors))
    {
      handle_server_message(&connection);
    }
  }

  close(connection.socket);
  return 0;
}


//something to look into for better stdin
//http://stackoverflow.com/questions/421860/capture-characters-from-standard-input-without-waiting-for-enter-to-be-pressed
// #include <unistd.h>
// #include <termios.h>
//
// char getch() {
//         char buf = 0;
//         struct termios old = {0};
//         if (tcgetattr(0, &old) < 0)
//                 perror("tcsetattr()");
//         old.c_lflag &= ~ICANON;
//         old.c_lflag &= ~ECHO;
//         old.c_cc[VMIN] = 1;
//         old.c_cc[VTIME] = 0;
//         if (tcsetattr(0, TCSANOW, &old) < 0)
//                 perror("tcsetattr ICANON");
//         if (read(0, &buf, 1) < 0)
//                 perror ("read()");
//         old.c_lflag |= ICANON;
//         old.c_lflag |= ECHO;
//         if (tcsetattr(0, TCSADRAIN, &old) < 0)
//                 perror ("tcsetattr ~ICANON");
//         return (buf);
// }
