#include <filesystem>
#include <iostream>
#include <libnotify/notify.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace fs = std::filesystem;
const char *DGRAM_FILE = "#abstract:simpleeyeprotector.dgram";
const char *STATUS_FILE = "/tmp/simpleeyeprotector.status";

void notify(bool is_working, int minutes) {
  char buf[16];
  sprintf(buf, "%d minutes", minutes);
  auto msg = is_working ? "Take a break" : "Get to work";
  auto icon = is_working ? "face-smile" : "face-plain";
  auto obj = notify_notification_new(msg, buf, icon);
  auto sound = g_variant_new_string("alarm-clock-elapsed");
  notify_notification_set_hint(obj, "sound-name", sound);
  if (is_working) {
    notify_notification_set_timeout(obj, minutes * 60 * 1000);
  }
  if (!notify_notification_show(obj, NULL)) {
    std::cerr << "failed to send notification." << std::endl;
  };
  g_object_unref(G_OBJECT(obj));
}

void usage() {
  std::cout << "sep [start <work> <break>|stop|status|skip]" << std::endl;
}

void write_status(int minutes, int max) {
  auto f = std::fopen(STATUS_FILE, "w+");
  if (minutes == -1) {
    std::fputs("N/A", f);
  } else {
    fprintf(f, "%d/%d", minutes, max);
  }
  std::fclose(f);
}

int listen(int work, int break_) {
  int sock;
  sockaddr_un addr;
  socklen_t addrlen;
  char buf[8];
  int n;

  sock = socket(AF_UNIX, SOCK_DGRAM, 0);
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, DGRAM_FILE);
  addrlen = SUN_LEN(&addr);
  addr.sun_path[0] = '\0';

  if (bind(sock, (sockaddr *)&addr, addrlen)) {
    return -1;
  }
  auto start_time = std::chrono::steady_clock::now();
  auto is_working = true;
  write_status(0, work);

  notify_init("simpleeyeprotector");
  struct timeval tv;
  tv.tv_sec = 60;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv,
             sizeof(struct timeval));

  while (1) {
    memset(buf, 0, sizeof(buf));
    n = recv(sock, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
      if (strcmp(buf, "stop") == 0) {
        write_status(-1, 0);
        break;
      } else if (strcmp(buf, "skip") == 0) {
        start_time = std::chrono::steady_clock::now();
        is_working = true;
      }
    }
    auto span = std::chrono::duration_cast<std::chrono::minutes>(
        std::chrono::steady_clock::now() - start_time);
    // take a break
    if (is_working && span.count() >= work) {
      notify(is_working, break_);
      start_time = std::chrono::steady_clock::now();
      is_working = false;
      write_status(0, break_);
    // go to work
    } else if (!is_working && span.count() >= break_) {
      notify(is_working, work);
      start_time = std::chrono::steady_clock::now();
      is_working = true;
      write_status(0, work);
    } else {
      write_status(span.count(), is_working ? work : break_);
    }
  }

  close(sock);
  return 0;
}

void start(int work, int break_) {
  pid_t pid;
  switch (pid = fork()) {
  case -1:
    std::cerr << "failed to fork" << std::endl;
    break;
  case 0:
    // daemon process
    if (0 != listen(work, break_)) {
      std::cerr << "failed to bind" << std::endl;
    };
    exit(0);
  default:
    break;
  }
}

void control(const char *code) {
  int sock;
  sockaddr_un addr;
  socklen_t addrlen;
  sock = socket(AF_UNIX, SOCK_DGRAM, 0);
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, DGRAM_FILE);
  addrlen = SUN_LEN(&addr);
  addr.sun_path[0] = 0;
  sendto(sock, code, strlen(code), 0, (sockaddr *)&addr, addrlen);
  close(sock);
}

void status() {
  if (!fs::exists(STATUS_FILE)) {
    std::cout << "N/A" << std::endl;
  } else {
    auto f = std::fopen(STATUS_FILE, "r");
    char buf[8];
    auto s = std::fgets(buf, 7, f);
    std::fclose(f);
    if (s != nullptr) {
      std::cout << s << std::endl;
    } else {
      std::cout << "N/A" << std::endl;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    usage();
  } else if (strcmp(argv[1], "start") == 0) {
    if (argc != 4) {
      usage();
      return 0;
    }
    int work = atoi(argv[2]);
    int break_ = atoi(argv[3]);
    start(work, break_);
    std::cout << "started" << std::endl;
  } else if (strcmp(argv[1], "stop") == 0) {
    control("stop");
    std::cout << "stoped" << std::endl;
  } else if (strcmp(argv[1], "status") == 0) {
    status();
  } else if (strcmp(argv[1], "skip") == 0) {
    control("skip");
    std::cout << "skiped" << std::endl;
  }

  return 0;
}
