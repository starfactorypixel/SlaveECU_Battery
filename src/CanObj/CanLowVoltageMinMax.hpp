#pragma once
#include <inttypes.h>
#include <CanObjectBase.h>

// Данный объект реализует информационный параметр, который отправляется по request, timer, event

class CanLowVoltageMinMax : public CANObjectBase
{
    struct __attribute__((packed)) request_t { uint8_t fId; };
    struct __attribute__((packed)) timer_t { uint8_t fId = CAN_FUNC_TIMER_NORMAL; uint16_t val[2]; uint8_t num[2]; };
    struct __attribute__((packed)) event_ok_t { uint8_t fId = CAN_FUNC_EVENT_OK; uint16_t val[2]; uint8_t num[2]; };
    
    using function_classifier_t = uint8_t (*)(uint8_t idx, uint16_t value);
    
    public:

        CanLowVoltageMinMax(can_object_id_t id, uint16_t timer, const uint16_t *val0, const uint16_t *val1, const uint8_t *num0, const uint8_t *num1) : CANObjectBase(id), _value_ptr{val0, val1}, _number_ptr{num0, num1}
        {           
            this->SetTimerPeriod(timer);
            
            return;
        }

        void SetValueClassifier(function_classifier_t function)
        {
            _FuncClassifier = function;
            
            return;
        }
        
    protected:
        virtual void OnTick(uint32_t time) noexcept override
        {
            return;
        }

        virtual void OnProcessFrame(can_frame_t &can_frame) noexcept override
        {
            uint8_t fId = can_frame.raw_data[0];
            switch(fId)
            {
                case CAN_FUNC_REQUEST_IN:
                {
                    event_ok_t answer = {};
                    answer.val[0] = *_value_ptr[0];
                    answer.val[1] = *_value_ptr[1];
                    answer.num[0] = *_number_ptr[0];
                    answer.num[1] = *_number_ptr[1];
                    this->SendFrame((uint8_t *)&answer, sizeof(answer));
                    
                    break;
                }
            }

            return;
        }

        virtual void OnTimer() noexcept override
        {
            timer_t answer = {};
            
            for(uint8_t i = 0; i < 2; ++i)
            {
                const uint16_t val = *_value_ptr[i];
                uint8_t fId = _FuncClassifier(i, val);
                if(fId > answer.fId)
                    answer.fId = fId;
                answer.val[i] = val;
                answer.num[i] = *_number_ptr[i];
            }
            this->SendFrame((uint8_t *)&answer, sizeof(answer));

            return;
        }
        
    private:
        const uint16_t *_value_ptr[2];
        const uint8_t *_number_ptr[2];
        function_classifier_t _FuncClassifier = [](uint8_t idx, uint16_t value) -> uint8_t { return CAN_FUNC_TIMER_NORMAL; };
};
