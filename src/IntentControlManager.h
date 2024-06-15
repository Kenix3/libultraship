#include <libultraship/libultra/controller.h>
#include <map>

class IntentControlManager {
    public: 
    IntentControls* intentControls = new IntentControls();

    std::map<uint16_t, uint8_t> pendingStates = std::map<uint16_t, uint8_t>();

    std::map<uint16_t, uint8_t> curStates = std::map<uint16_t, uint8_t>();
    std::map<uint16_t, uint8_t> prevStates = std::map<uint16_t, uint8_t>();
    std::map<uint16_t, uint8_t> pressStates = std::map<uint16_t, uint8_t>();
    std::map<uint16_t, uint8_t> relStates = std::map<uint16_t, uint8_t>();

    IntentControlManager(){
        intentControls->userData = this;
        intentControls->checkIntentButton = +[](void* userData, uint16_t intentId, uint8_t stateVersionId){
            return ((IntentControlManager*) userData)->checkIntentButton(intentId, stateVersionId);
        };

        intentControls->registerButtonState = [](void* userData, uint16_t intentId, uint8_t pressed){
            return ((IntentControlManager*) userData)->registerButtonState(intentId, pressed);
        };

        intentControls->updateState = +[](void* userData){
            ((IntentControlManager*) userData)->updateState();
        };
    }

    uint8_t checkIntentButton(uint16_t intentId, uint8_t stateVersionId){
        std::map<uint16_t, uint8_t>* stateVersion = nullptr;
        switch (stateVersionId)
        {
        case BUTTON_STATE_CUR:
            stateVersion = &this->curStates;
            break;
        case BUTTON_STATE_PREV:
            stateVersion = &this->prevStates;
            break;
        case BUTTON_STATE_PRESS:
            stateVersion = &this->pressStates;
            break;
        case BUTTON_STATE_REL:
            stateVersion = &this->relStates;
            break;
        
        default:
            return 0;
        }

        stateVersion->try_emplace(intentId, 0);
        return stateVersion->at(intentId);
    }

    void registerButtonState(uint16_t intentId, uint8_t pressed){
        pendingStates.insert_or_assign(intentId, pressed);
    }

    void updateState(){
        this->prevStates.clear();
        for(auto s : this->curStates){
            prevStates.insert_or_assign(s.first, s.second);
        }

        this->curStates.clear();
        for(auto s : this->pendingStates){
            curStates.insert_or_assign(s.first, s.second);
        }
    
        for(auto s : this->prevStates){
            this->curStates.try_emplace(s.first, 0);
            uint8_t cur = this->curStates.at(s.first);
            uint8_t diff = !cur != !s.second;

            this->pressStates.insert_or_assign(s.first, (diff && cur));
            this->relStates.insert_or_assign(s.first, (diff && s.second));
        }
    }

    ~IntentControlManager(){

    }
};