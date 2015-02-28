#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h>
#include <stdlib.h>

#define RCVBUFSIZE 2048   /* Size of receive buffer */
#define FILEBUFSIZE 512 /* Size of file reader buffer */
#define MAXLINE 512 /* MAX SIZE OF A LINE */

char *getContentType(char *filename, char *type); /* Get the content type from the file type */
char *getFileType(char *filename); /* Get the file type */
char *getFirstLine(char *buffer); /* Parse first line */
void sendFile(int clntSocket, char *filename); /* Send file function */
void DieWithError(char *errorMessage);  /* Error handling function */

static char* ok =
  "HTTP/1.0 200 OK\n"
  "Connection: close\n"
  "Content-type: ";

static char* bad_request = 
  "HTTP/1.0 404 Not Found\n"
  "Content-type: text/html\n"
  "Connection: close\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "  <h1>Bad Request</h1>\n"
  "  <p>File not found.</p>\n"
  " </body>\n"
  "</html>\n";

static char* invalid_method =
  "HTTP/1.0 400 Bad Request\n"
  "Content-type: text/html\n"
  "Connection: close\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "   <h1>Bad Request</h1>\n"
  "   <p>Invalid method.</p>\n"
  " </body>\n"
  "</html>\n";

void HandleTCPClient(int clntSocket)
{
    char echoBuffer[RCVBUFSIZE];        /* Buffer for echo string */
    int recvMsgSize;                    /* Size of received message */

    /* Receive message from client */
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    /* Send received string and receive again until end of transmission */
    if (recvMsgSize > 0)      /* zero indicates end of transmission */
    {
        //printf("%s\n", echoBuffer);
	
	/* Parse first line of request */
	char *line = getFirstLine(echoBuffer);
	char method[16], version[16], URI[MAXLINE];
	sscanf(line, "%s %s %s", method, URI, version);
	free(line);

	/* Handle if method isn't GET request */
	if (strcmp(method, "GET") != 0) {
	  send(clntSocket, invalid_method, strlen(invalid_method), 0);
	}

	char filename[MAXLINE+1]; filename[0] = '.'; filename[1] = '\0';
	strcat(filename, URI);

        /* Echo message back to client */
	sendFile(clntSocket, filename);

        /* See if there is more data to receive */
        //if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        //    DieWithError("recv() failed");
    }

    close(clntSocket);    /* Close client socket */
}

void sendFile(int clntSocket, char *filename) {
  FILE *fp = NULL;
  /* Get the filename. If none provided, get index.html */
  printf("%s\n", filename);
  if (strcmp(filename, "./") == 0) 
    fp = fopen("index.html", "r");
  else 
    fp = fopen(filename, "r");

  int size = 0;
  if (!fp) {
    size = send(clntSocket, bad_request, strlen(bad_request), 0);
    if (size < 0)
      DieWithError("send() failed");
    return;
  }
  int read_size = 0;
  char file_buffer[FILEBUFSIZE];
  char *type = getFileType(filename);
  
  char *content = getContentType(filename, type);
  int h_size = strlen(ok) + strlen(content);
  char *header = malloc( h_size * sizeof(char) ); 
  strcpy(header, ok);
  strcat(header, content);

  size = send(clntSocket, header, strlen(header), 0);

  do {
    read_size = fread(file_buffer, sizeof(char), FILEBUFSIZE, fp);
    if (read_size > 0)
      size = send(clntSocket, file_buffer, read_size, 0);

  } while (read_size > 0);

  fclose(fp);

  free(header);
  free(content);
  free(type);
}

char *getFirstLine(char *buffer) {
  int i;
  for(i=0; i < strlen(buffer); i++) {
    if (buffer[i] == '\n') {
      break;
    }
  }

  char* substr = malloc( i * sizeof(char) );
  strncpy(substr, buffer, i);
  return substr;
}

char *getFileType(char *filename) {
  int len = strlen(filename);
  int i = len-1;
  for(; i >= 0; i--) {
    if (filename[i] == '.')
      break;
  }

  char* type = malloc( (len - i) * sizeof(char) );
  strncpy(type, filename+i, (len-i));
  return type;
}

char *getContentType(char *filename, char *type) {
  char *content = malloc( 32 * sizeof(char) );
  if (!strcmp(type, filename) || !strcmp(type, ".html")) {
    strcpy(content,"text/html\n\n");
  } else if (!strcmp(type, ".txt")) {
    strcpy(content,"text/plain\n\n");
  } else if (!strcmp(type, ".jpg")) {
    strcpy(content,"image/jpeg\n\n");
  } else if (!strcmp(type, ".png")) {
    strcpy(content,"image/png\n\n");
  } else if (!strcmp(type, ".gif")) {
    strcpy(content,"image/gif\n\n");
  } else if (!strcmp(type, ".pdf")) {
    strcpy(content,"application/pdf\n\n");
  } else {
    strcpy(content,"text/html\n\n");
  }

  return content;
}
