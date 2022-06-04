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
#include <unistd.h>

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)
#define PI 3.141
#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR
#define FPS_AVERAGE_COUNT 50
#define FPS_FONT "IBMPlexSans-Bold.ttf"

static TaskHandle_t TaskTwo = NULL;
static TaskHandle_t CircleBlinkOne = NULL;
static TaskHandle_t CircleBlinkTwo = NULL;
static SemaphoreHandle_t my_lock = NULL;
static StaticTask_t xBlinkTwoTCB;
static StackType_t uxBlinkTwoStack[ 200 ];

void vDrawFPS(void)
{
    static unsigned int periods[FPS_AVERAGE_COUNT] = { 0 };
    static unsigned int periods_total = 0;
    static unsigned int index = 0;
    static unsigned int average_count = 0;
    static TickType_t xLastWakeTime = 0, prevWakeTime = 0;
    static char str[10] = { 0 };
    static int text_width;
    int fps = 0;
    font_handle_t cur_font = tumFontGetCurFontHandle();

    if (average_count < FPS_AVERAGE_COUNT) {
        average_count++;
    }
    else {
        periods_total -= periods[index];
    }

    xLastWakeTime = xTaskGetTickCount();

    if (prevWakeTime != xLastWakeTime) {
        periods[index] =
            configTICK_RATE_HZ / (xLastWakeTime - prevWakeTime);
        prevWakeTime = xLastWakeTime;
    }
    else {
        periods[index] = 0;
    }

    periods_total += periods[index];

    if (index == (FPS_AVERAGE_COUNT - 1)) {
        index = 0;
    }
    else {
        index++;
    }

    fps = periods_total / average_count;

    tumFontSelectFontFromName(FPS_FONT);

    sprintf(str, "FPS: %2d", fps);

    if (!tumGetTextSize((char *)str, &text_width, NULL))
        tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                              SCREEN_HEIGHT - DEFAULT_FONT_SIZE * 1.5,
                              Skyblue);
    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
}


void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ 200 ];
    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = 200;
}

void DrawingTask(void *pvParameters){
    tumDrawBindThread();
}

void vCircleBlinkOne(void *pvParameters){
    printf("Start One \n");
    static bool end_task_flag = false;
    tumDrawBindThread();
    while(1){
        tumEventFetchEvents(FETCH_EVENT_NONBLOCK); // Query events backend for new events, ie. button presses
        xGetButtonInput(); // Update global input
            // `buttons` is a global shared variable and as such needs to be
            // guarded with a mutex, mutex must be obtained before accessing the
            // resource and given back when you're finished. If the mutex is not
            // given back then no other task can access the reseource.
        evaluateButtons(&end_task_flag); //evaluate Pressed Buttons
        tumDrawCircle(SCREEN_WIDTH/4, SCREEN_HEIGHT/2, SCREEN_HEIGHT/4, White);
        vDrawFPS();
        tumDrawUpdateScreen();
        vTaskDelay((TickType_t)250);
        tumDrawCircle(SCREEN_WIDTH/4, SCREEN_HEIGHT/2, SCREEN_HEIGHT/4, Pink);
        vDrawFPS();
        tumDrawUpdateScreen();
        vTaskDelay((TickType_t)250);
    }
}

void vCircleBlinkTwo(void *pvParameters){
    while(1){
        tumDrawCircle(SCREEN_WIDTH*3/4, SCREEN_HEIGHT/2, SCREEN_HEIGHT/4, White);
        vTaskDelay((TickType_t)500);
        tumDrawCircle(SCREEN_WIDTH*3/4, SCREEN_HEIGHT/2, SCREEN_HEIGHT/4, Pink);
        vTaskDelay((TickType_t)500);
    }
}


void vTaskTwo(void *pvParameters)
{   
    static double rotatingvalue = 0.0;
    static char my_string[100];
    static char my_moving_string[100];
    static int my_string_width = 0;
    static int my_moving_string_width = 0;
    static int mouse_x;
    static int mouse_y;
    static bool end_task_flag = false;
    TickType_t xLastWakeTime;
    const TickType_t frameratePeriod = 20;
    extern SDL_Window *window;

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

    while (1) {
        if(xSemaphoreTake(my_lock, portMAX_DELAY)==pdTRUE){
            xLastWakeTime = xTaskGetTickCount();
            tumDrawBindThread();
            end_task_flag = false;
            tumEventFetchEvents(FETCH_EVENT_NONBLOCK); // Query events backend for new events, ie. button presses
            xGetButtonInput(); // Update global input
            // `buttons` is a global shared variable and as such needs to be
            // guarded with a mutex, mutex must be obtained before accessing the
            // resource and given back when you're finished. If the mutex is not
            // given back then no other task can access the reseource.
            evaluateButtons(&end_task_flag); //evaluate Pressed Buttons
            SDL_GetGlobalMouseState(&mouse_x,&mouse_y); // get current mouse cursor position
            tumDrawClear(White); // Clear screen

            sprintf(my_moving_string, "X Coordinate of mouse:%d, Y-Coordinate of mouse:%d", mouse_x, mouse_y); //print mouse coursor coordinates
            sprintf(my_string, "Number of times each button has been pressed - A: %u, B:%u, C:%u, D:%u", //print number of times each button has been pressed
                    getButtonCounter(KEYBOARD_A), 
                    getButtonCounter(KEYBOARD_B),
                    getButtonCounter(KEYBOARD_C),
                    getButtonCounter(KEYBOARD_D));
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
            vDrawFPS();
            tumDrawUpdateScreen(); // Refresh the screen to draw elements
            if(end_task_flag){
                vTaskResume(CircleBlinkOne);
                vTaskResume(CircleBlinkTwo);
                xSemaphoreGive(my_lock);
                vTaskSuspend(TaskTwo);
            }
            // Basic sleep of 20 milliseconds
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(frameratePeriod));
            xSemaphoreGive(my_lock);
            rotatingvalue += 0.025;
        }
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

    if (xTaskCreate(vTaskTwo, "TaskTwo", mainGENERIC_STACK_SIZE*2, NULL,
                    mainGENERIC_PRIORITY, &TaskTwo) != pdPASS) {
        goto err_demotask;
    }

    if (xTaskCreate(vCircleBlinkOne, "CircleBlinkOne", mainGENERIC_STACK_SIZE * 2, NULL,
                    1, &CircleBlinkOne) != pdPASS) {
        goto err_demotask;
    }
    CircleBlinkTwo= xTaskCreateStatic(vCircleBlinkTwo, "CircleBlinkTwo", 200, NULL,
                    2, uxBlinkTwoStack, &xBlinkTwoTCB);
    if (CircleBlinkTwo == NULL){
        goto err_demotask;
    }
    vTaskSuspend(CircleBlinkOne);
    vTaskSuspend(CircleBlinkTwo);
    my_lock = xSemaphoreCreateMutex();
    xSemaphoreGive(my_lock);
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
