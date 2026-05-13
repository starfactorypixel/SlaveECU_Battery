#pragma once
#include <OneWireTSensEx.h>

extern TIM_HandleTypeDef htim1;

namespace OneWire
{
	OneWireDriver oneWire(GPIOB, GPIO_PIN_9, &htim1);
	OneWireTSensEx<16> sensors(oneWire);
	
	
	inline void Setup()
	{
		sensors.RegReadyCallback([](OneWireTSensEx<16>::sensor_t *obj, uint8_t count) -> void
		{
			for(uint8_t i = 0; i < count; ++i)
			{
				Temp::PutFrom1WireByIdx(0, obj[i].temp, i);
				Temp::PutFrom1WireByIdx(1, obj[i].temp, i);

				Logger.Printf("Rom: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X, Temp: %05d°C, Valid: %d, Min: %d, Mid: %d, Max: %d", 
				obj[i].rom->raw[0], obj[i].rom->raw[1], obj[i].rom->raw[2], obj[i].rom->raw[3], 
				obj[i].rom->raw[4], obj[i].rom->raw[5], obj[i].rom->raw[6], obj[i].rom->raw[7], 
				obj[i].temp, obj[i].valid, sensors.GetMinTemp(), sensors.GetMidTemp(), sensors.GetMaxTemp()).PrintNewLine();
			}
		});		
		
		return;
	}
	
	inline void Loop(uint32_t &current_time)
	{
		//sensors.Processing(current_time);
		
		
		// При выходе обновляем время
		current_time = HAL_GetTick();
		
		return;
	}
};
