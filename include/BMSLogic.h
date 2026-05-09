#pragma once
#include <BMSManager.h>
#include <drivers/BMSAnt.h>
#include <drivers/BMSOther.h>
#include <DrakePinD.hpp>
#include <CUtils.h>

extern UART_HandleTypeDef hBms1Uart;
extern UART_HandleTypeDef hBms2Uart;

namespace BMSLogic
{
	
	void BMS_UART_TX(uint8_t idx, const uint8_t *raw, const uint16_t length);
	void BMS_ERROR(uint8_t idx, int8_t code);
	
	BMSAnt Ant1;
	BMSAnt Ant2;
	BMSManager Bms(BMS_UART_TX, BMS_ERROR);

	DrakePinD Bms1En({GPIOA, GPIO_PIN_1}, DrakePin::Output, DrakePin::Low);
	DrakePinD Bms2En({GPIOB, GPIO_PIN_3}, DrakePin::Output, DrakePin::Low);


	void OnLowVoltageBatt1Stream(auto coll_el, uint8_t idx);
	void OnLowVoltageBatt2Stream(auto coll_el, uint8_t idx);

	CollectionStream<uint16_t> LowVoltageBatt1Stream(Ant1.data->cell_voltage, BMSANT::CellsNumber, OnLowVoltageBatt1Stream);
	CollectionStream<uint16_t> LowVoltageBatt2Stream(Ant2.data->cell_voltage, BMSANT::CellsNumber, OnLowVoltageBatt2Stream);
	
	struct data_t
	{
		UART_HandleTypeDef *hal;		// Указатель на объект HAL USART
		uint8_t hot[200];				// Горячий массив данных (Работа в прерывании)
	} uart_data[2];
	
	enum bms_num_t : uint8_t { BMS_1 = 0, BMS_2 = 1 };
	
	
	inline void UART_RX(uint8_t idx, const uint16_t length)
	{
		Bms.DataRx(idx, uart_data[idx].hot, length);
		
		return;
	}
	
	void BMS_UART_TX(uint8_t idx, const uint8_t *raw, const uint16_t length)
	{
		HAL_UART_Transmit(uart_data[idx].hal, (uint8_t *)raw, length, 64U);
		
		return;
	}
	
	void BMS_ERROR(uint8_t idx, int8_t code)
	{
		DEBUG_LOG_TOPIC("BMS-ERR", "idx: %d, code: %d\n", idx, code);

		return;
	}


	void UpdateCANObjects_BMS1(const BMSANT::packet_raw_reverse_t *data)
	{
		timer_type_t timer_type = CAN_TIMER_TYPE_NORMAL;
		event_type_t event_type = CAN_EVENT_TYPE_NONE;



		CANLib::obj_high_current_1.SetValue(0, data->total_current, CAN_TIMER_TYPE_NORMAL);


		timer_type = CAN_TIMER_TYPE_NORMAL;
		if (data->capacity_percent < 15)
		{
			timer_type = CAN_TIMER_TYPE_CRITICAL;
		}
		else if (data->capacity_percent < 30)
		{
			timer_type = CAN_TIMER_TYPE_WARNING;
		}
		CANLib::obj_battery_percent_1.SetValue(0, data->capacity_percent, timer_type);



		CANLib::obj_battery_power_1.SetValue(0, data->total_power, CAN_TIMER_TYPE_NORMAL);
		
		//CANLib::obj_battery_state_1.SetValue(0, data->, CAN_TIMER_TYPE_NORMAL);
		
		CANLib::obj_high_voltage_1.SetValue(0, data->total_voltage, CAN_TIMER_TYPE_NORMAL);

		

		timer_type = CAN_TIMER_TYPE_NORMAL;
		event_type = CAN_EVENT_TYPE_NONE;
		if(data->cell_vmin_volt >= 2700 && data->cell_vmin_volt <= 3000)
		{
			timer_type = CAN_TIMER_TYPE_WARNING;
		}
		else if(data->cell_vmin_volt < 2700 || data->cell_vmax_volt > 4200)
		{
			timer_type = CAN_TIMER_TYPE_CRITICAL;
			event_type = CAN_EVENT_TYPE_ERROR;
		}
		CANLib::obj_low_voltage_min_max_delta_1.SetValue(0, data->cell_vmin_volt, timer_type, event_type);
		CANLib::obj_low_voltage_min_max_delta_1.SetValue(1, data->cell_vmax_volt, timer_type, event_type);
		CANLib::obj_low_voltage_min_max_delta_1.SetValue(2, (data->cell_vmax_volt - data->cell_vmin_volt), timer_type, event_type);

		Temp::PutFromBms(0, data->temperature, BMSANT::TempsNumber);



		//UpdateMaxTemperature();
	}
	
