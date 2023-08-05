/*
05.08.2023 10:51 UTC
This code has been written especially for the OpenNokiaProject
by it's first contributor and developer in order to replace
TinyGSM and reduce the size of the whole firmware. The only GSM
module it supports is SIM800L since it's the one and only that's
used in the OpenNokiaProject.
*/

#ifndef SIM800L_H
#define SIM800L_H

#include "Arduino.h"

class GSM {
public:
  GSM(Stream& serialPort);
  bool begin();
  bool ping();
  int getFunctionality();
  bool setFunctionality(int mode);
  int getSleep();
  bool setSleep(bool sleep);
  bool powerOff();
  int getCsq(bool raw = true);
  String getCCID();
  String getRegisterStatus();
  String getModuleInfo();
  String getNetworkName();
  bool sendSMS(String number, String text);
  bool getBatteryData(int& batteryPercent, int& batteryVoltage);
  String getPhoneNumber();
  bool sendATCommand(const char* cmd, String& response = _cr, unsigned long timeout = 500);
  
private:
  Stream& _serial;
  static String _cr; // common throwaway response variable, hacky workaround
  // bool sendATCommand(const char* cmd, String& response = _cr, unsigned long timeout = 500);
};

#endif