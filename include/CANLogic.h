#pragma once
#include <DrakePinD.hpp>
#include <CANLibrary.h>
#include <drivers/BMSAnt_Data.h>

extern CAN_HandleTypeDef hcan;
extern void HAL_CAN_Send(uint16_t id, uint8_t *data_raw, uint8_t length_raw);

namespace CANLib
{
	static constexpr uint8_t CFG_CANObjectsCount = 22;
	static constexpr uint8_t CFG_CANFrameBufferSize = 16;
	static constexpr uint16_t CFG_CANFirstId = 0x0180;
	
	DrakePinD can_rs({GPIOA, GPIO_PIN_15}, DrakePin::OutputOpenDrain, DrakePin::High);
	
	CANManager<CFG_CANObjectsCount, CFG_CANFrameBufferSize> can_manager(&HAL_CAN_Send);
	
	CANObject<uint8_t,  7> obj_block_info(CFG_CANFirstId + 0);
	CANObject<uint8_t,  7> obj_block_health(CFG_CANFirstId + 1);
	CANObject<uint8_t,  7> obj_block_features(CFG_CANFirstId + 2);
	CANObject<uint8_t,  7> obj_block_error(CFG_CANFirstId + 3);

	CANObject<int16_t,  1> obj_high_current_1(CFG_CANFirstId + 4, 1000);
	CANObject<int16_t,  1> obj_high_current_2(CFG_CANFirstId + 5, 1000);
	CANObject<uint8_t,  1> obj_battery_percent_1(CFG_CANFirstId + 6, 10000);
	CANObject<uint8_t,  1> obj_battery_percent_2(CFG_CANFirstId + 7, 10000);
	CANObject<int16_t,  1> obj_battery_power_1(CFG_CANFirstId + 8, 250);
	CANObject<int16_t,  1> obj_battery_power_2(CFG_CANFirstId + 9, 250);
	CANObject<int8_t,   1> obj_battery_state_1(CFG_CANFirstId + 10, CAN_TIMER_DISABLED, 300);
	CANObject<int8_t,   1> obj_battery_state_2(CFG_CANFirstId + 11, CAN_TIMER_DISABLED, 300);
	CANObject<uint16_t, 1> obj_high_voltage_1(CFG_CANFirstId + 12, 1000);
	CANObject<uint16_t, 1> obj_high_voltage_2(CFG_CANFirstId + 13, 1000);
	CANObject<uint16_t, 3> obj_low_voltage_min_max_delta_1(CFG_CANFirstId + 14, 5000, 300);
	CANObject<uint16_t, 3> obj_low_voltage_min_max_delta_2(CFG_CANFirstId + 15, 5000, 300);
	CANObject<uint16_t, 2> obj_low_voltage_batt_1(CFG_CANFirstId + 16);
	CANObject<uint16_t, 2> obj_low_voltage_batt_2(CFG_CANFirstId + 17);
	CANObject<int8_t,   1> obj_max_temperature_1(CFG_CANFirstId + 18, 5000, 300);
	CANObject<int8_t,   1> obj_max_temperature_2(CFG_CANFirstId + 19, 5000, 300);
	CANObject<int8_t,   2> obj_temperature_1(CFG_CANFirstId + 20);
	CANObject<int8_t,   2> obj_temperature_2(CFG_CANFirstId + 21);
	
	
	void CAN_Enable()
	{
		HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE);
		HAL_CAN_Start(&hcan);
		
		can_rs.Off();
		
