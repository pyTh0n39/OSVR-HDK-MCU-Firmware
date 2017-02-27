/*
 * Solomon.h
 *
 * Created: 9/7/2013 11:35:02 PM
 *  Author: Sensics
 */

#ifndef SOLOMON_H_
#define SOLOMON_H_

#include "GlobalOptions.h"
#include "DeviceDrivers/Display.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef SVR_HAVE_SOLOMON1
#define Solomon1 0
#define Solomon1_CSN IOPORT_CREATE_PIN(PORTC, 4)
#define Solomon1_AddrData IOPORT_CREATE_PIN(PORTB, 1)
#define Solomon1_VOUT_Shutdown IOPORT_CREATE_PIN(PORTA, 1)  // when low, Solomon is activated
#define Solomon1_Reset IOPORT_CREATE_PIN(PORTA, 2)
#endif  // SVR_HAVE_SOLOMON1

#ifdef SVR_HAVE_SOLOMON2
#define Solomon2 1
// todo: map to actual one and enable
//#define Solomon2_CSN            IOPORT_CREATE_PIN(PORTF, 3)
#define Solomon2_AddrData IOPORT_CREATE_PIN(PORTB, 3)  // todo: map to actual one and enable
#define Solomon2_VOUT_Shutdown IOPORT_CREATE_PIN(PORTA, 3)
#define Solomon2_Reset IOPORT_CREATE_PIN(PORTA, 4)
/* When Mux_OE is low, Mux_select low is Solomon1, Mux_select high is Solomon2 */
#define SPI_Mux_OE IOPORT_CREATE_PIN(PORTC, 2)
#define SPI_Mux_Select IOPORT_CREATE_PIN(PORTC, 3)
#endif  // SVR_HAVE_SOLOMON2

void init_solomon(void);
bool init_solomon_device(uint8_t deviceID);
uint16_t read_solomon(uint8_t channel, uint8_t address);
void write_solomon(uint8_t channel, uint8_t address, uint16_t data);
void write_solomon_pair(uint8_t channel, uint8_t address, uint16_t data1, uint16_t data2);
void raise_sdc(uint8_t channel);
void lower_sdc(uint8_t channel);
void Solomon_Reset(uint8_t SolomonNum);

/// Writes out the config register to the console.
void Solomon_Dump_All_Config_Debug(const char* loc);
/// Writes out the config register to the console for just one device.
void Solomon_Dump_Config_Debug(uint8_t deviceId, const char* loc);

#define SOLOMON_REG_CFGR UINT8_C(0xB7)
static const uint16_t SOLOMON_CFGR_TXD_bm = (UINT16_C(0x01) << 11);
static const uint16_t SOLOMON_CFGR_LPE_bm = (UINT16_C(0x01) << 10);
static const uint16_t SOLOMON_CFGR_REN_bm = (UINT16_C(0x01) << 7);
static const uint16_t SOLOMON_CFGR_DCS_bm = (UINT16_C(0x01) << 6);

/// Clock select - only touch when PEN is off.
static const uint16_t SOLOMON_CFGR_CSS_bm = (UINT16_C(0x01) << 5);
/// video enable
static const uint16_t SOLOMON_CFGR_VEN_bm = (UINT16_C(0x01) << 3);
/// Sleep/ULP mode
static const uint16_t SOLOMON_CFGR_SLP_bm = (UINT16_C(0x01) << 2);
/// clock lane enable high speed in all cases
static const uint16_t SOLOMON_CFGR_CKE_bm = (UINT16_C(0x01) << 1);
/// Send data using HS interface
static const uint16_t SOLOMON_CFGR_HS_bm = (UINT16_C(0x01));

/// Packet drop register
#define SOLOMON_REG_PDR UINT8_C(0xBF)
/// PLL control reg
#define SOLOMON_REG_PCR UINT8_C(0xBF)
/// PLL enable
static const uint16_t SOLOMON_PCR_PEN_bm = (UINT16_C(0x01));

// read the solomon ID
uint16_t read_Solomon_ID(uint8_t channel);

#endif /* SOLOMON_H_ */
