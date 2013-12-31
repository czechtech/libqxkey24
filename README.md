libqxkey24
==========

QT Library for PI Engineering XK-24 Device

Attaches to first XK-24 device connected to the computer and creates qt singals/slots for all events:
  Slots:
    queryForDevice
    setBacklightIntensity
    setButtonBlueLEDState
    setButtonRedLEDState
    setPanelLED
  Signals:
    panelDisconnected
	panelConnected
    errorEvent
    dataEvent
    buttonDown
    buttonUp

Install:
  qmake && make
  sudo make install

Example Use:
  test.cpp

Depends on:
  QT 4.x
  piehid -- https://github.com/piengineering/xkeys

OS Compatibility:
  Linux
  Windows
  Mac OS X

**QT and PIEHID and this library should be compatible on all three major operating systems.  Code was originally developed in a linux environment.  As such, windows and mac os x will likely need minor tweaks to the qmake project file to properly link with the necessary libraries.
