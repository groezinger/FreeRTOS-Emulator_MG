#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"
#include "TUM_Font.h"

#include "AsyncIO.h"
#include "buttons.h"

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR
static buttons_buffer_t buttons = { 0 };
void xGetButtonInput()
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

int buttonLockInit(){
    buttons.lock = xSemaphoreCreateMutex(); // Locking mechanism
    if (!buttons.lock) {
        return 0;
    }
    else {
        return 1;
    }
}

void buttonLockExit(){
    vSemaphoreDelete(buttons.lock);
}

bool debounceButton(my_button_t* my_button, int reading){
    bool return_value = false;
    if (reading != my_button->last_button_state){
        my_button->last_debounce_time = xTaskGetTickCount();
    }
    if((xTaskGetTickCount() - my_button->last_debounce_time)>100){
        if(reading != my_button->button_state){
            my_button->button_state = reading;
            if(my_button->button_state){
                return_value = true;
            }
        }
    }
    my_button->last_button_state = reading;
    return return_value;
}

void evaluateButtons(my_button_t* my_used_buttons){
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        if(debounceButton(&my_used_buttons[KEYBOARD_A], buttons.buttons[KEYCODE(A)])){
            my_used_buttons[KEYBOARD_A].counter +=1;
        }
        if(debounceButton(my_used_buttons+KEYBOARD_B, buttons.buttons[KEYCODE(B)])){
            my_used_buttons[KEYBOARD_B].counter +=1;
        }
        if(debounceButton(my_used_buttons+KEYBOARD_C, buttons.buttons[KEYCODE(C)])){
            my_used_buttons[KEYBOARD_C].counter +=1;
        }
        if(debounceButton(my_used_buttons+KEYBOARD_D, buttons.buttons[KEYCODE(D)])){
            my_used_buttons[KEYBOARD_D].counter +=1;
        }
        if(debounceButton(my_used_buttons+KEYBOARD_Q, buttons.buttons[KEYCODE(Q)])){
            exit(EXIT_SUCCESS);
        }      
        xSemaphoreGive(buttons.lock);         
    }
    if(debounceButton(&my_used_buttons[MOUSE_LEFT], tumEventGetMouseLeft())
                || debounceButton(&my_used_buttons[MOUSE_MIDDLE], tumEventGetMouseMiddle()) 
                ||  debounceButton(&my_used_buttons[MOUSE_RIGHT], tumEventGetMouseRight())){
        my_used_buttons[KEYBOARD_A].counter = 0;
        my_used_buttons[KEYBOARD_B].counter = 0;
        my_used_buttons[KEYBOARD_C].counter = 0;
        my_used_buttons[KEYBOARD_D].counter = 0;
    }

}
