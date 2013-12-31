#include <iostream>
#include <qxkey24.h>
#include <QCoreApplication>
#include <QTimer>

using namespace std;

QXKey24 *g_xkey24;

class TestClass: public QObject
{
    Q_OBJECT
public slots:
	void buttonDown(unsigned int num, quint32 time);
	void buttonUp(unsigned int num, quint32 time, quint32 duration);
};

void TestClass::buttonDown(unsigned int num, quint32 time)
{
	qDebug() << time << ": Button " << num << " Down";
}

void TestClass::buttonUp(unsigned int num, quint32 time, quint32 duration)
{
	qDebug() << time << ": Button " << num << " Up.  Was down for " << duration << "ms";
	g_xkey24->setButtonBlueLEDState(num, OFF);
	g_xkey24->setButtonRedLEDState(num, OFF);
	if(duration > 1000) {
		qDebug() << "Blue LED On";
		g_xkey24->setButtonBlueLEDState(num, ON);
	}
	if(duration > 2000) {
		qDebug() << "Red LED On";
		g_xkey24->setButtonRedLEDState(num, ON);
	}
}


int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  TestClass t;
  QXKey24 x(NULL);
  g_xkey24 = &x;
  
  QObject::connect(&x, SIGNAL(buttonDown(unsigned int, quint32)), &t, SLOT(buttonDown(unsigned int, quint32)));
  QObject::connect(&x, SIGNAL(buttonUp(unsigned int, quint32, quint32)), &t, SLOT(buttonUp(unsigned int, quint32, quint32)));
  
  QTimer::singleShot(30000, &a, SLOT(quit()));
  return a.exec();
}

#include "test.moc" //http://stackoverflow.com/questions/5854626/qt-signals-and-slots-error-undefined-reference-to-vtable-for