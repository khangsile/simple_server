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

/*
  Base header for a good request
 */
static char* ok =
  "HTTP/1.0 200 OK\n"
  "Connection: close\n"
  "Content-type: ";

/*
  HTML for a 404 Not Found
 */
static char* bad_request_begin = 
  "HTTP/1.0 404 Not Found\n"
  "Content-type: text/html\n"
  "Connection: close\n"
  "\n"
  "<html>\n"
  " <body>\n"
  "  <h1>Bad Request</h1>\n"
  "  <p>The file at ";

/*
  HTML for a 404 Not Found
 */
static char* bad_request_end = 
  " was not found</p>\n"
  " </body>\n"
  "</html>\n";

/*
  HTML if the method is not a GET request
 */
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

    if (recvMsgSize > 0)      /* zero indicates end of transmission */
    {
        //printf("%s\n", echoBuffer);
	
	/* Parse first line of request */
	char *line = getFirstLine(echoBuffer);
	char method[16], version[16], URI[MAXLINE];
	/* Break the first line of the request into the method, path, and http version */
	sscanf(line, "%s %s %s", method, URI, version);
	free(line);

	/* Handle if method isn't GET request */
	if (strcmp(method, "GET") != 0) {
	  send(clntSocket, invalid_method, strlen(invalid_method) * sizeof(char), 0);
	}

	/* Add . to the beginning of file path to indicate start from current directory */
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

/* Request method is good. Send the file to the client */
void sendFile(int clntSocket, char *filename) {
  FILE *fp = NULL;
  /* Get the filename. If none provided, get index.html */
  printf("%s\n", filename);
  if (strcmp(filename, "./") == 0) 
    fp = fopen("index.html", "r");
  else 
    fp = fopen(filename, "r");

  int size = 0;
  /* If the file pointer is null, then the file does not exist. Return a 404 */
  if (!fp) {
    int response_size = strlen(bad_request_begin) + strlen(bad_request_end) + strlen(filename);
    char *bad_request = malloc( response_size * sizeof(char) );
    strcpy(bad_request, bad_request_begin);
    strcat(bad_request, filename);
    strcat(bad_request, bad_request_end);
    size = send(clntSocket, bad_request, response_size * sizeof(char), 0);
    free(bad_request);
    if (size < 0)
      DieWithError("send() failed");
    return;
  }
  int read_size = 0;
  char file_buffer[FILEBUFSIZE];
  /* Get the file type based on the extension */
  char *type = getFileType(filename);
  
  /* Based on file type, get the content type */
  char *content = getContentType(filename, type);
  int h_size = strlen(ok) + strlen(content) + 1; /* Make space for null terminator */
  /* Create the full header for an ok request */
  char *header = malloc( h_size * sizeof(char) ); 
  strcpy(header, ok);
  strcat(header, content);

  /* Send the header */
  size = send(clntSocket, header, strlen(header), 0);
  printf("Header: \n%s", header);

  /* Read the entire file and send it back in chunks of FILEBUFSIZE */
  do {
    read_size = fread(file_buffer, sizeof(char), FILEBUFSIZE, fp);
    if (read_size > 0)
      size = send(clntSocket, file_buffer, read_size, 0);

  } while (read_size > 0);

  /* Cleanup */
  fclose(fp);

  free(header);
  free(content);
  free(type);
}

/* Gets the first line of the request header by finding the first occurence of a line break */
char *getFirstLine(char *buffer) {
  int i;
  for(i=0; i < strlen(buffer); i++) {
    if (buffer[i] == '\n') {
      break;
    }
  }

  char* substr = malloc( (i+1) * sizeof(char) );
  strncpy(substr, buffer, i);
  substr[i] = '\0';
  return substr;
}

/* Gets the file type by starting from the end of the filename and finding the first . */
char *getFileType(char *filename) {
  int len = strlen(filename);
  int i = len-1;
  for(; i >= 0; i--) {
    if (filename[i] == '.')
      break;
  }
  if (i < 0) i = 0;

  // Account for termination character
  char* type = malloc( (len - i + 1) * sizeof(char) );
  strncpy(type, filename+i, (len-i + 1));
  return type;
}

/* Gets the content type based on the file type. If the type cannot be determined 
   based on the filename, then the default value is text/html.
 */
char *getContentType(char *filename, char *type) {
  char *content = malloc( 32 * sizeof(char) );
  printf("Type: %s\n", type);
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
