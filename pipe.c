#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <libwebsockets.h>
#include <pthread.h>

#define DEFAULT_MAX_USERS   1024
#define DEFAULT_MAX_BUFFER  1024
#define DEFAULT_PORT        8000

static struct lws_protocols *protocols;
static struct lws_context *context;
static uint16_t port = DEFAULT_PORT;
static uint32_t max_users = DEFAULT_MAX_USERS;
static uint32_t max_buffer_size = DEFAULT_MAX_BUFFER;
static uint32_t use_ssl = 0;

static char *served_html_file;
static char *first_message;

static pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct lws **users;
static uint32_t num_users;

static void send_buffer(char *buffer, uint32_t len);

static int ws_server_callback(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user,
                       void *in,
                       size_t len) {
  switch (reason) {
    case LWS_CALLBACK_HTTP:
    {
      if (served_html_file) {
        lws_serve_http_file(wsi, served_html_file, "text/html", NULL, 0);
        fprintf(stderr, "HTTP Request: serving %s\n", served_html_file);
      } else {
        fprintf(stderr, "HTTP Request error: no file to serve (use the -f flag)\n");
      }
      break;
    }
    case LWS_CALLBACK_RECEIVE:
    {
      write(STDOUT_FILENO, in, len);
      break;
    }
    case LWS_CALLBACK_ESTABLISHED:
    {
      pthread_mutex_lock(&users_mutex);
      users[num_users++] = wsi;
      pthread_mutex_unlock(&users_mutex);
      fprintf(stderr, "Connect: %d users\n", num_users);
      break;
    }
    case LWS_CALLBACK_CLOSED:
    {
      pthread_mutex_lock(&users_mutex);
      // Find the user and remove them
      int i;
      for (i = 0; i < num_users; i++) {
        if (wsi == users[i]) {
          // If there are users beyond this, move them closer
          if (i + 1 < num_users) {
            memmove(&users[i], &users[i+1], (num_users - i - 1) * sizeof(struct lws *));
          }
          num_users--;
        }
      }
      pthread_mutex_unlock(&users_mutex);
      fprintf(stderr, "Disconnect: %d users\n", num_users);
      break;
    }
    default: break;
  }
  return 0;
}

static int ws_client_callback(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user,
                       void *in,
                       size_t len) {
  switch (reason) {
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      fprintf(stderr, "Error attempting to connect: %s\n", (char *)in);
      exit(0);
      break;
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      fprintf(stderr, "Successfully connected to server.\n");
      if (first_message) {
        send_buffer(first_message, strlen(first_message));
      }
      break;
    case LWS_CALLBACK_CLOSED:
      fprintf(stderr, "Connection closed.\n");
      exit(0);
      break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
      write(STDOUT_FILENO, in, len);
      break;
    default:
      break;
  }
  return 0;
}

static int initialize_ws_server(void) {
  struct lws_protocols _protocols[] = {
    {
      "default",
      ws_server_callback,
      0,
      max_buffer_size
    },
    { NULL, NULL, 0 }
  };
  protocols = malloc(sizeof(_protocols));
  memcpy(protocols, _protocols, sizeof(_protocols));

  struct lws_context_creation_info *info = calloc(1, sizeof(struct lws_context_creation_info));
  info->port = port;
  info->iface = NULL;
  info->protocols = protocols;
  info->extensions = NULL;
  context = lws_create_context(info);
  if (context ==  NULL) {
    fprintf(stderr, "Unable to create lws context.\n");
    goto error;
  }

  return 0;

error:
  free(protocols);
  free(info);
  return -1;
}

