#pragma once

#include <OtaUpdater.h>

#if __has_include("secrets.h")
#include "secrets.h"

static const OtaUpdater::WifiNetwork WIFI_NETWORKS[] = {
  {SECRET_SSID, SECRET_PASS},
#ifdef SECRET_SSID_2
  {SECRET_SSID_2, SECRET_PASS_2},
#endif
#ifdef SECRET_SSID_3
  {SECRET_SSID_3, SECRET_PASS_3},
#endif
#ifdef SECRET_SSID_4
  {SECRET_SSID_4, SECRET_PASS_4},
#endif
};

#define WIFI_CREDENTIALS WIFI_NETWORKS, sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0])
#else
#define WIFI_CREDENTIALS nullptr, nullptr
#endif
