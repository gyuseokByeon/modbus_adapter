#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <stdbool.h>
#include <assert.h>
#include <libconfig.h>
#include "config.h"
#include "utility.h"


void initialize_connection(modbus_t *ctx) {
  if (modbus_connect(ctx) == -1) {
    fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
    modbus_free(ctx);
    exit(1);
  }
}

int main(int argc, char*argv[])
{
  struct ModbusConfig *config = parse_config_devices();
  struct ModbusDevice *devices = config->devices;
  print_configs(devices);

  int mIterations = 0;

  struct ModbusDevice *plc = get_device(config, "hitachiwj200");
  struct ModbusDevice *kep = get_device(config, "kepware");

  modbus_t *plc_ctx;
  modbus_t *kep_ctx;

  if (plc == NULL || kep == NULL) {
    printf("Check configuration.");
    exit(1);
  }

  plc_ctx = plc->conn->ctx;
  kep_ctx = kep->conn->ctx;

  if (!plc_ctx || !kep_ctx) {
    fprintf(stderr, "Failed to connect context: %s\n", modbus_strerror(errno));
    exit(1);
  }

  modbus_set_slave(plc_ctx, plc->address);
  modbus_set_slave(kep_ctx, kep->address);

  initialize_connection(plc_ctx);
  initialize_connection(kep_ctx);

  int write_freq_addr = get_data(plc, "write_freq")->address;
  int write_coil_addr = get_data(plc, "write_coil")->address;
  int read_freq_addr  = get_data(plc, "read_freq")->address;
  int read_coil_addr  = get_data(plc, "read_estop")->address;

  set_speed(plc_ctx, write_freq_addr, 15);
  set_coil(plc_ctx, write_coil_addr, 1);
  
  uint16_t real_freq;
  uint8_t estop_coil;
  int success;

  /* Relay values to Kepware via TCP */
  success = read_register(plc_ctx, read_freq_addr, &real_freq);
  if (success == 1) {
    set_speed(kep_ctx, get_data(kep, "write_freq")->address, 15);
    printf("\nreal_freq: %i\n", real_freq);
  }
/
  success = read_coil(plc_ctx, read_coil_addr, &estop_coil);
  if (success == 1) {
    // real_freq is in absolute value not in hertz
    set_coil(kep_ctx, get_data(kep, "write_coil")->address, 1);
  }

  modbus_close(plc_ctx);
  modbus_free(plc_ctx);

  modbus_close(kep_ctx);
  modbus_free(kep_ctx);

  free(config);
  free(devices);
}
