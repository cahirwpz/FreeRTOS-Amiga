#pragma once

typedef enum EvKind {
  EV_READ,  /* there's new data to read */
  EV_WRITE, /* there's more space available for write */
} __packed EvKind_t;
