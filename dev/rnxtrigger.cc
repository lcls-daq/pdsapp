/* $Id$ */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pds/rayonix/rayonix_common.hh"
#include "rnxtrigger.hh"

#define DEST_IPADDR "10.0.1.101"

data_footer_t triggerMsg;
int writeSize;
int triggerFd;
struct sockaddr_in triggeraddr;

void triggerInit()
{
  /* open trigger socket */
  triggerFd = socket(AF_INET,SOCK_DGRAM, 0);
  if (triggerFd == -1) {
    perror("socket");
  }

  /* init triggerMsg */
  bzero(&triggerMsg, sizeof(triggerMsg));

  /* init triggeraddr */
  bzero(&triggeraddr, sizeof(triggeraddr));
  triggeraddr.sin_family = AF_INET;
  triggeraddr.sin_addr.s_addr=inet_addr(DEST_IPADDR);
  triggeraddr.sin_port=htons(RNX_SIM_TRIGGER_PORT);
}

void trigger()
{
  /* write to trigger socket */
  int sent = sendto(triggerFd, (void *)&triggerMsg, sizeof(triggerMsg), 0,
             (struct sockaddr *)&triggeraddr, sizeof(triggeraddr));
  if (sent == -1) {
    perror("sendto");
  } else if (sent != sizeof(triggerMsg)) {
    printf("%s: sent %d of %u bytes\n", __FUNCTION__, sent, sizeof(triggerMsg));
  }
}
