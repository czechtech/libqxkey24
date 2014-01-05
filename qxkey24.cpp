#include "qxkey24.h"

QXKey24 *g_x_key_24; // HACK

QXKey24::QXKey24(QObject* parent)
	: QObject(parent), m_dbus( QDBusConnection::connectToBus( QDBusConnection::SystemBus, "system" ) )
{
	g_x_key_24 = this; // HACK
	
	m_dev        = NULL;
	m_devicePath = "";
	
	m_buttons = new unsigned char[XK24_REPORT_LENGTH];
	m_buttonTimes = new quint32[XK24_BUTTONS];

	m_timer = new QTimer(this);
	m_timer->start(500);

	connect(this, SIGNAL(panelDisconnected()), this, SLOT(queryForDevice()));
	connect(m_timer, SIGNAL(timeout()),        this, SLOT(queryForDevice()));
	
	// Listen for USB Device List changes: http://www.qtforum.org/article/27807/problem-in-connecting-to-deviceadded-signal.html
	// Or, use SYSFS: http://www.signal11.us/oss/udev/ -- "libudev - Monitoring Interface"
	m_dbus.connect("org.freedesktop.Hal",
					"/org/freedesktop/Hal/Manager",
					"org.freedesktop.Hal.Manager",
					"DeviceRemoved",
					this, SLOT(busDeviceRemoved(QString)));
}


QXKey24::~QXKey24()
{
	m_timer->stop();
	
	if(hasDevice()) {
		m_devicePath = "";
		CloseInterface(m_dev->Handle);
	}

	//delete m_dev; responsiblity of piehid
	delete m_buttons;
	delete m_buttonTimes;
	delete m_timer;
}


// Slot:
void
QXKey24::busDeviceRemoved(QString dev)
{
	// DJC: can dev and m_devicePath be compared?
	qDebug() << "QXKeys: Some Device Removed" << dev << endl;
	queryForDevice();
}


// Slot:
void
QXKey24::queryForDevice()
{
	TEnumHIDInfo info[MAX_XKEY_DEVICES];
	long count;

	qDebug() << "Enumerating Attached PI Engineering Devices.";

	unsigned int res = EnumeratePIE(PI_VID, info, &count);

	if(res != 0) {
		cerr << "QXKeys: Error [" << res << "] Finding PI Engineering Devices." << endl;
		return;
	}

	// Test for change in current device connection:
	if(m_devicePath != "") {
		for(int i = 0; i < count; i++) {
			TEnumHIDInfo *d = &info[i];
			if(m_devicePath == (QString)(d->DevicePath)) { // uuid would be more appropriate
				setupDevice(d);
				return;
			}
		}
		qDebug() << "QXKeys: Device Disconnected from " << m_devicePath;
		m_dev = NULL;
		m_devicePath = "";
		m_timer->setInterval(500);
		
		emit panelDisconnected();
	}
	
	// Setup Interface:
	for(int i = 0; i < count && !hasDevice(); i++) {
		TEnumHIDInfo *d = &info[i];
		if((d->PID == XK24_PID1 || d->PID == XK24_PID2) &&
			d->UP == XK24_USAGE_PAGE && d->Usage == XK24_USAGE) {
			setupDevice(d);
			memset(m_buttons, 0, XK24_REPORT_LENGTH);
			memset(m_buttonTimes, 0, XK24_BUTTONS);
			qDebug() << "QXKeys: Found New Device at " << getDevicePath();
			sendCommand(XK24_CMD_DESC, 0);
			sendCommand(XK24_CMD_TIMES, true);
			//sendCommand(XK24_CMD_STATUS, 0);

			emit panelConnected();
		}
	}
}


//private
void
QXKey24::setupDevice(TEnumHIDInfo *d)
{
	if(d == NULL) {
		return;
	}

	char *p = d->DevicePath;
	long h  = d->Handle;
	unsigned int e;

	e = SetupInterfaceEx(h);
	if(e != 0) {
		cerr << "QXKey24: Failed [" << e << "] Setting up PI Engineering Device at " << p << endl;
		return;
	}

	e = SetDataCallback(h, HandleDataEvent);
	if(e != 0) {
		cerr << "QXKey24: Critical Error [" << e << "] setting event callback for device at " << p << endl;
		return;
	}

	e = SetErrorCallback(h, HandleErrorEvent);
	if(e != 0) {
		cerr << "QXKey24: Critical Error [" << e << "] setting error callback for device at " << p << endl;
		return;
	}

	SuppressDuplicateReports(h, false); // true?

	m_dev = d;
	m_devicePath = d->DevicePath;

	//m_timer->setInterval(5000);	
	m_timer->stop(); // stop querying
}


/*static*/
unsigned int
QXKey24::HandleDataEvent(unsigned char *pData, unsigned int deviceID, unsigned int error)
{
	return g_x_key_24->handleDataEvent(pData, deviceID, error);
}
// private:
unsigned int
QXKey24::handleDataEvent(unsigned char *pData, unsigned int deviceID, unsigned int error)
{
	emit dataEvent(pData);

	// Constant 214
	if(pData[0] == 0 && pData[2] != 214)
	{
		processButtons(pData);
	}
	
	if(error != 0) {
		handleErrorEvent(deviceID, error);
	}
	
	return true;
}


