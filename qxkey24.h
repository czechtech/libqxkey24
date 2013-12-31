#ifndef QXKEYS_H
#define QXKEYS_H

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <QObject>
#include <QDebug>
#include <QMap>
#include <QTimer>
#include <PieHid32.h>

#define XK24_PID1 0x0405
#define XK24_PID2 0x0403

#define XK24_REPORT_LENGTH 33
#define XK24_MESSAGE_LENGTH 36

#define XK24_BUTTONS 24

#define XK24_CMD_BTN_LED 181
#define XK24_CMD_PNL_LED 179
#define XK24_CMD_DESC    214
#define XK24_CMD_STATUS  177
#define XK24_CMD_TIMES   210
#define XK24_CMD_LED_INT 187
#define XK24_CMD_FLSH_FR 180
#define XK24_CMD_KBD     201
#define XK24_CMD_MOUSE   203
#define XK24_CMD_JOYSTCK 202
#define XK24_CMD_SET_PID 204

#define XK24_USAGE_PAGE 0x000c
#define XK24_USAGE      0x0001

typedef enum {
	OFF   = 0,
	ON    = 1,
	BLINK = 2
} LEDMode;

typedef enum {
	GRN_LED = 6,
	RED_LED = 7
} PanelLED;

using namespace std;



class QXKey24 : public QObject
{
	Q_OBJECT

public:
	explicit QXKey24(QObject* parent = NULL);
	~QXKey24();

protected:
	TEnumHIDInfo* getDevice() { return m_dev; }
	
private:
	TEnumHIDInfo  *m_dev;
	QTimer        *m_timer;
	unsigned char *m_buttons;
	quint32       *m_buttonTimes;
	QString       m_devicePath;

public:
	bool         hasDevice()        { return (m_dev != NULL); }; // && m_dev->Handle != 0

    unsigned int getPID()           { return (hasDevice()?m_dev->PID       : 0);  };
    unsigned int getUsage()         { return (hasDevice()?m_dev->Usage     : 0);  };
    unsigned int getUP()            { return (hasDevice()?m_dev->UP        : 0);  };
    long         getReadSize()      { return (hasDevice()?m_dev->readSize  : 0);  };
    long         getWriteSize()     { return (hasDevice()?m_dev->writeSize : 0);  };
    unsigned int getHandle()        { return (hasDevice()?m_dev->Handle    : 0);  };
    unsigned int getVersion()       { return (hasDevice()?m_dev->Version   : 0);  };
    QString      getDevicePath()    { return (hasDevice()?m_devicePath     : ""); };

    QString      getProductString();
    QString      getManufacturerString();

	// keyboard reflector
	void sendKeyboardMsg(unsigned char modifier, unsigned char hc1, unsigned char hc2, unsigned char hc3,
												 unsigned char hc4, unsigned char hc5, unsigned char hc6);
	// mouse reflector
	void    sendMouseMsg(unsigned char buttons,  unsigned char X,   unsigned char Y,
												 unsigned char whlX,unsigned char whlY);
	// joystick reflector
	void sendJoystickMsg(unsigned char   x, unsigned char      y, unsigned char zRot,
						 unsigned char   z, unsigned char slider, unsigned char  gb1,
						 unsigned char gb2, unsigned char    gb3, unsigned char  gb4,
						 unsigned char hat);


public slots:
	void queryForDevice();

	void setBacklightIntensity(float blueBrightness, float redBrightness);
	void setFlashFrequency(unsigned char freq);

	void setButtonBlueLEDState(quint8 buttonNumber, LEDMode mode);
	void  setButtonRedLEDState(quint8 buttonNumber, LEDMode mode);

	void setPanelLED(PanelLED ledNum, LEDMode mode);

signals:
	void panelDisconnected();
	void    panelConnected();

	void errorEvent(unsigned int  status);
	void  dataEvent(unsigned char *pData);
	
	void buttonDown(unsigned int number, quint32 timeStamp);
	void   buttonUp(unsigned int number, quint32 timeStmap);
	void   buttonUp(unsigned int number, quint32 timeStmap, quint32 pressDuration);

private:
	void setupDevice(TEnumHIDInfo *dev);
	void sendCommand(unsigned char command, unsigned char data1  = 0, unsigned char data2  = 0, unsigned char data3 = 0,
											unsigned char data4  = 0, unsigned char data5  = 0, unsigned char data6 = 0,
											unsigned char data7  = 0, unsigned char data8  = 0, unsigned char data9 = 0,
											unsigned char data10 = 0, unsigned char data11 = 0);
	void processButtons(unsigned char *pData);
	bool isNotButtonNumber(int num);
	quint32 dataToTime(unsigned char *pData);

	unsigned char* createDataBuffer();
	void sendData(unsigned char *buffer);

	static unsigned int HandleErrorEvent(unsigned int deviceID, unsigned int status);
	       unsigned int handleErrorEvent(unsigned int deviceID, unsigned int status);
	static unsigned int  HandleDataEvent(unsigned char  *pData, unsigned int deviceID, unsigned int error);
	       unsigned int  handleDataEvent(unsigned char  *pData, unsigned int deviceID, unsigned int error);
};

#endif //QXKEYS_H
