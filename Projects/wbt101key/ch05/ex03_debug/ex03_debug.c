/* Debugging Example */

#include "wiced.h"
#include "wiced_platform.h"
#include "sparcommon.h"
#include "wiced_bt_stack.h"
#include "wiced_rtos.h"
#include "spar_utils.h"
#include "wiced_hal_wdog.h"
#include "tx_port.h"

/*****************************    Constants   *****************************/
/* Thread will delay for 250ms so that LED frequency will be 500ms = 2 Hz */
#define THREAD_DELAY_IN_MS          (250)

/* Useful macros for thread priorities */
#define PRIORITY_HIGH               (3)
#define PRIORITY_MEDIUM             (5)
#define PRIORITY_LOW                (7)

/* Sensible stack size for most threads */
#define THREAD_STACK_MIN_SIZE       (500)

/*****************************    Variables   *****************************/
wiced_thread_t * led_thread;

/*****************************    Function Prototypes   *******************/
wiced_result_t bt_cback( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data );
void led_control( uint32_t arg );

/*****************************    Functions   *****************************/
/*  Main application. This just starts the BT stack and provides the callback function.
 *  The actual application initialization will happen when stack reports that BT device is ready. */
APPLICATION_START( )
{
    /* Debugging setup */
    #ifdef DEBUG
    SETUP_APP_FOR_DEBUG_IF_DEBUG_ENABLED();
    BUSY_WAIT_TILL_MANUAL_CONTINUE_IF_DEBUG_ENABLED();
    #endif

    wiced_bt_stack_init( bt_cback, NULL, NULL );                    // Register BT stack callback
}


/* Callback function for Bluetooth events */
wiced_result_t bt_cback( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data )
{
    wiced_result_t result = WICED_SUCCESS;

    switch( event )
    {
        /* BlueTooth stack enabled */
        case BTM_ENABLED_EVT:
            /* Start a thread to control LED */
            led_thread = wiced_rtos_create_thread();       // Get memory for the thread handle
            wiced_rtos_init_thread(
                    led_thread,                     // Thread handle
                    PRIORITY_MEDIUM,                // Priority
                    "LED",                       // Name
                    led_control,                    // Function
                    THREAD_STACK_MIN_SIZE,          // Stack
                    NULL );                         // Function argument
            break;

        default:
            break;
    }
    return result;
}


/* Thread function to control the LED */
void led_control( uint32_t arg )
{
    volatile uint32_t led = 0; /* This variable never gets set so the LED will always be off */

    for(;;)
    {
        wiced_hal_gpio_set_pin_output( WICED_GPIO_PIN_LED_1, led );

        /* Send the thread to sleep for a period of time */
        wiced_rtos_delay_milliseconds( THREAD_DELAY_IN_MS, ALLOW_THREAD_TO_SLEEP );
    }
}