/*static*/
unsigned int
QXKey24::HandleErrorEvent(unsigned int deviceID, unsigned int status)
{
	return g_x_key_24->handleErrorEvent(deviceID, status);
}
// private:
unsigned int
QXKey24::handleErrorEvent(unsigned int deviceID, unsigned int status)
{
	if(!hasDevice()) {
		return false;
	}
	
	char err[128];
	GetErrorString(status, err, 128);

	cerr << "QXKeys: ErrorEvent deviceID: " << deviceID << endl;
	qDebug() << err;

	emit errorEvent(status);

	queryForDevice();
	return true;
}


// Public:
bool
QXKey24::isButtonDown(int num)
{
	if(!hasDevice() || isNotButtonNumber(num)) {
		return false;
	}
	int col = 3+ num / 8; // button data stored in 3,4,5,6
	int row = num % 8;
	int bit = 1 << row;
	
	bool isdown = (m_buttons[col] & bit) > 0;

	return isdown;
}


// Public:
void
QXKey24::sendKeyboardMsg(unsigned char modifier, unsigned char hc1, unsigned char hc2, unsigned char hc3,
												 unsigned char hc4, unsigned char hc5, unsigned char hc6)

{
	if(!hasDevice()) {
		return;
	}

	/*
	Modifier: Bit 1=Left Ctrl, bit 2=Left Shift, bit 3=Left Alt, bit 4=Left Gui, bit 5=Right Ctrl, bit 6=Right Shift, bit 7=Right Alt, bit 8=Right Gui.
	HC1=Hid Code for 1st key down, or 0 to release previous key press in this byte position.
	HC2=Hid Code for 2nd key down, or 0 to release previous key press in this byte position.
	HC3=Hid Code for 3rd key down, or 0 to release previous key press in this byte position.
	HC4=Hid Code for 4th key down, or 0 to release previous key press in this byte position. 
	HC5=Hid Code for 5th key down, or 0 to release previous key press in this byte position.
	HC6=Hid Code for 6th key down, or 0 to release previous key press in this byte position.
	*/
	sendCommand(XK24_CMD_KBD, modifier, 0, hc1, hc2, hc3, hc4, hc5, hc6);
}


// Public:
void
QXKey24::sendMouseMsg(unsigned char buttons, unsigned char x, unsigned char y, unsigned char whlX, unsigned char whlY)
{
	if(!hasDevice()) {
		return;
	}
	
	if(getPID() != XK24_PID1) {
		sendCommand(XK24_CMD_SET_PID, 2); // Mode "2" is for PID #1
	}

	/*
	Buttons: Bit 1=Left, bit 2=Right, bit 3=Center, bit 4=XButton1, bit 5=XButton2.
	X=Mouse X motion. 128=0 no motion, 1-127 is right, 255-129=left, finest inc (1 and 255) to coarsest (127 and 129).
	Y=Mouse Y motion. 128=0 no motion, 1-127 is down, 255-129=up, finest inc (1 and 255) to coarsest (127 and 129).
	WX=Wheel X. 128=0 no motion, 1-127 is up, 255-129=down, finest inc (1 and 255) to coarsest (127 and 129). 
	WY=Wheel Y. 128=0 no motion, 1-127 is up, 255-129=down, finest inc (1 and 255) to coarsest (127 and 129).
	*/
	sendCommand(XK24_CMD_MOUSE, buttons, x, y, whlX, whlY);
}


// Public:
void
QXKey24::sendJoystickMsg(unsigned char      x, unsigned char   y, unsigned char zRot, unsigned char   z,
						 unsigned char slider, unsigned char gb1, unsigned char  gb2, unsigned char gb3,
						 unsigned char    gb4, unsigned char hat)
{
	if(!hasDevice()) {
		return;
	}
	
	if(getPID() != XK24_PID2) {
		sendCommand(XK24_CMD_SET_PID, 0); // Mode "0" is for PID #2
	}

	/*
	X: Joystick X, 0-127 is from center to full right, 255-128 is from center to full left.
	Y: Joystick Y, 0-127 is from center to bottom, 255-128 is from center to top.
	Z rot.: Joystick Z rot., 0-127 is from center to bottom, 255-128 is from center to top.
	Z.: Joystick Z, 0-127 is from center to bottom, 255-128 is from center to top.
	Slider: Joystick Slider, 0-127 is from center to bottom, 255-128 is from center to top. 
	GB1: Game buttons 1-8, bit 1= game button 1, bit 2=game button 2, etc.
	GB2: Game buttons 9-16, bit 1= game button 9, bit 2=game button 10, etc.
	GB3: Game buttons 17-24, bit 1= game button 17, bit 2=game button 18, etc.
	GB4: Game buttons 25-32, bit 1= game button 25, bit 2=game button 26, etc. 
	Hat: 0 to 7 clockwise, 8 is no hat.
	*/
	sendCommand(XK24_CMD_JOYSTCK, x, y, zRot, z, slider, gb1, gb2, gb3, gb4, 0, hat);
}


