/*
 * This file has been automatically generated by the WICED 20719-B1 Designer.
 * Bluetooth Application.
 *
 */

/** ex03_ble_adv.c
 *
 */

#include "wiced.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_uuid.h"
#include "wiced_bt_gatt.h"
#include "wiced_hal_nvram.h"
#include "wiced_hal_gpio.h"
#include "wiced_bt_app_hal_common.h"
#include "wiced_hal_platform.h"
#include "wiced_bt_trace.h"
#include "sparcommon.h"
#include "hci_control_api.h"
#include "wiced_transport.h"
#include "wiced_hal_pspi.h"
#include "wiced_bt_cfg.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_app_common.h"
#include "wiced_hal_wdog.h"


/*******************************************************************
 * Constant Definitions
 ******************************************************************/
#define TRANS_UART_BUFFER_SIZE  1024
#define TRANS_UART_BUFFER_COUNT 2

/*******************************************************************
 * Variable Definitions
 ******************************************************************/
extern const wiced_bt_cfg_settings_t wiced_bt_cfg_settings;
extern const wiced_bt_cfg_buf_pool_t wiced_bt_cfg_buf_pools[WICED_BT_CFG_NUM_BUF_POOLS];
// Local Device Name in device configuration
extern uint8_t BT_LOCAL_NAME[];
// Transport pool for sending RFCOMM data to host
static wiced_transport_buffer_pool_t* transport_pool = NULL;

/* This value will be part of the advertisement packet.
 * The 1st 2 bytes are Cypress ID. The last byte will increment when MB1 is pressed */
uint8_t manuf_data[] = {0x31, 0x01, 0x00};

/*******************************************************************
 * Function Prototypes
 ******************************************************************/
static void                  ex01_wicedhci_app_init               ( void );
static wiced_bt_dev_status_t ex01_wicedhci_management_callback    ( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data );
static void                  ex01_wicedhci_set_advertisement_data ( void );
static void                  ex01_wicedhci_advertisement_stopped  ( void );
static void                  ex01_wicedhci_reset_device           ( void );
static uint32_t              hci_control_process_rx_cmd          ( uint8_t* p_data, uint32_t len );
#ifdef HCI_TRACE_OVER_TRANSPORT
static void                  ex01_wicedhci_trace_callback         ( wiced_bt_hci_trace_type_t type, uint16_t length, uint8_t* p_data );
#endif
void button_cback( void *data, uint8_t port_pin );

/*******************************************************************
 * Macro Definitions
 ******************************************************************/
// Macro to extract uint16_t from little-endian byte array
#define LITTLE_ENDIAN_BYTE_ARRAY_TO_UINT16(byte_array) \
        (uint16_t)( ((byte_array)[0] | ((byte_array)[1] << 8)) )

/*******************************************************************
 * Transport Configuration
 ******************************************************************/
wiced_transport_cfg_t transport_cfg =
{
    WICED_TRANSPORT_UART,              /**< Wiced transport type. */
    {
        WICED_TRANSPORT_UART_HCI_MODE, /**<  UART mode, HCI or Raw */
        HCI_UART_DEFAULT_BAUD          /**<  UART baud rate */
    },
    {
        TRANS_UART_BUFFER_SIZE,        /**<  Rx Buffer Size */
        TRANS_UART_BUFFER_COUNT        /**<  Rx Buffer Count */
    },
    NULL,                              /**< Wiced transport status handler.*/
    hci_control_process_rx_cmd,        /**< Wiced transport receive data handler. */
    NULL                               /**< Wiced transport tx complete callback. */
};

/*******************************************************************
 * Function Definitions
 ******************************************************************/

/*
 * Entry point to the application. Set device configuration and start BT
 * stack initialization.  The actual application initialization will happen
 * when stack reports that BT device is ready
 */
