#include <stdint.h>

int search_dev_tbl(uint8_t *);

int search_route_tbl(uint32_t);
uint32_t gateway(int);
const char *iface(int);

const uint8_t *ip2mac(uint32_t);
