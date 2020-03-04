#ifndef IOTMODULE_H
#define IOTMODULE_H

#include <Arduino.h>
#include <eboot_command.h>

#include <IotLogger.h>
#include <WifiConnectionManager.h>
#include <IotWebServer.h>
#include <IotTime.h>
#include <IotPicUpdater.h>
#include <IotUartComm.h>
#include <IotHexFileReader.h>
#include <IotUtility.h>

#define JSON_BUFFER_SIZE 1000

#define BOOT_BUTTON_PIN 0
#define LED_PIN 4
#define LED_GREEN 0
#define LED_RED 1

#define ICSP_MCLR_PIN 12
#define ICSP_CLK_PIN 15
#define ICSP_DAT_PIN 13

#define RESET_SAFEMODE_DATA {0xffeeeeff, 0xcbabcdef}
#define RESET_SAFEMODE_ADDR sizeof(struct eboot_command)/4
#define RESET_SAFEMODE_SIZE 2

#endif