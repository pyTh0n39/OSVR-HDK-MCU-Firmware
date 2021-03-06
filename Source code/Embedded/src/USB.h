/*
 * USB.h
 *
 * Created: 10/23/2015 5:32:56 PM
 *  Author: Sensics
 */

#ifndef USB_H_
#define USB_H_

#include <usb_protocol_cdc.h>

#include <stdbool.h>
#include <stdint.h>

/*! \brief Opens the communication port
 * This is called by CDC interface when USB Host enable it.
 *
 * \retval true if cdc startup is successfully done
 */
bool main_cdc_enable(void);

/*! \brief Closes the communication port
 * This is called by CDC interface when USB Host disable it:
 * not necessarily corresponding to when the serial client
 * is closed.
 */
void main_cdc_disable(void);

void main_cdc_rx_notify(void);

/// @brief Check to see if USB CDC is active. Requires that clients set DTR!
bool usb_cdc_is_active(void);

/// @brief checks usb_cdc_is_active and that our transmit buffer isn't full.
bool usb_cdc_should_tx(void);

/*! \brief Called when a start of frame is received on USB line each 1ms.
 */
void main_sof_action(void);

/*! \brief Enters the application in low power mode
 * Callback called when USB host sets USB line in suspend state
 */
void main_suspend_action(void);

/*! \brief Turn on a led to notify active mode
 * Called when the USB line is resumed from the suspend state
 */
void main_resume_action(void);

/*! \brief Save new DTR state to change led behavior.
 * The DTR notify that the terminal have open or close the communication port.
 */
void main_cdc_set_dtr(bool b_enable);

#endif /* USB_H_ */