// Slot:
void
QXKey24::setFlashFrequency(unsigned char freq)
{
	if(!hasDevice()) {
		return;
	}

	sendCommand(XK24_CMD_FLSH_FR, freq);
}


// Slot:
void
QXKey24::setBacklightIntensity(float blueBrightness, float redBrightness)
{
	if(!hasDevice()) {
		return;
	}

	// remove whole digits
	blueBrightness -= (int)blueBrightness;
	redBrightness  -=  (int)redBrightness;
	unsigned char bb = blueBrightness * 255;
	unsigned char rb =  redBrightness * 255;

	// Red backlight LEDs are bank 2
	sendCommand(XK24_CMD_LED_INT, bb, rb);
}


// Slot:
void
QXKey24::setButtonRedLEDState(quint8 ledNum, LEDMode mode)
{
	if(!hasDevice() || isNotButtonNumber(ledNum) ) {
		return;
	}
	
	// Red backlight LEDs are indexed +32 higher than the blues
	sendCommand(XK24_CMD_BTN_LED, ledNum+32, mode);
}


// Slot:
void
QXKey24::setButtonBlueLEDState(quint8 ledNum, LEDMode mode)
{
	if(!hasDevice() || isNotButtonNumber(ledNum)) {
		return;
	}
	
	sendCommand(XK24_CMD_BTN_LED, ledNum, mode);
}


// Slot:
void
QXKey24::setPanelLED(PanelLED ledNum, LEDMode mode)
{
	if(!hasDevice()) {
		return;
	}

	quint8 n = (quint8)ledNum;
	quint8 m = (quint8)mode;

	sendCommand(XK24_CMD_PNL_LED, n, m);
}


// private:
bool
QXKey24::isNotButtonNumber(int num)
{
	return ( !( (num >=  0 && num <  6) ||
				(num >=  8 && num < 14) ||
				(num >= 16 && num < 22) ||
				(num >= 24 && num < 30) )) ;
}


//private:
void
QXKey24::processButtons(unsigned char *pData)
{
	int columns = 4;
	int rows    = 8;
	int d       = 3; // start byte
	
	for(int i = 0; i < columns; i++) {
		for(int j = 0; j < rows; j++) {
			unsigned char bit = 1 << j;
			bool wasdown = (m_buttons[i+d] & bit) > 0;
			bool  isdown = (  pData[i+d] & bit) > 0;

			if( isdown && !wasdown ) {
				quint32 time = dataToTime(pData);
				m_buttonTimes[i*6+j] = time;
				
				emit buttonDown(j+i*rows);
				emit buttonDown(j+i*rows, time);
			}
			if( !isdown && wasdown ) {
				quint32 time = dataToTime(pData);
				quint32 duration = time - m_buttonTimes[i*6+j];
				m_buttonTimes[i*6+j] = 0;
				
				emit buttonUp(j+i*rows);
				emit buttonUp(j+i*rows, time);
				emit buttonUp(j+i*rows, time, duration);
			}
		}
		
		// Update 
		m_buttons[i+d] = pData[i+d];
	}
}


// private:
quint32
QXKey24::dataToTime(unsigned char *pData)
{
	return
	  (uint32_t)pData[7] << 24 |
      (uint32_t)pData[8] << 16 |
      (uint32_t)pData[9] << 8  |
      (uint32_t)pData[10];
}


// private:
unsigned char*
QXKey24::createDataBuffer()
{
	int length = GetWriteLength(m_dev->Handle);

	unsigned char *buffer = new unsigned char[length];
	memset(buffer, 0, length);
	
	return buffer;
}


// private:
void
QXKey24::sendCommand(unsigned char command, unsigned char data1,  unsigned char data2, unsigned char data3,
											unsigned char data4,  unsigned char data5, unsigned char data6,
											unsigned char data7,  unsigned char data8, unsigned char data9,
											unsigned char data10, unsigned char data11)
{
	if(!hasDevice()) {
		return;
	}

	unsigned char *buffer = createDataBuffer();
	
	buffer[0]  = 0; // constant
	buffer[1]  = command;
	buffer[2]  = data1;
	buffer[3]  = data2;
	buffer[4]  = data3;
	buffer[5]  = data4;
	buffer[6]  = data5;
	buffer[7]  = data6;
	buffer[8]  = data7;
	buffer[9]  = data8;
	buffer[10] = data9;
	buffer[11] = data10;
	buffer[12] = data11;
	
	unsigned int result;
	result = WriteData(m_dev->Handle, buffer);

	if (result != 0) {
		cerr << "QXKeys: Write Error [" << result << "]  - Unable to write to Device at " << m_dev->DevicePath << endl;
		queryForDevice();
	}

	delete[] buffer;
}


// public:
QString
QXKey24::getProductString()
{
	if(!hasDevice()) {
		return "";
	}
	
	QString q = m_dev->ProductString;
	return q;
}


// public:
QString
QXKey24::getManufacturerString()
{
	if(!hasDevice()) {
		return "";
	}

	QString q = m_dev->ManufacturerString;
	return q;
}
