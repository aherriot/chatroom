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
      // clear_stdin_buffer();

      puts("Username must be 20 characters or less.");

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


void stop_client(connection_info *connection)
{
  close(connection->socket);
  exit(0);
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

  connection->address.sin_addr.s_addr = inet_addr(address);
  connection->address.sin_family = AF_INET;
  connection->address.sin_port = htons(atoi(port));

  //Connect to remote server
  if (connect(connection->socket, (struct sockaddr *)&connection->address , sizeof(connection->address)) < 0)
  {
      perror("Connect failed.");
      exit(1);
  }

  set_username(connection);

  puts("Connected to server.");
}


void handle_user_input(connection_info *connection)
{
  message msg;
  fgets(msg.data, 255, stdin);
  trim_newline(msg.data);

  if(strcmp(msg.data, "/q") == 0 || strcmp(msg.data, "/quit") == 0)
  {
    stop_client(connection);
  }
  else if(strcmp(msg.data, "/l") == 0 || strcmp(msg.data, "/list") == 0)
  {

    msg.type = GET_USERS;

    if(send(connection->socket, &msg, sizeof(message), 0) < 0)
    {
        perror("Send failed");
        exit(1);
    }
  }
  else if(strcmp(msg.data, "/h") == 0 || strcmp(msg.data, "/help") == 0)
  {
    puts("/quit or /q: Exit the program.");
    puts("/help or /h: Displays help information.");
    puts("/list or /l: Displays list of users in chatroom.");
    puts("/m <username>: Send private message to given username.");
  }
  else if(strncmp(msg.data, "/m ", 2) == 0)
  {
    //TODO: private messaging.
    puts("Private messaging to be implemented.");
  }
  else //regular public message
  {
    message msg;
    msg.type = PUBLIC_MESSAGE;
    strncpy(msg.username, connection->username, 20);

    // clear_stdin_buffer();

    if(strlen(msg.data) == 0) {
        return;
    }

    //Send some data
    if(send(connection->socket, &msg, sizeof(message), 0) < 0)
    {
        perror("Send failed");
        exit(1);
    }
  }



}

void handle_server_message(connection_info *connection)
{
  message msg;

  //Receive a reply from the server
  ssize_t recv_val = recv(connection->socket, &msg, sizeof(message), 0);
  if(recv_val < 0)
  {
      perror("recv failed");
      exit(1);

  }
  else if(recv_val == 0)
  {
    close(connection->socket);
    puts("Server disconnected.");
    exit(0);
  }

  switch(msg.type)
  {

    case CONNECT:
      printf(KCYN "%s has connected." RESET "\n", msg.username);
    break;

    case DISCONNECT:
      printf(KCYN "%s has disconnected." RESET "\n" , msg.username);
    break;

    case GET_USERS:
      printf("users: %s\n", msg.data);
    break;

    case SET_USERNAME:
      //TODO: implement
    break;

    case PUBLIC_MESSAGE:
      printf(KGRN "%s" RESET ": %s\n", msg.username, msg.data);
    break;

    case PRIVATE_MESSAGE:
      printf(KGRN "From %s:" KMAG " %s\n" RESET, msg.username, msg.data);
    break;

    case TOO_FULL:
      printf(KRED "Server chatroom is too full to accept new clients.\n" RESET);
      exit(0);
    break;



    default:
      fprintf(stderr, KRED "Unknown message type received.\n" RESET);
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
    fflush(stdin);

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