static int initialize_ws_client(char *address) {
  struct lws_protocols _protocols[] = {
    {
      "default",
      ws_client_callback,
      0,
      max_buffer_size
    },
    { NULL, NULL, 0 }
  };

  protocols = malloc(sizeof(_protocols));
  memcpy(protocols, _protocols, sizeof(_protocols));

  struct lws_context_creation_info *info = calloc(1, sizeof(struct lws_context_creation_info));
  info->port = CONTEXT_PORT_NO_LISTEN;
  info->iface = NULL;
  info->protocols = protocols;
  info->extensions = NULL;

  context = lws_create_context(info);
  if (context ==  NULL) {
    fprintf(stderr, "Unable to create lws context.\n");
    goto error;
  }

  // Parse out the address:port/path
  char hostname[100];
  char path[100];
  // Defaults
  port = 80;
  strcpy(path, "/");

  // Check for ws:// or wss://
  const char wss_prefix[] = "wss://";
  const char ws_prefix[] = "ws://";

  size_t wss_prefix_len = strlen(wss_prefix);
  size_t ws_prefix_len = strlen(ws_prefix);

  if (strncmp(wss_prefix, address, wss_prefix_len) == 0) {
    address = &(address[wss_prefix_len]);
    port = 443;
    use_ssl = 1;
  } else if (strncmp(ws_prefix, address, ws_prefix_len) == 0) {
    address = &(address[ws_prefix_len]);
  }

  if (sscanf(address, "%99[^:]:%99hu/%99[^\n]", hostname, &port, &path[1]) < 2) {
    sscanf(address, "%99[^/]/%99[^\n]", hostname, &path[1]);
  }

  char address_w_port[1024];
  sprintf(address_w_port, "%s:%d", hostname, port);

  if (port != 80) {
    fprintf(stderr, "Connecting to %s on %s\n", address_w_port, path);
  } else {
    fprintf(stderr, "Connecting to %s on %s\n", hostname, path);
  }

  struct lws *client_wsi = lws_client_connect(context, hostname, port, use_ssl, path, address_w_port, NULL, NULL, -1);
  if (client_wsi == NULL) {
    fprintf(stderr, "Unable to connect to client.\n");
    goto error;
  }
  users = calloc(1, sizeof(struct lws *));
  users[0] = client_wsi;
  num_users = 1;

  return 0;

error:
  free(protocols);
  free(info);
  return -1;
}

static void *ws_thread_loop(void *args) {
  while(1) {
    lws_service(context, 50);
  }
  return NULL;
}

static void send_buffer(char *buffer, uint32_t len) {
  fprintf(stderr, "Sending %u byte(s) to %d user(s)\n", (unsigned int)len, num_users);
  char out_buffer[LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING];
  memset(&out_buffer[LWS_SEND_BUFFER_PRE_PADDING], 0, len);
  memcpy(&out_buffer[LWS_SEND_BUFFER_PRE_PADDING], buffer, len);
  int i;
  pthread_mutex_lock(&users_mutex);
  for (i = 0; i < num_users; i++) {
    lws_write(users[i], (unsigned char *)&out_buffer[LWS_SEND_BUFFER_PRE_PADDING], len, LWS_WRITE_BINARY);
  }
  pthread_mutex_unlock(&users_mutex);
}

static void turn_off_errors(void) {
  // Remove errors
  int devnull_fd = open("/dev/null", O_WRONLY);
  dup2(devnull_fd, STDERR_FILENO);
}

static void read_input(void) {
  char buffer[max_buffer_size];
  while(1) {
    int s = read(STDIN_FILENO, buffer, sizeof buffer);
    if(s < 0) {
      if(errno == EINTR) continue;
      perror("read");
      break;
    }
    if(s < 1) break;
    send_buffer(buffer, s);
  }
}

static void print_usage() {
  printf("webpipe [-p port] [-f file.html] [-d] [server]\n");
}

int main(int argc, char *argv[]) {
  int debug_flag = 0;
  int c;
  while((c = getopt(argc, argv, "p:U:B:f:i:sdh?")) != -1) {
    switch (c) {
      case 'p':
        port = atoi(optarg);
        break;
      case 'U':
        max_users = atoi(optarg);
        break;
      case 'B':
        max_buffer_size = atoi(optarg);
        break;
      case 'f':
        served_html_file = optarg;
        break;
      case 'i':
        first_message = optarg;
        break;
      case 'd':
        debug_flag = 1;
        break;
      case 's':
        use_ssl = 2;
        break;
      case 'h':
      case '?':
      default:
        print_usage();
        break;
    }
  }

  if (!debug_flag) {
    turn_off_errors();
  }

  int err = 0;

  if (argv[optind] != NULL) {
    fprintf(stderr, "Starting a client connection to %s.\n", argv[optind]);
    // Start a client
    err = initialize_ws_client(argv[optind]);
  } else {
    fprintf(stderr, "Starting a server.\n");
    users = calloc(max_users, sizeof(struct lws *));
    // Start a server
    err = initialize_ws_server();
  }
sleep(1);
  // Start websocket connection on a different thread.
  pthread_t ws_thread;
  if (!err) {
    pthread_create(&ws_thread, NULL, ws_thread_loop, NULL);
  }

  // Read input.
  read_input();

  return 0;
}

