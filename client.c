#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


#include <sys/socket.h>
#include <arpa/inet.h>

#include "chatroom_utils.h"

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
      int c;
      while((c = getchar()) != '\n' && c != EOF)
        /* discard */ ;

      puts("Username must be 20 characters or less.");
      fflush(stdout);
    } else {
      break;
    }
  }
}

void set_username(int sock, char *username)
{

  message username_message;
  username_message.type = SET_USERNAME;
  strncpy(username_message.username, username, 20);

  if(send(sock, (void*)&username_message, sizeof(username_message), 0) < 0)
  {
    puts("Send failed");
    exit(1);
  }
}

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char server_reply[2000];
    fd_set file_descriptors;
    char username[22];

    char stdin_buf[1024];
    setvbuf(stdin, stdin_buf, _IOFBF, 1024);

    if (argc != 3) {

      char errorStr[100];
      printf(errorStr,"Usage: %s <IP> <port>\n", argv[0]);
    }

    get_username(username);

    //Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP)) < 0)
    {
        printf("Could not create socket\n");
    }
    puts("Socket created\n");

    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons( atoi(argv[2]) );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("Connect failed.");
        return 1;
    }

    puts("Connected\n");
    set_username(sock, username);

    //keep communicating with server
    while(true)
    {
        FD_ZERO(&file_descriptors);
        FD_SET(STDIN_FILENO, &file_descriptors);
        FD_SET(sock, &file_descriptors);

        if(select(sock+1, &file_descriptors, NULL, NULL, NULL) < 0)
        {
          perror("Select failed.");
        }

        if(FD_ISSET(STDIN_FILENO, &file_descriptors))
        {
          message public_message;
          public_message.type = PUBLIC_MESSAGE;
          strncpy(public_message.username, username, 21);

          fgets(public_message.data, 256, stdin);
          trim_newline(public_message.data);

          //Send some data
          if(send(sock, &public_message, sizeof(message), 0) < 0)
          {
              puts("Send failed");
              exit(1);
          }

          //printf("me: ");
        }

        if(FD_ISSET(sock, &file_descriptors))
        {

          message received_message;


          //Receive a reply from the server
          ssize_t recv_val = recv(sock, &received_message, sizeof(message), 0);
          if(recv_val < 0)
          {
              puts("recv failed");
              break;

          }
          else if(recv_val == 0)
          {
            close(sock);
            puts("Server disconnected.");
            break;
          }

          switch(received_message.type)
          {

          case SET_USERNAME:
            break;

          case PUBLIC_MESSAGE:
            printf("%s: %s\n", received_message.username, received_message.data);
            break;

          default:
            printf("Unknown message type received.\n");
            break;
          }


          //puts("Server reply :");
          puts(server_reply);


        }
    }

    close(sock);
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
