#pragma once
#include <inttypes.h>

class BMSManager;

class BMSDeviceInterface
{
	public:
		
		enum error_code_t : int8_t
		{
			ERROR_NONE = 0,			// Ошибок нет
			ERROR_BUSY = -1,		// Новый пакет пришёл раньше чем данные были обработаны
			ERROR_LENGTH = -2,		// Новый пакет не верной длинны
			ERROR_HEADER = -3,		// Новый пакет имеет неверный заголовок
			ERROR_CRC = -4,			// Новый пакет не прошёл проверку CRC
			//ERROR_AUTH = -5,		// Пакет авторизации не соотвествует BMS
			ERROR_CTRL = 1,			// Получен код ошибки от BMS
			ERROR_LOST = 2,			// Потеря связи с BMS
		};
		
		BMSDeviceInterface() : _initiated(false), _manager(nullptr), _idx(0) {}
		
		virtual void Init() = 0;
		virtual void Tick(uint32_t &time) = 0;
		virtual void DataRx(const uint8_t *raw, const uint8_t length) = 0;
		
		bool IsInitiated()
		{
			return _initiated;
		}
		
		void PrepareInit(BMSManager *obj, uint8_t idx)
		{
			_initiated = true;
			_manager = obj;
			_idx = idx;
		}
		
		bool GetNewError(error_code_t &code)
		{
			code = _error.code;
			if(code != _error.prev)
			{
				_error.prev = code;
				return true;
			}
			
			return false;
		}
	
	protected:
		
		bool _initiated;
		BMSManager *_manager;
		uint8_t _idx;

		struct error_t
		{
			volatile error_code_t code = ERROR_NONE;
			error_code_t prev = ERROR_NONE;
		} _error;
		
	private:
		
};
