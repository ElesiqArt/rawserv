#include <stdlib.h> // atoi() EXIT_SUCCESS
#include <string.h> // memcmp() memcpy() memset() strlen()

#include <arpa/inet.h> // htons()
#include <sys/socket.h> // socket() bind() listen() accept()
#include <signal.h> // sigaction() sigemptyset()
#include <unistd.h> // open() lseek() close() read() write()
#include <fcntl.h> // O_RDONLY
#include <sys/stat.h> // fstat()
#include <sys/sendfile.h> // sendfile()

#define CONTENT_LENGTH "\r\nContent-Length: 0\r\n\r\n"

const char bad_request[] = "HTTP/1.1 400 Bad Request" CONTENT_LENGTH;
const char not_found[] = "HTTP/1.1 404 Not Found" CONTENT_LENGTH;
const char method_not_allowed[] = "HTTP/1.1 405 Method Not Allowed" CONTENT_LENGTH;
const char uri_too_long[] = "HTTP/1.1 414 URI Too Long" CONTENT_LENGTH;
const char internal_server_error[] = "HTTP/1.1 500 Internal Server Error" CONTENT_LENGTH;

#undef CONTENT_LENGTH

#define DEFAULT_MAX_PATH_LENGTH 256
#define MAX_METHOD_LENGTH 4 // for 'GET '

#define QUEUE_LENGTH 32

#define EXIT_USAGE 1
#define EXIT_SIGNAL 2
#define EXIT_SOCKET 3

static int received_signal = 0;

void sig_handler(int signo)
{
  received_signal = signo;
}

void write_(int sockfd, const char * buffer, size_t length)
{
  size_t sent = 0;

  while(sent < length)
    {
      ssize_t sending = write(sockfd, buffer + sent, length - sent);

      if(sending == -1) return;

      sent += sending;
    }
}

int main(int argc, char ** argv)
{
  if(argc < 3)
    {
      return EXIT_USAGE;
    }

  int port = atoi(argv[1]);
  char * root = argv[2];
  size_t max_path_length = 4 <= argc ? atoi(argv[3]) : DEFAULT_MAX_PATH_LENGTH;

  struct sockaddr_in address;
  {
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;
  }

  char path[strlen(root) + max_path_length + 1];
  memcpy(path, root, strlen(root));
  memset(path + strlen(root), 0, max_path_length + 1);

  {
    struct sigaction sig_config;

    sig_config.sa_handler = sig_handler;
    sigemptyset(&sig_config.sa_mask);
    sig_config.sa_flags = 0;

    if(sigaction(SIGINT, &sig_config, NULL) == -1)
    {
      return EXIT_SIGNAL;
    }
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  {
    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
      {
	close(server_fd);
	return EXIT_SOCKET;
      }

    if(listen(server_fd, QUEUE_LENGTH) == -1)
      {
	close(server_fd);
	return EXIT_SOCKET;
      }
  }

  int addrlen = sizeof(struct sockaddr_in);
  struct sockaddr_in client_address;
  char buffer[MAX_METHOD_LENGTH + max_path_length];

  while(received_signal == 0)
    {
      int client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&addrlen);

      if(client_fd == -1)
	{
	  break;
	}

      ssize_t length = read(client_fd, buffer, MAX_METHOD_LENGTH + max_path_length);

      if(length == -1)
	{
	  write_(client_fd, internal_server_error, strlen(internal_server_error));
	  goto close_client;
	}

      if(length <= MAX_METHOD_LENGTH)
	{
	  write_(client_fd, bad_request, strlen(bad_request));
	  goto close_client;
	}

      if(memcmp(buffer, "GET", 3) != 0)
	{
	  write_(client_fd, method_not_allowed, strlen(method_not_allowed));
	  goto close_client;
	}

      if(buffer[3] != ' ')
	{
	  write_(client_fd, bad_request, strlen(bad_request));
	  goto close_client;
	}

      char * begin = buffer + MAX_METHOD_LENGTH;
      char * end = buffer + MAX_METHOD_LENGTH;

      while(end < buffer + length - MAX_METHOD_LENGTH && *end != ' ') ++end;

      if(max_path_length < (size_t)(end - begin))
	{
	  write_(client_fd, uri_too_long, strlen(uri_too_long));
	  goto close_client;
	}

      memcpy(path + strlen(root), begin, end - begin);

      int file = -1;

      if((file = open(path, O_RDONLY)) == -1)
	{
	  write_(client_fd, not_found, strlen(not_found));
	  goto close_client;
	}

      struct stat sfile;

      if(fstat(file, &sfile) == -1) goto internal_server_error;

      if(!S_ISREG(sfile.st_mode))
	{
	  write_(client_fd, not_found, strlen(not_found));
	  goto close_file;
	}

      off_t size = -1;

      if((size = lseek(file, 0, SEEK_END)) == -1) goto internal_server_error;
      if(lseek(file, 0, SEEK_SET) == -1) goto internal_server_error;

      off_t sent = 0;

      while(sent < size)
	{
	  ssize_t send = sendfile(client_fd, file, &sent, size - sent);

	  if(send == -1 && sent == 0) goto internal_server_error;
	  if(send == -1) goto close_file;
	}

      goto close_file;

    internal_server_error:
      write_(client_fd, internal_server_error, strlen(internal_server_error));

    close_file:
      close(file);

    close_client:
      close(client_fd);
      memset(path + strlen(root), 0, max_path_length);
    }

  close(server_fd);

  return EXIT_SUCCESS;
}
