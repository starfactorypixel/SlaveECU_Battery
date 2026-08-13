#pragma once
#include <CANLibrary.h>
#include "CanObj/CanBlockInfo.hpp"
#include "CanObj/CanBlockCfg.hpp"
#include "CanObj/CanInfoParam.hpp"
#include "CanObj/CanStreamObj.hpp"
#include "CANFunc.h"
#include <DrakePinD.hpp>

extern CAN_HandleTypeDef hcan;
extern bool HAL_CAN_Send(can_object_id_t id, uint8_t *data, uint8_t length);

namespace CANLib
{
	static constexpr uint8_t CFG_CANObjectsCount = 22;
	static constexpr uint16_t CAN_BASE_ID = 0x0180;
	
	DrakePinD can_rs({GPIOA, GPIO_PIN_15}, DrakePin::OutputOpenDrain, DrakePin::High);





	CANManager<CFG_CANObjectsCount> can_manager(&HAL_CAN_Send, &HAL_GetTick, &OnInterruptCtrl);
	
	CanBlockInfo obj_block_info(CAN_BASE_ID+0, OnStaticInfoReq, OnDynamicInfoReq);
	CanBlockCfg obj_block_cfg(CAN_BASE_ID+1, OnCfgSaveReset, block_cfg_table, block_cfg_table_count);

	auto &data1 = BMSLogic::Bms.common_obj[BMSLogic::BMS_1].data;
	auto &data2 = BMSLogic::Bms.common_obj[BMSLogic::BMS_2].data;
	
	CanInfoParam<int16_t>  obj_high_current_1(CAN_BASE_ID+4, 1000, &data1.current_pwr);
	CanInfoParam<int16_t>  obj_high_current_2(CAN_BASE_ID+5, 1000, &data2.current_pwr);
	CanInfoParam<uint8_t>  obj_battery_percent_1(CAN_BASE_ID+6, 10000, &data1.percent);
	CanInfoParam<uint8_t>  obj_battery_percent_2(CAN_BASE_ID+7, 10000, &data2.percent);

	CanInfoParam<int16_t>  obj_battery_power_1(CAN_BASE_ID+8, 250, &data1.power_pwr);
	CanInfoParam<int16_t>  obj_battery_power_2(CAN_BASE_ID+9, 250, &data2.power_pwr);
	//CanInfoParam<int8_t>   obj_battery_state_1(CAN_BASE_ID+10, CAN_TIMER_DISABLED, 300);
	//CanInfoParam<int8_t>   obj_battery_state_2(CAN_BASE_ID+11, CAN_TIMER_DISABLED, 300);
	CanInfoParam<uint16_t> obj_high_voltage_1(CAN_BASE_ID+12, 1000, &data1.voltage_pwr);
	CanInfoParam<uint16_t> obj_high_voltage_2(CAN_BASE_ID+13, 1000, &data2.voltage_pwr);
	CanInfoParam<uint16_t, 3> obj_low_voltage_min_max_delta_1(CAN_BASE_ID+14, 5000, &data1.cell_vmin_volt, &data1.cell_vmax_volt, &data1.cell_delta_volt);
	CanInfoParam<uint16_t, 3> obj_low_voltage_min_max_delta_2(CAN_BASE_ID+15, 5000, &data2.cell_vmin_volt, &data2.cell_vmax_volt, &data2.cell_delta_volt);
	CanStreamObj<uint16_t> obj_low_voltage_batt_1(CAN_BASE_ID+16, data1.cell_voltage, BMSANT::CellsNumber);
	CanStreamObj<uint16_t> obj_low_voltage_batt_2(CAN_BASE_ID+17, data2.cell_voltage, BMSANT::CellsNumber);
	CanInfoParam<int8_t>   obj_max_temperature_1(CAN_BASE_ID+18, 5000, &Temp::max[0]);
	CanInfoParam<int8_t>   obj_max_temperature_2(CAN_BASE_ID+19, 5000, &Temp::max[1]);
	CanStreamObj<int8_t>   obj_temperature_1(CAN_BASE_ID+20, Temp::temperatures[0], Temp::TEMP_TOTAL_COUNT);
	CanStreamObj<int8_t>   obj_temperature_2(CAN_BASE_ID+21, Temp::temperatures[1], Temp::TEMP_TOTAL_COUNT);
	
	
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

	static inline uint8_t BatteryPercentClassifier(uint8_t idx, uint8_t value)
	{
		if(value <= 20) return CAN_FUNC_TIMER_CRITICAL;
		if(value <= 50) return CAN_FUNC_TIMER_WARNING;
		return CAN_FUNC_TIMER_NORMAL;
	}

	static inline uint8_t CellMinMaxDeltaClassifier(uint8_t idx, uint16_t value)
	{
		switch(idx)
		{
			case 2:
			{
				if(value >= 100) return CAN_FUNC_TIMER_CRITICAL;
				if(value >= 50) return CAN_FUNC_TIMER_WARNING;
				return CAN_FUNC_TIMER_NORMAL;
			}
		}
		return CAN_FUNC_TIMER_NORMAL;
	}

	static inline uint8_t TemperatureClassifier(uint8_t idx, int8_t value)
	{
		if(value >= 80) return CAN_FUNC_TIMER_CRITICAL;
		if(value >= 60) return CAN_FUNC_TIMER_WARNING;
		return CAN_FUNC_TIMER_NORMAL;
	}
	
	
	inline void Setup()
	{
		can_rs.Init();

		obj_battery_percent_1.SetValueClassifier(BatteryPercentClassifier);
		obj_battery_percent_2.SetValueClassifier(BatteryPercentClassifier);
		obj_low_voltage_min_max_delta_1.SetValueClassifier(CellMinMaxDeltaClassifier);
		obj_low_voltage_min_max_delta_2.SetValueClassifier(CellMinMaxDeltaClassifier);
		obj_max_temperature_1.SetValueClassifier(TemperatureClassifier);
		obj_max_temperature_2.SetValueClassifier(TemperatureClassifier);

		can_manager.AddObject(obj_block_info);
		can_manager.AddObject(obj_block_cfg);
		can_manager.AddObject(obj_high_current_1);
		can_manager.AddObject(obj_high_current_2);
		can_manager.AddObject(obj_battery_percent_1);
		can_manager.AddObject(obj_battery_percent_2);
		can_manager.AddObject(obj_battery_power_1);
		can_manager.AddObject(obj_battery_power_2);
		//can_manager.AddObject(obj_battery_state_1);
		//can_manager.AddObject(obj_battery_state_2);
		can_manager.AddObject(obj_high_voltage_1);
		can_manager.AddObject(obj_high_voltage_2);
		can_manager.AddObject(obj_low_voltage_min_max_delta_1);
		can_manager.AddObject(obj_low_voltage_min_max_delta_2);
		can_manager.AddObject(obj_low_voltage_batt_1);
		can_manager.AddObject(obj_low_voltage_batt_2);
		can_manager.AddObject(obj_max_temperature_1);
		can_manager.AddObject(obj_max_temperature_2);
		can_manager.AddObject(obj_temperature_1);
		can_manager.AddObject(obj_temperature_2);
		
		CAN_Enable();
		
		return;
	}
	
	inline void Loop(uint32_t &current_time)
	{
		can_manager.Processing();
		
		current_time = HAL_GetTick();
		return;
	}
}

IBlockInfoSender &BlockInfoSender = CANLib::obj_block_info;
