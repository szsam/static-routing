#include <stdint.h>
#include <netinet/in.h> /* for in_addr */

int search_dev_tbl(uint8_t *);

int search_route_tbl(struct in_addr ipaddr);
struct in_addr gateway(int);
const char *iface(int);

const uint8_t *ip2mac(struct in_addr);

void init_config();