	void OnLowVoltageBatt1Stream(auto coll_el, uint8_t idx)
	{
		CANLib::obj_low_voltage_batt_1.SetValue(0, (idx + 1), CAN_TIMER_TYPE_NONE, CAN_EVENT_TYPE_NORMAL);
		CANLib::obj_low_voltage_batt_1.SetValue(1, coll_el, CAN_TIMER_TYPE_NONE, CAN_EVENT_TYPE_NORMAL);
		
		return;
	}
	
	void OnLowVoltageBatt2Stream(auto coll_el, uint8_t idx)
	{
		CANLib::obj_low_voltage_batt_2.SetValue(0, (idx + 1), CAN_TIMER_TYPE_NONE, CAN_EVENT_TYPE_NORMAL);
		CANLib::obj_low_voltage_batt_2.SetValue(1, coll_el, CAN_TIMER_TYPE_NONE, CAN_EVENT_TYPE_NORMAL);

		return;
	}
	
	
	inline void Setup()
	{
		Bms1En.Init();
		Bms2En.Init();
		// Реализовать управление Bms1En Bms2En

		memset(uart_data, 0x00, sizeof(uart_data));
		
		uart_data[BMS_1].hal = &hBms1Uart;
		uart_data[BMS_2].hal = &hBms2Uart;
		
		Ant1.SetReadyCallback( UpdateCANObjects_BMS1 );
		//Ant2.SetReadyCallback();

		Bms.SetModel(BMS_1, Ant1);
		Bms.SetModel(BMS_2, Ant2);

		CANLib::obj_low_voltage_batt_1.RegisterFunctionRequest([](can_frame_t &can_frame, can_error_t &error) -> can_result_t
		{
			LowVoltageBatt1Stream.Start(100);
			
			return CAN_RESULT_IGNORE;
		});
		CANLib::obj_low_voltage_batt_2.RegisterFunctionRequest([](can_frame_t &can_frame, can_error_t &error) -> can_result_t
		{
			LowVoltageBatt2Stream.Start(100);
			
			return CAN_RESULT_IGNORE;
		});

		
		HAL_UARTEx_ReceiveToIdle_IT(uart_data[BMS_1].hal, uart_data[BMS_1].hot, sizeof(uart_data[BMS_1].hot));
		HAL_UARTEx_ReceiveToIdle_IT(uart_data[BMS_2].hal, uart_data[BMS_2].hot, sizeof(uart_data[BMS_2].hot));

		Bms1En.On();
		Bms2En.On();
		
		return;
	}
	
	inline void Loop(uint32_t &current_time)
	{
		Bms.Tick(current_time);

		LowVoltageBatt1Stream.Processing(current_time);
		LowVoltageBatt2Stream.Processing(current_time);
		
		static uint32_t tick500 = 0;
		if(current_time - tick500 > 500)
		{
			tick500 = current_time;
/*
			DEBUG_LOG_TOPIC("BMS1", "vol: %d, cur: %d, pow: %d\n", Bms.data[0].voltage, Bms.data[0].current, Bms.data[0].power);
			DEBUG_LOG_TOPIC("BMS2", "vol: %d, cur: %d, pow: %d\n", Bms.data[1].voltage, Bms.data[1].current, Bms.data[1].power);
			DEBUG_LOG_ARRAY_HEX("BMS1-hex", (uint8_t *)Ant1.data, 140);
			DEBUG_LOG_NEW_LINE();
*/
		}
					//CANLib::UpdateCANObjects_BMS(obj->cold);




		current_time = HAL_GetTick();
		
		return;
	}
}