		return;
	}
	
	void CAN_Disable()
	{
		HAL_CAN_DeactivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE);
		HAL_CAN_Stop(&hcan);
		
		can_rs.On();
		
		return;
	}
	
	inline void Setup()
	{
		can_rs.Init();
		
		set_block_info_params(obj_block_info);
		set_block_health_params(obj_block_health);
		set_block_features_params(obj_block_features);
		set_block_error_params(obj_block_error);
		
		can_manager.RegisterObject(obj_block_info);
		can_manager.RegisterObject(obj_block_health);
		can_manager.RegisterObject(obj_block_features);
		can_manager.RegisterObject(obj_block_error);

		can_manager.RegisterObject(obj_high_current_1);
		can_manager.RegisterObject(obj_high_current_2);
		can_manager.RegisterObject(obj_battery_percent_1);
		can_manager.RegisterObject(obj_battery_percent_2);
		can_manager.RegisterObject(obj_battery_power_1);
		can_manager.RegisterObject(obj_battery_power_2);
		can_manager.RegisterObject(obj_battery_state_1);
		can_manager.RegisterObject(obj_battery_state_2);
		can_manager.RegisterObject(obj_high_voltage_1);
		can_manager.RegisterObject(obj_high_voltage_2);
		can_manager.RegisterObject(obj_low_voltage_min_max_delta_1);
		can_manager.RegisterObject(obj_low_voltage_min_max_delta_2);
		can_manager.RegisterObject(obj_low_voltage_batt_1);
		can_manager.RegisterObject(obj_low_voltage_batt_2);
		can_manager.RegisterObject(obj_max_temperature_1);
		can_manager.RegisterObject(obj_max_temperature_2);
		can_manager.RegisterObject(obj_temperature_1);
		can_manager.RegisterObject(obj_temperature_2);
		
		
		// Передача версий и типов в объект block_info
		obj_block_info.SetValue(0, (About::board_type << 3 | About::board_ver), CAN_TIMER_TYPE_NORMAL);
		obj_block_info.SetValue(1, (About::soft_ver << 2 | About::can_ver), CAN_TIMER_TYPE_NORMAL);
		
		CAN_Enable();
		
		return;
	}
	
	inline void Loop(uint32_t &current_time)
	{
		can_manager.Process(current_time);

		// Передача UpTime блока в объект block_info
		static uint32_t iter1000 = 0;
		if(current_time - iter1000 > 1000)
		{
			iter1000 = current_time;
			
			uint8_t *data = (uint8_t *)&current_time;
			obj_block_info.SetValue(2, data[0], CAN_TIMER_TYPE_NORMAL);
			obj_block_info.SetValue(3, data[1], CAN_TIMER_TYPE_NORMAL);
			obj_block_info.SetValue(4, data[2], CAN_TIMER_TYPE_NORMAL);
			obj_block_info.SetValue(5, data[3], CAN_TIMER_TYPE_NORMAL);
		}
		
		// При выходе обновляем время
		current_time = HAL_GetTick();
		
		return;
	}

	void UpdateMaxTemperature()
	{
		// 0x0046	MaxTemperature
		// request | timer:5000 | event
		// int8_t	°C	1 + 1	{ type[0] data[1] }
		// Ограничения: <60:WARN, <80: CRIT
		// Максимально зафиксированная температура.
		int8_t max_temp = 0;
		int8_t curr_temp = 0;

		for (uint8_t i = 0; i < 7; i++)
		{
			curr_temp = obj_temperature_1.GetValue(i);
			if (curr_temp > max_temp)
				max_temp = curr_temp;
		}
		for (uint8_t i = 0; i < 7; i++)
		{
			curr_temp = obj_temperature_2.GetValue(i);
			if (curr_temp > max_temp)
				max_temp = curr_temp;
		}
		for (uint8_t i = 0; i < 7; i++)
		{
			//curr_temp = obj_temperature_3.GetValue(i);
			if (curr_temp > max_temp)
				max_temp = curr_temp;
		}

		timer_type_t timer_type = CAN_TIMER_TYPE_NORMAL;
		event_type_t event_type = CAN_EVENT_TYPE_NONE;

		if (max_temp >= 60 && max_temp < 80)
		{
			timer_type = CAN_TIMER_TYPE_WARNING;
		}
		else if (max_temp >= 80)
		{
			timer_type = CAN_TIMER_TYPE_CRITICAL;
		}

		// TODO: надо ещё обсудить по event_type и его присваивать
		//obj_max_temperature.SetValue(0, max_temp, timer_type, event_type);
	}

	
}
