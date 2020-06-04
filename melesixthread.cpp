
#include "melesixthread.h"


using namespace std;

#define TEMP_THREAD_HOLD 35.0
#define OFFSET_MICROS 850

#define FPS 4
#define FRAME_TIME_MICROS (1000000/FPS)

#define MLX_I2C_ADDR 0x33

Melesix_thread::Melesix_thread(QObject *parent) : QThread(parent)
{
	frame_time = std::chrono::microseconds(frame_time_micros + OFFSET_MICROS);

	frame_time_micros =  (1000000/FPS);
	fps = 4;

	
	MLX90640_SetDeviceMode(MLX_I2C_ADDR, 0);
	MLX90640_SetSubPageRepeat(MLX_I2C_ADDR, 0);
	switch(fps){
		case 1:
			MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b001);
			break;
		case 2:
			MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b010);
			break;
		case 4:
			printf("set fps\n");
			MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b011);
			break;
		case 8:
			MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b100);
			break;
		case 16:
			MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b101);
			break;
		case 32:
			MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b110);
			break;
		case 64:
			MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b111);
			break;
		default:
			fprintf(stderr, "Unsupported framerate: %d\n", fps);
			
	}
	MLX90640_SetChessMode(MLX_I2C_ADDR);


	MLX90640_DumpEE(MLX_I2C_ADDR, eeMLX90640);
	MLX90640_SetResolution(MLX_I2C_ADDR, 0x03);
	MLX90640_ExtractParameters(eeMLX90640, &mlx90640);

	printf("init thread\n");

}
void Melesix_thread::run()
{
	float normal_average;
	float limited_average;
	float max_temp;
	int temp_index; 
	bool IsThreadHold = false;

	while(1)
	{
		auto start = std::chrono::system_clock::now();
		MLX90640_GetFrameData(MLX_I2C_ADDR, frame);
		MLX90640_InterpolateOutliers(frame, eeMLX90640);

		eTa = MLX90640_GetTa(frame, &mlx90640); // Sensor ambient temprature
		MLX90640_CalculateTo(frame, &mlx90640, emissivity, eTa, mlx90640To); //calculate temprature of all pixels, base on emissivity of object

		//MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640To, 1, &mlx90640);
		//MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640To, 1, &mlx90640);

		max_temp = 0;	
		normal_average = 0;
		temp_index = 0;
		limited_average = 0;

		IsThreadHold = false;


		for(int y = 0; y < 24; y++)
		{
			for(int x = 0; x < 32; x++)	
			{
				float val = mlx90640To[32 * (23-y) + x];
				
				
				if(val < 250.0)
				{			
					normal_average += val;
				}
				
				if(TEMP_THREAD_HOLD < val && val < 250.0)
				{
					limited_average += val;
					IsThreadHold = true;
					temp_index++;	

				}			
			}	
		}	


		if(IsThreadHold == true)
		{				
			IsThreadHold = false;		
			max_temp = (limited_average	/ temp_index );
		}
		else
		{
			max_temp = (normal_average	/ (32 * 24) );
		}

		cout << "max_temp : " << max_temp <<   endl;		

		auto end = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		std::this_thread::sleep_for(std::chrono::microseconds(frame_time - elapsed));
		//emit FinishTemp(max_temp);
	}
	
}

