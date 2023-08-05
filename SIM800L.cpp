#include "SIM800L.h"

// Initialize the static member variable
String GSM::_cr = "";

GSM::GSM(Stream& serialPort) : _serial(serialPort) {}

bool GSM::begin() {
  return GSM::ping();
}

bool GSM::ping() {
  return sendATCommand("AT");
}

int GSM::getCsq(bool raw) { // raw = true ==  csq, raw = false == percent
  String response;
  sendATCommand("AT+CSQ", response);
  response.remove(0, 7);

  int commaPos = response.indexOf(',');
  if (commaPos >= 0) {
    if (raw)
      return response.substring(0, commaPos).toInt();
    else
      return response.substring(commaPos + 1).toInt();
  }

  return 0;
}

String GSM::getCCID() {
  String response;
  sendATCommand("AT+CCID", response);
  //response.remove(0, 7);
  return response;
}

String GSM::getRegisterStatus() {
  String response;
  sendATCommand("AT+CREG?", response);
  response.remove(0, 7);
  return response;
}

String GSM::getModuleInfo() {
  String response;
  sendATCommand("ATI", response);
  return response;
}

String GSM::getNetworkName() {
  String response;
  sendATCommand("AT+COPS?", response);
  response.remove(0, 12);
  return response;
}

bool GSM::getBatteryData(int& batteryPercent, int& batteryVoltage) {
  String response;
  if (sendATCommand("AT+CBC", response)) {
    response.remove(0, 8);

    int commaPos = response.indexOf(',');
    if (commaPos >= 0) {
      batteryPercent = response.substring(0, commaPos).toInt();
      batteryVoltage = response.substring(commaPos + 1).toInt();
      return true;
    }
  }
  return false;
}


String GSM::getPhoneNumber() {
  String response;
  if (sendATCommand("AT+CNUM", response)) {
    int start = response.indexOf("\"") + 1;
    int end = response.indexOf("\"", start);
    if (start > 0 && end > 0 && end > start) {
      return response.substring(start, end);
    }
  }
  return "";
}

int GSM::getFunctionality() {
  String response;
  sendATCommand("AT+CFUN?", response);
  response.remove(0, 7);
  int number = response.toInt();
  return number;
}

bool GSM::setFunctionality(int mode) {
  String command = "AT+CFUN=";
  command += mode;
  return sendATCommand(command.c_str());
}

int GSM::getSleep() {
  String response;
  sendATCommand("AT+CSCLK?", response);
  response.remove(0, 7);
  int number = response.toInt();
  return number;
}

bool GSM::setSleep(bool sleep) {
  String command = "AT+CSCLK=0";
  if (sleep) command = "AT+CSCLK=2";
  return sendATCommand(command.c_str());
}

bool GSM::powerOff() {
  return sendATCommand("AT+CPOWD=1");
}

bool GSM::sendSMS(String number, String text) { // number in format "+48501501501"
  sendATCommand("AT+CMGF=1"); // text message mode
  String command = "AT+CMGS=\"" + number + "\"";
  sendATCommand(command.c_str());
  _serial.print(text.c_str());
  _serial.write(26); // SMS text section end, Ctrl+Z
  while (_serial.available()) {
    if (_serial.readString() == "OK") // did it send?
      return true;
  }

  return false;
}

bool GSM::sendATCommand(const char* cmd, String& response, unsigned long timeout) {
  while (_serial.available()) {
    _serial.read();
  }

  _serial.println(cmd);
  unsigned long startTime = millis();

  while ((millis() - startTime) < timeout) {
    if (_serial.available()) {
      response = _serial.readStringUntil('\n');
      response.trim();

      if (response == "ERROR") return false;

      if (response.length() > 5) {
        return true; // return prematurely since response contains custom info
      }

      if (response == "OK") {
        return true;
      }
    }
  }
  return false;
}