void application_start(void)
{
    /* Initialize the transport configuration */
    wiced_transport_init( &transport_cfg );

    /* Initialize Transport Buffer Pool */
    transport_pool = wiced_transport_create_buffer_pool ( TRANS_UART_BUFFER_SIZE, TRANS_UART_BUFFER_COUNT );

#if ((defined WICED_BT_TRACE_ENABLE) || (defined HCI_TRACE_OVER_TRANSPORT))
    /* Set the Debug UART as WICED_ROUTE_DEBUG_NONE to get rid of prints */
    //  wiced_set_debug_uart( WICED_ROUTE_DEBUG_NONE );

    /* Set Debug UART as WICED_ROUTE_DEBUG_TO_PUART to see debug traces on Peripheral UART (PUART) */
    wiced_set_debug_uart( WICED_ROUTE_DEBUG_TO_PUART );

    /* Set the Debug UART as WICED_ROUTE_DEBUG_TO_WICED_UART to send debug strings over the WICED debug interface */
    //wiced_set_debug_uart( WICED_ROUTE_DEBUG_TO_WICED_UART );
#endif

    /* Initialize Bluetooth Controller and Host Stack */
    wiced_bt_stack_init(ex01_wicedhci_management_callback, &wiced_bt_cfg_settings, wiced_bt_cfg_buf_pools);
}

/*
 * This function is executed in the BTM_ENABLED_EVT management callback.
 */
void ex01_wicedhci_app_init(void)
{
    /* Initialize Application */
    wiced_bt_app_init();

    /* Configure the Button GPIO as an input with a resistive pull up and interrupt on rising edge */
    wiced_hal_gpio_register_pin_for_interrupt( WICED_GPIO_PIN_BUTTON_1, button_cback, NULL );
    wiced_hal_gpio_configure_pin( WICED_GPIO_PIN_BUTTON_1, ( GPIO_INPUT_ENABLE | GPIO_PULL_UP | GPIO_EN_INT_FALLING_EDGE ), GPIO_PIN_OUTPUT_HIGH );

    /* Allow peer to pair */
    wiced_bt_set_pairable_mode(WICED_TRUE, 0);

    /* Set Advertisement Data */
    ex01_wicedhci_set_advertisement_data();

    /* Start Undirected LE Advertisements on device startup.
     * The corresponding parameters are contained in 'wiced_bt_cfg.c' */
    /* TODO: Make sure that this is the desired behavior. */
    wiced_bt_start_advertisements(BTM_BLE_ADVERT_NONCONN_HIGH, 0, NULL);
}

