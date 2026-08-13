#pragma once
#include <inttypes.h>
#include <CUtils.h>
#include <BMSDeviceInterface.h>
#include <drivers/BMSAnt_Data.h>

class BMSAnt : public BMSDeviceInterface
{
	static constexpr uint8_t REQUEST_TRY_MAX = 3;
	using callback_ready_t = void (*)(const BMSANT::packet_raw_reverse_t *data);
	
	public:
		
		BMSAnt() : BMSDeviceInterface(), _callback_ready(nullptr), _busy(false), _ready(false), _request_try_count(0), _last_request_time(0)
		{
			memset(_data, 0x00, sizeof(_data));
			
			return;
		}
		
		void SetReadyCallback(callback_ready_t ready)
		{
			_callback_ready = ready;
			
			return;
		}
		
		virtual void Init() override
		{

		}
		
		virtual void Tick(uint32_t &time) override
		{
			if(_ready == true)
			{
				_ready = false;
			
				// Т.к. данные представленны в формате big-endian, обращаем массив для просторы работы
				array_reverse(_data, sizeof(_data));

				_PostProcessing();

				{
					// Напихиваем полезные данные в общий объект
					BMSManagerData::common_data_t *common_data = &_manager->common_obj[_idx].data;
					
					common_data->current_pwr = data->total_current;
					common_data->percent = data->capacity_percent;
					common_data->power_pwr = data->total_power;
					common_data->voltage_pwr = data->total_voltage;
					common_data->cell_vmin_volt = data->cell_vmin_volt;
					common_data->cell_vmax_volt = data->cell_vmax_volt;
					common_data->cell_delta_volt = data->cell_vmax_volt - data->cell_vmin_volt;
					memcpy(common_data->cell_voltage, data->cell_voltage, BMSANT::CellsNumber);
					
					if(data->status_charge_fet > 1 || data->status_dcharge_fet > 1)
					{
						_error.code = ERROR_CTRL;
					}
					
					if(_callback_ready != nullptr)
					{
						_callback_ready(data);
					}
				}

				_busy = false;
				_request_try_count = 0;
			}
			
			if(time - _last_request_time > BMSANT::PacketRequestInerval)
			{
				_last_request_time = time;

				_manager->DataTx(_idx, BMSANT::PacketRequest, sizeof(BMSANT::PacketRequest));

				if(_request_try_count <= REQUEST_TRY_MAX) _request_try_count++;
			}

			if(_request_try_count > REQUEST_TRY_MAX)
			{
				_manager->ResetCommonData(_idx);
				_error.code = ERROR_LOST;
			}
			
			return;
		}
		
		// Приём пакета, в прерывании. Минимум самый важный действий.
		virtual void DataRx(const uint8_t *raw, const uint8_t length) override
		{
			_error.code = ERROR_NONE;
			
			if(_busy == true){ _error.code = ERROR_BUSY; return; }
			if(length != BMSANT::PacketSize){ _error.code = ERROR_LENGTH; return; }
			if(memcmp(BMSANT::PacketHeader, raw, sizeof(BMSANT::PacketHeader)) != 0){ _error.code = ERROR_HEADER; return; }
			if(_CheckCRCSum(raw) == false){ _error.code = ERROR_CRC; return; }
			
			memcpy(_data, raw, sizeof(_data));
			
			_ready = true;
			_busy = true;
			
			return;
		}
		
		// Объект готовых данных.
		const BMSANT::packet_raw_reverse_t *data = (BMSANT::packet_raw_reverse_t *)_data;
		
	private:
		
		bool _CheckCRCSum(const uint8_t *array)
		{
			uint16_t crc = 0x0000;
			
			for(uint8_t i = 4; i < 138; ++i) { crc += array[i]; }
			
			return ( ((crc >> 8) & 0xFF) == array[138] && (crc & 0xFF) == array[139] );
		}
		
		void _PostProcessing()
		{
			BMSANT::packet_raw_reverse_t *data = (BMSANT::packet_raw_reverse_t *) _data;
			
			array_reverse(data->cell_voltage, BMSANT::CellsNumber);
			array_reverse(data->temperature, BMSANT::TempsNumber);
			
			return;
		}
		
		callback_ready_t _callback_ready;
		
		bool _busy;								// Флаг того, что разбор данных не окончен и новые копировать нельзя
		bool _ready;							// Флаг того, что массив данные приняты и готовы к анализу
		uint8_t _data[BMSANT::PacketSize];		// Холодный массив данных (Работа в программе)
		uint8_t _request_try_count;				// Кол-во попыток запроса данных

		uint32_t _last_request_time;
		
};
