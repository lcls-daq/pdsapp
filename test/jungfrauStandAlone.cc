#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pds/jungfrau/Driver.hh"

void print_status(Pds::Jungfrau::Driver* drv)
{
  printf("status %s\n", drv->status_str().c_str());
}


int main() {

  Pds::Jungfrau::Driver* det = new Pds::Jungfrau::Driver(0, "det-jungfrau-33", "10.1.1.105", 32410, "00:60:dd:43:6a:43", "10.1.1.55");
  
  det->configure(0, JungfrauConfigType::Normal, JungfrauConfigType::Quarter, 0.000238, 0.000010, 200);

  sleep(1);

  uint16_t* data = new uint16_t[50*4096*128];
  int16_t frame = -1;

  if (det->start()) {
    for(int i=0; i<50; i++) {
      frame = det->get_frame(&data[i*4096*128]);
      printf("got frame: %d\n", frame);
    }
    det->stop();
    sleep(1);
  }

  FILE *f = fopen("jungfrau.data", "wb");
  fwrite(data, sizeof(uint16_t), 50*4096*128, f);
  fclose(f);

  delete det;
  det = 0;
  delete[] data;

  return 0;
}