/* Set Advertisement Data */
void ex01_wicedhci_set_advertisement_data( void )
{
    wiced_bt_ble_advert_elem_t adv_elem[4] = { 0 };
    uint8_t adv_flag = BTM_BLE_GENERAL_DISCOVERABLE_FLAG | BTM_BLE_BREDR_NOT_SUPPORTED;
    uint8_t adv_appearance[] = { BIT16_TO_8( APPEARANCE_GENERIC_TAG ) };
    uint8_t num_elem = 0; 

    /* Advertisement Element for Flags */
    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_FLAG;
    adv_elem[num_elem].len = sizeof(uint8_t);
    adv_elem[num_elem].p_data = &adv_flag;
    num_elem++;

    /* Advertisement Element for Name */
    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
    adv_elem[num_elem].len = strlen((const char*)BT_LOCAL_NAME);
    adv_elem[num_elem].p_data = BT_LOCAL_NAME;
    num_elem++;

    /* Advertisement Element for Appearance */
    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_APPEARANCE;
    adv_elem[num_elem].len = sizeof(adv_appearance);
    adv_elem[num_elem].p_data = adv_appearance;
    num_elem++;

    /* Advertisement Element for Manufacturer Data */
    adv_elem[num_elem].advert_type = BTM_BLE_ADVERT_TYPE_MANUFACTURER;
    adv_elem[num_elem].len = sizeof(manuf_data);
    adv_elem[num_elem].p_data = manuf_data;
    num_elem++;

    /* Set Raw Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(num_elem, adv_elem);
}

/* This function is invoked when advertisements stop */
void ex01_wicedhci_advertisement_stopped( void )
{
    WICED_BT_TRACE("Advertisement stopped\n");

    /* TODO: Handle when advertisements stop */
}

/* TODO: This function should be called when the device needs to be reset */
void ex01_wicedhci_reset_device( void )
{
    /* TODO: Clear any additional persistent values used by the application from NVRAM */

    // Reset the device
    wiced_hal_wdog_reset_system( );
}

/* Bluetooth Management Event Handler */
wiced_bt_dev_status_t ex01_wicedhci_management_callback( wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data )
{
    wiced_bt_dev_status_t status = WICED_BT_SUCCESS;
    wiced_bt_device_address_t bda = { 0 };
    wiced_bt_dev_ble_pairing_info_t *p_ble_info = NULL;
    wiced_bt_ble_advert_mode_t *p_adv_mode = NULL;

    switch (event)
    {
    case BTM_ENABLED_EVT:
        /* Bluetooth Controller and Host Stack Enabled */

#ifdef HCI_TRACE_OVER_TRANSPORT
        // There is a virtual HCI interface between upper layers of the stack and
        // the controller portion of the chip with lower layers of the BT stack.
        // Register with the stack to receive all HCI commands, events and data.
        wiced_bt_dev_register_hci_trace(ex01_wicedhci_trace_callback);
#endif

        WICED_BT_TRACE("Bluetooth Enabled (%s)\n",
                ((WICED_BT_SUCCESS == p_event_data->enabled.status) ? "success" : "failure"));

        if (WICED_BT_SUCCESS == p_event_data->enabled.status)
        {
            /* Bluetooth is enabled */
            wiced_bt_dev_read_local_addr(bda);
            WICED_BT_TRACE("Local Bluetooth Address: [%B]\n", bda);

            /* Perform application-specific initialization */
            ex01_wicedhci_app_init();
        }
        break;
    case BTM_DISABLED_EVT:
        /* Bluetooth Controller and Host Stack Disabled */
        WICED_BT_TRACE("Bluetooth Disabled\n");
        break;
    case BTM_SECURITY_REQUEST_EVT:
        /* Security Request */
        WICED_BT_TRACE("Security Request\n");
        wiced_bt_ble_security_grant(p_event_data->security_request.bd_addr, WICED_BT_SUCCESS);
        break;
    case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
        /* Request for Pairing IO Capabilities (BLE) */
        WICED_BT_TRACE("BLE Pairing IO Capabilities Request\n");
        /* No IO Capabilities on this Platform */
        p_event_data->pairing_io_capabilities_ble_request.local_io_cap = BTM_IO_CAPABILITIES_NONE;
        p_event_data->pairing_io_capabilities_ble_request.oob_data = BTM_OOB_NONE;
        p_event_data->pairing_io_capabilities_ble_request.auth_req = BTM_LE_AUTH_REQ_BOND|BTM_LE_AUTH_REQ_MITM;
        p_event_data->pairing_io_capabilities_ble_request.max_key_size = 0x10;
        p_event_data->pairing_io_capabilities_ble_request.init_keys = 0;
        p_event_data->pairing_io_capabilities_ble_request.resp_keys = BTM_LE_KEY_PENC|BTM_LE_KEY_PID;
        break;
    case BTM_PAIRING_COMPLETE_EVT:
        /* Pairing is Complete */
        p_ble_info = &p_event_data->pairing_complete.pairing_complete_info.ble;
        WICED_BT_TRACE("Pairing Complete %d.\n", p_ble_info->reason);
        break;
    case BTM_ENCRYPTION_STATUS_EVT:
        /* Encryption Status Change */
        WICED_BT_TRACE("Encryption Status event: bd ( %B ) res %d\n", p_event_data->encryption_status.bd_addr, p_event_data->encryption_status.result);
        break;
    case BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
        /* Paired Device Link Keys Request */
        WICED_BT_TRACE("Paired Device Link Request Keys Event\n");
        /* Device/app-specific TODO: HANDLE PAIRED DEVICE LINK REQUEST KEY - retrieve from NVRAM, etc */
#if 0
        if (ex01_wicedhci_read_link_keys( &p_event_data->paired_device_link_keys_request ))
        {
            WICED_BT_TRACE("Key Retrieval Success\n");
        }
        else
#endif
        /* Until key retrieval implemented above, just fail the request - will cause re-pairing */
        {
            WICED_BT_TRACE("Key Retrieval Failure\n");
            status = WICED_BT_ERROR;
        }
        break;
    case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        /* Advertisement State Changed */
        p_adv_mode = &p_event_data->ble_advert_state_changed;
        WICED_BT_TRACE("Advertisement State Change: %d\n", *p_adv_mode);
        if ( BTM_BLE_ADVERT_OFF == *p_adv_mode )
        {
            ex01_wicedhci_advertisement_stopped();
        }
        break;
    case BTM_USER_CONFIRMATION_REQUEST_EVT:
        /* Pairing request, TODO: handle confirmation of numeric compare here if desired */
        WICED_BT_TRACE("numeric_value: %d\n", p_event_data->user_confirmation_request.numeric_value);
        wiced_bt_dev_confirm_req_reply( WICED_BT_SUCCESS , p_event_data->user_confirmation_request.bd_addr);
        break;
    default:
        WICED_BT_TRACE("Unhandled Bluetooth Management Event: 0x%x (%d)\n", event, event);
        break;
    }

    return status;
}


/* Handle Command Received over Transport */
uint32_t hci_control_process_rx_cmd( uint8_t* p_data, uint32_t len )
{
    uint8_t status = 0;
    uint8_t cmd_status = HCI_CONTROL_STATUS_SUCCESS;
    uint8_t opcode = 0;
    uint8_t* p_payload_data = NULL;
    uint8_t payload_length = 0;

    WICED_BT_TRACE("hci_control_process_rx_cmd : Data Length '%d'\n", len);

    // At least 4 bytes are expected in WICED Header
    if ((NULL == p_data) || (len < 4))
    {
        WICED_BT_TRACE("Invalid Parameters\n");
        status = HCI_CONTROL_STATUS_INVALID_ARGS;
    }
    else
    {
        // Extract OpCode and Payload Length from little-endian byte array
        opcode = LITTLE_ENDIAN_BYTE_ARRAY_TO_UINT16(p_data);
        payload_length = LITTLE_ENDIAN_BYTE_ARRAY_TO_UINT16(&p_data[sizeof(uint16_t)]);
        p_payload_data = &p_data[sizeof(uint16_t)*2];

        // TODO : Process received HCI Command based on its Control Group
        // (see 'hci_control_api.h' for additional details)
        switch ( HCI_CONTROL_GROUP(opcode) )
        {
        default:
            // HCI Control Group was not handled
            cmd_status = HCI_CONTROL_STATUS_UNKNOWN_GROUP;
            wiced_transport_send_data(HCI_CONTROL_EVENT_COMMAND_STATUS, &cmd_status, sizeof(cmd_status));
            break;
        }
    }

    // When operating in WICED_TRANSPORT_UART_HCI_MODE or WICED_TRANSPORT_SPI,
    // application has to free buffer in which data was received
    wiced_transport_free_buffer( p_data );
    p_data = NULL;

    return status;
}

#ifdef HCI_TRACE_OVER_TRANSPORT
/* Handle Sending of Trace over the Transport */
void ex01_wicedhci_trace_callback( wiced_bt_hci_trace_type_t type, uint16_t length, uint8_t* p_data )
{
    wiced_transport_send_hci_trace( transport_pool, type, length, p_data );
}
#endif

/* Interrupt callback function for BUTTON_1 */
void button_cback( void *data, uint8_t port_pin )
{
    /* Clear the GPIO interrupt */
    wiced_hal_gpio_clear_pin_interrupt_status( WICED_GPIO_PIN_BUTTON_1 );

    /* Increment manufacturing data variable */
    manuf_data[2]++;

    /* Call routine to setup advertising packet and restart advertising */
    ex01_wicedhci_set_advertisement_data();
    WICED_BT_TRACE( "Manuf Data updated to %d\n\r", manuf_data[2] );

}
