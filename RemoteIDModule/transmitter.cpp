/*
  common functions for all transmitter backends
 */

#include "transmitter.h"

void Transmitter::generate_random_mac(uint8_t mac[6])
{
    for (uint8_t i=0; i<6; i++) {
        mac[i] = uint8_t(random(256));
    }
}
