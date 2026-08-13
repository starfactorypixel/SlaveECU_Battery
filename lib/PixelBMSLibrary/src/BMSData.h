#pragma once
#include <inttypes.h>

namespace BMSManagerData
{
	struct common_data_t
	{
		int16_t current_pwr;		// Ток АКБ, 0.1A
		uint8_t percent;			// % заряда АКБ
		int16_t power_pwr;			// Протекающая мощность АКБ, Wt
		uint16_t voltage_pwr;		// Напряжение АКБ, 0.1V
		uint16_t cell_vmin_volt;	// Минимальное напряжение в ячейках, 0.001V
		uint16_t cell_vmax_volt;	// Максимальное напряжение в ячейках, 0.001V
		uint16_t cell_delta_volt;	// Разница напряжение в ячейках, 0.001V
		uint16_t cell_voltage[32];	// Напряжение на ячейках АКБ, 0.001V
	};

};
