#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <libwebsockets.h>
#include <pthread.h>

#define MAX_USERS 1024
#define MAX_BUFFER 1024

static struct lws_protocols *protocols;
static struct lws_context *context;
static uint16_t port = 8000;

static char *served_html_file;

static pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct lws *users[MAX_USERS];
static uint32_t num_users;

static int ws_callback(struct lws *wsi,
                       enum lws_callback_reasons reason,
                       void *user,
                       void *in,
                       size_t len) {
  switch (reason) {
    case LWS_CALLBACK_HTTP:
    {
      if (served_html_file) {
        lws_serve_http_file(wsi, served_html_file, "text/html", NULL, 0);
      }
      break;
    }
    case LWS_CALLBACK_RECEIVE:
    {
      printf("%s\n", (char *)in);
      break;
    }
    case LWS_CALLBACK_ESTABLISHED:
    {
      pthread_mutex_lock(&users_mutex);
      users[num_users++] = wsi;
      pthread_mutex_unlock(&users_mutex);
      break;
    }
    case LWS_CALLBACK_CLOSED:
    {
      // Should handle this.
      break;
    }
    default: break;
  }
  return 0;
}

static int initialize_ws_server(void) {
  struct lws_protocols _protocols[] = {
    {
      "http-only",
      ws_callback,
      0
    },
    { NULL, NULL, 0 }
  };
  protocols = malloc(sizeof(_protocols));
  memcpy(protocols, _protocols, sizeof(_protocols));

  struct lws_context_creation_info *info = calloc(1, sizeof(struct lws_context_creation_info));
  info->port = port;
  info->iface = NULL;
  info->protocols = protocols;
  info->extensions = lws_get_internal_extensions();
  context = lws_create_context(info);
  if (context ==  NULL) {
    return -1;
  }

  return 0;
}

void *server_thread_loop(void *args) {
  while(1) {
    lws_service(context, 50);
  }
  return NULL;
}

void send_buffer(char *buffer, uint32_t len) {
  char out_buffer[LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING];
  memset(&out_buffer[LWS_SEND_BUFFER_PRE_PADDING], 0, len);
  strncpy(&out_buffer[LWS_SEND_BUFFER_PRE_PADDING], buffer, len);
  int i;
  pthread_mutex_lock(&users_mutex);
  for (i = 0; i < num_users; i++) {
    lws_write(users[i], (unsigned char *)&out_buffer[LWS_SEND_BUFFER_PRE_PADDING], len, LWS_WRITE_TEXT);
  }
  pthread_mutex_unlock(&users_mutex);
}

void turn_off_errors(void) {
  // Remove errors
  int devnull_fd = open("/dev/null", O_WRONLY);
  dup2(devnull_fd, STDERR_FILENO);
}

void read_input(void) {
  char buffer[MAX_BUFFER];
  uint32_t pos;
  char ch;
  while (read(STDIN_FILENO, &ch,1) > 0) {
    if (ch == '\n') {
      buffer[pos] = '\0';
      send_buffer(buffer, pos);
      pos = 0;
      continue;
    } else {
      buffer[pos++] = ch;
    }
  }
}

void print_usage() {
  printf("webpipe [-p port] [-f file.html] [-d]\n");
}

int main(int argc, char *argv[]) {
  int debug_flag = 0;
  int c;
  while((c = getopt(argc, argv, "p:f:dh?")) != -1) {
    switch (c) {
      case 'p':
        port = atoi(optarg);
        break;
      case 'f':
        served_html_file = optarg;
        break;
      case 'd':
        debug_flag = 1;
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

  // Start a server on a different thread.
  pthread_t server_thread;
  if (!initialize_ws_server()) {
    pthread_create(&server_thread, NULL, server_thread_loop, NULL);
  }

  // Read input.
  read_input();

  return 0;
}

