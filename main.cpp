#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

typedef struct
{
  std::string name;
  char macaddr[6];
} WOLTarget;

int parse_physical_addr(char *dst, const std::string &str)
{
  std::string hex;
  size_t i = 0;

  for (char const &c : str)
  {
    if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
    {
      hex.push_back(c);
      if (hex.length() == 2)
      {
        dst[i] = std::stoi(hex, nullptr, 16);
        hex.clear();
        ++i;
        if (i > 6)
          break;
      }
    }
  }
  return 0;
}

int create_wol_packet(WOLTarget &tgt, char *dst)
{
  for (size_t i = 0; i < 6; ++i)
    dst[i] = 0xFF;
  for (size_t i = 6; i < 102; ++i)
    dst[i] = tgt.macaddr[i % 6];
  return 0;
}

std::vector<WOLTarget> get_config()
{
  std::vector<WOLTarget> targets;

  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  const std::string configpath = std::string(homedir) + "/.config/wol_targets";
  std::ifstream ifs(configpath);

  if (!ifs.is_open())
  {
    std::cerr << "Error: no target on " << configpath << std::endl;
    return targets;
  }

  std::string line, name, addrbuf;
  while (std::getline(ifs, line))
  {
    if (line.at(0) == '#' || line.length() < 3)
      continue;
    std::stringstream ss{line};
    std::getline(ss, name, ' ');
    std::getline(ss, addrbuf);
    char addr[6];
    parse_physical_addr(addr, addrbuf);
    WOLTarget t{name};
    memcpy(t.macaddr, addr, 6);
    targets.push_back(t);
  }
  return targets;
}

#define WOL_PORT 9

int send_wolsignal(WOLTarget &tgt)
{
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1)
    return 1;
  int y = 1;
  setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&y, sizeof(y));
  struct sockaddr_in sa = {0};
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  sa.sin_port = htons(WOL_PORT);
  char buf[102];
  create_wol_packet(tgt, buf);
  int r = sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&sa, sizeof(sa));
  if (r == -1)
    return -1;
  close(sock);
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "usage: " << argv[0] << " target_name" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::vector<WOLTarget> targets = get_config();
  for (auto t = targets.begin(), end = targets.end(); t != end; ++t)
  {
    for (int i = 1; i < argc; i++)
    {
      if (!strcmp(t->name.c_str(), argv[i]))
      {
        if (!send_wolsignal(*t))
          std::cout << "sent signal to " << argv[i] << std::endl;
        else
          std::cerr << "failed to send to " << argv[i] << std::endl;
      }
    }
  }
  return 0;
}
