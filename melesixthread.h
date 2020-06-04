#ifndef MELESIX_THREAD_H
#define MELESIX_THREAD_H

#include <QThread>
#include <stdint.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <chrono>
#include <thread>
#include <math.h>

#include "MLX90640_API.h"

class Melesix_thread : public QThread
{
	Q_OBJECT
public:
	explicit Melesix_thread(QObject* parent = 0);
private:
	float mlx90640To[768];
	uint16_t eeMLX90640[832];
	paramsMLX90640 mlx90640;
	uint16_t frame[834];
	float emissivity = 0.8;
	float eTa;
	long frame_time_micros;
	int fps;
	std::chrono::microseconds frame_time;
	void run(); 			// QThread ½ÇÇà
signals:
	void FinishTemp(const double value);
public slots:
};

#endif // MELESIX_THREAD_H

