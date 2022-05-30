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

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)
#define PI 3.141

static TaskHandle_t DemoTask = NULL;

void vDemoTask(void *pvParameters)
{   
    static double rotatingvalue = 0.0;
    static char my_string[100];
    char my_moving_string[100];
    static int my_string_width = 0;
    static int my_moving_string_width = 0;
    static my_button_t my_used_buttons[8];
    int mouse_x;
    int mouse_y;
    extern SDL_Window *window;

    for(int i=0; i< sizeof(my_used_buttons)/ sizeof(my_button_t); i++){
        my_used_buttons[i].last_debounce_time=0;
        my_used_buttons[i].counter=0;
        my_used_buttons[i].button_state=false;
        my_used_buttons[i].last_button_state=false;
    }

    coord_t triangle_coords[3];
    triangle_coords[0].x = SCREEN_WIDTH*12/24;
    triangle_coords[0].y = SCREEN_HEIGHT*11/24;
    triangle_coords[1].x = SCREEN_WIDTH*11/24;
    triangle_coords[1].y = SCREEN_HEIGHT*13/24;
    triangle_coords[2].x = SCREEN_WIDTH*13/24;
    triangle_coords[2].y = SCREEN_HEIGHT*13/24;


    // Needed such that Gfx library knows which thread controlls drawing
    // Only one thread can call tumDrawUpdateScreen while and thread can call
    // the drawing functions to draw objects. This is a limitation of the SDL
    // backend.
    tumDrawBindThread();

    while (1) {
        tumEventFetchEvents(FETCH_EVENT_NONBLOCK); // Query events backend for new events, ie. button presses
        xGetButtonInput(); // Update global input
        // `buttons` is a global shared variable and as such needs to be
        // guarded with a mutex, mutex must be obtained before accessing the
        // resource and given back when you're finished. If the mutex is not
        // given back then no other task can access the reseource.
        evaluateButtons(my_used_buttons); //evaluate Pressed Buttons
        SDL_GetGlobalMouseState(&mouse_x,&mouse_y); // get current mouse cursor position
        tumDrawClear(White); // Clear screen

        sprintf(my_moving_string, "X Coordinate of mouse:%d, Y-Coordinate of mouse:%d", mouse_x, mouse_y); //print mouse coursor coordinates
        sprintf(my_string, "Number of times each button has been pressed - A: %u, B:%u, C:%u, D:%u", //print number of times each button has been pressed
                my_used_buttons[KEYBOARD_A].counter, 
                my_used_buttons[KEYBOARD_B].counter,
                my_used_buttons[KEYBOARD_C].counter,
                my_used_buttons[KEYBOARD_D].counter);
        SDL_SetWindowPosition(window, mouse_x-SCREEN_WIDTH/2, mouse_y-SCREEN_HEIGHT/2); //move window like the mouse move

        tumDrawFilledBox(SCREEN_WIDTH/6 * cos(PI+rotatingvalue) + SCREEN_WIDTH/2 - SCREEN_WIDTH/24,
                        SCREEN_WIDTH/6 *sin(PI+rotatingvalue) + SCREEN_HEIGHT/2 - SCREEN_WIDTH/24, 
                        SCREEN_WIDTH/12, SCREEN_WIDTH / 12 , TUMBlue);
        tumDrawCircle(SCREEN_WIDTH/6 * cos(rotatingvalue) + SCREEN_WIDTH/2, 
                    SCREEN_WIDTH/6 *sin(rotatingvalue)+SCREEN_HEIGHT/2, 
                    SCREEN_WIDTH / 24, Red );
        tumDrawTriangle(triangle_coords, Green);

        if (!tumGetTextSize((char *)my_string, &my_string_width, NULL))
            tumDrawText(my_string,
                        SCREEN_WIDTH / 2 - my_string_width / 2,
                        SCREEN_HEIGHT *11/12 - DEFAULT_FONT_SIZE / 2,
                        TUMBlue);
        if (!tumGetTextSize((char *)my_moving_string, &my_moving_string_width, NULL))
            tumDrawText(my_moving_string,
                        SCREEN_WIDTH/2 - my_moving_string_width/2 + cos(rotatingvalue)*my_moving_string_width/4,
                        SCREEN_HEIGHT *1/12 - DEFAULT_FONT_SIZE / 2,
                        TUMBlue);

        tumDrawUpdateScreen(); // Refresh the screen to draw elements

        // Basic sleep of 20 milliseconds
        vTaskDelay((TickType_t)20);
        rotatingvalue += 0.025;
    }
}

int main(int argc, char *argv[])
{
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);

    printf("Initializing: ");

    if (tumDrawInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize drawing");
        goto err_init_drawing;
    }

    if (tumEventInit()) {
        PRINT_ERROR("Failed to initialize events");
        goto err_init_events;
    }

    if (tumSoundInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize audio");
        goto err_init_audio;
    }

    if (!buttonLockInit()) {
        PRINT_ERROR("Failed to create buttons lock");
        goto err_buttons_lock;
    }

    if (xTaskCreate(vDemoTask, "DemoTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    mainGENERIC_PRIORITY, &DemoTask) != pdPASS) {
        goto err_demotask;
    }

    vTaskStartScheduler();

    return EXIT_SUCCESS;

err_demotask:
    buttonLockExit();
err_buttons_lock:
    tumSoundExit();
err_init_audio:
    tumEventExit();
err_init_events:
    tumDrawExit();
err_init_drawing:
    return EXIT_FAILURE;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
