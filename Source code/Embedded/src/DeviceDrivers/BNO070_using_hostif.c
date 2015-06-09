/*
 * NBO070.c
 *
 * Created: 10/30/2014 3:04:17 PM
 *  Author: Sensics Boger
 */

#include <asf.h>
#include "string.h"

#include "GlobalOptions.h"
#include "my_hardware.h"
#include "Console.h"

#include <ioport.h>
#include "BNO070.h"
#include "util/delay.h"

#ifdef BNO070

#include "sensorhub.h"
extern sensorhub_t sensorhub;

bool BNO070Active=false;
uint8_t BNO070_Report[USB_REPORT_SIZE];
bool TWI_BNO070_PORT_initialized=false; // true if already initialized

#if 0
#define DFU_MAJOR 1
#define DFU_MINOR 7
#define DFU_PATCH 0
#include "bno-hostif/1000-3251_1.7.0.384.c"

#else
#define DFU_MAJOR 1
#define DFU_MINOR 2
#define DFU_PATCH 50
#include "bno-hostif/1000-3251_1.2.5_avr.c"
#endif

//#define USE_GAME_ROTATION // if defined, uses game rotation vector, otherwise regular rotation vector

#define DEG2RAD (3.1415926535897932384626433832795 / 180.0)

static int32_t round32(float f) {
    return (int32_t)(f + ((f >= 0) ? 0.5 : -0.5));
}

static int32_t degToRadQ28(float deg) {
    return (int32_t) (deg * DEG2RAD * (1ul << 28));
}

#define toFixed32(x, Q) round32(x * (float)(1ul << Q))

static sensorhub_SensorFeature_t rotationVectorSetings_;
static sensorhub_SensorFeature_t accelerometerSetings_;

static sensorhub_ProductID_t readProductId(void);
static sensorhub_ProductID_t readProductId()
{
    sensorhub_ProductID_t id;
    memset(&id, 0, sizeof(id));

    sensorhub.debugPrintf("Requesting product ID...\r\n");
    int rc = sensorhub_getProductID(&sensorhub, &id);
    if (rc != SENSORHUB_STATUS_SUCCESS) {
        sensorhub.debugPrintf("readProductId received error: %d\r\n", rc);
        return id;
    }

    sensorhub.debugPrintf("  Version %d.%d.%d\r\n", id.swVersionMajor, id.swVersionMinor, id.swVersionPatch);
    sensorhub.debugPrintf("  Part number: %d\r\n", id.swPartNumber);
    sensorhub.debugPrintf("  Build number: %d\r\n", id.swBuildNumber);

    return id;
}

static void checkDfu(void);
static void checkDfu() {

#if 0 // Skip vesrion check and always DFU
    if (1) {
        sensorhub.debugPrintf("Performing DFU . . . \r\n");
#else
    sensorhub_ProductID_t id = readProductId();
    if ((id.swVersionMajor != DFU_MAJOR) ||
        (id.swVersionMinor != DFU_MINOR) ||
        (id.swVersionPatch != DFU_PATCH)) {
        sensorhub.debugPrintf("BNO is not at %d.%d.%d.  Performing DFU . . . \r\n", DFU_MAJOR, DFU_MINOR, DFU_PATCH);
#endif
        int rc = sensorhub_dfu_avr(&sensorhub, &dfuStream);
        if (rc != SENSORHUB_STATUS_SUCCESS) {
            sensorhub.debugPrintf("dfu received error: %d\r\n", rc);
            return;
        }
        sensorhub.debugPrintf("DFU Completed Successfully\r\n");
        // Re-probe:
        sensorhub_probe(&sensorhub);

        // Get the updated version number
        readProductId();
    }
}

static void startRotationVector(void) {
    sensorhub.debugPrintf("Enabling Rotation Vector events...\r\n");
    rotationVectorSetings_.changeSensitivityEnabled = false;
    rotationVectorSetings_.wakeupEnabled = false;
    rotationVectorSetings_.changeSensitivityRelative = true;
    rotationVectorSetings_.changeSensitivity = (uint16_t)(5.0 * DEG2RAD * (1 << 13));
    rotationVectorSetings_.reportInterval = 10000;
    rotationVectorSetings_.batchInterval = 0;
    rotationVectorSetings_.sensorSpecificConfiguration = 0;
#ifdef USE_GAME_ROTATION
    sensorhub_setDynamicFeature(&sensorhub, SENSORHUB_GAME_ROTATION_VECTOR, &rotationVectorSetings_);
    //sensorhub_getDynamicFeature(&sensorhub, SENSORHUB_GAME_ROTATION_VECTOR, &rotationVectorSetings_);
#else
    sensorhub_setDynamicFeature(&sensorhub, SENSORHUB_ROTATION_VECTOR, &rotationVectorSetings_);
    //sensorhub_getDynamicFeature(&sensorhub, SENSORHUB_ROTATION_VECTOR, &rotationVectorSetings_);
#endif

    sensorhub.debugPrintf("Rotation Vector Enabled: %d.\r\n", rotationVectorSetings_.reportInterval);
}

static void startAccelerometer(void) {
    sensorhub.debugPrintf("Enabling Accelerometer events...\r\n");
    accelerometerSetings_.changeSensitivityEnabled = false;
    accelerometerSetings_.wakeupEnabled = false;
    accelerometerSetings_.changeSensitivityRelative = false;
    accelerometerSetings_.changeSensitivity = (uint16_t)(5.0 * DEG2RAD * (1 << 13));
    accelerometerSetings_.reportInterval = 10000;
    accelerometerSetings_.batchInterval = 0;
    accelerometerSetings_.sensorSpecificConfiguration = 0;
    sensorhub_setDynamicFeature(&sensorhub, SENSORHUB_ACCELEROMETER, &accelerometerSetings_);
    //sensorhub_getDynamicFeature(&sensorhub, SENSORHUB_ACCELEROMETER, &accelerometerSetings_);

    sensorhub.debugPrintf("Accelerometer Enabled: %d.\r\n", accelerometerSetings_.reportInterval);
}

static void configureARVRStabilizationFRS(void) {
    int status;
    int32_t arvrConfig[4] = {
        toFixed32(0.2f,   30), // scaling
        degToRadQ28(7.3f), // maxRotation
        degToRadQ28(90.0f), // maxError
        degToRadQ28(0.0f), // stability
    };

    sensorhub.debugPrintf("Configuring ARVR Stabilization.\r\n");
    status = sensorhub_writeFRS(&sensorhub, SENSORHUB_FRS_ARVR_CONFIG,
                               (uint32_t*)arvrConfig, 4);
    if (status != SENSORHUB_STATUS_SUCCESS) {
        sensorhub.debugPrintf("Write FRS failed: %d", status);
    }

    sensorhub.debugPrintf("Configuring ARVR Stabilization.\r\n");
    status = sensorhub_writeFRS(&sensorhub, SENSORHUB_FRS_ARVR_CONFIG,
                                (uint32_t*)arvrConfig, 4);
    if (status != SENSORHUB_STATUS_SUCCESS) {
        sensorhub.debugPrintf("Write FRS failed: %d", status);
    }
}




static void printEvent(const sensorhub_Event_t * event)
{
    switch (event->sensor) {
        case SENSORHUB_ACCELEROMETER:
        {
            sensorhub.debugPrintf("Acc: 0x%04x 0x%04x 0x%04x\r\n",
                event->un.accelerometer.x_16Q8,
                event->un.accelerometer.y_16Q8,
                event->un.accelerometer.z_16Q8);
        }
        break;

        case SENSORHUB_ROTATION_VECTOR:
        {
            float scale = 1.0f / (float)(1 << 14);
            float q[4] = {
                event->un.rotationVector.i_16Q14 * scale,
                event->un.rotationVector.j_16Q14 * scale,
                event->un.rotationVector.k_16Q14 * scale,
                event->un.rotationVector.real_16Q14 * scale
            };
            sensorhub.debugPrintf("RV [xyzw]: %5.3f %5.3f %5.3f %5.3f\r\n", q[0], q[1], q[2], q[3]);
        }
        break;

        case SENSORHUB_GAME_ROTATION_VECTOR:
        {
            float scale = 1.0f / (float)(1 << 14);
            float q[4] = {
                event->un.gameRotationVector.i_16Q14 * scale,
                event->un.gameRotationVector.j_16Q14 * scale,
                event->un.gameRotationVector.k_16Q14 * scale,
                event->un.gameRotationVector.real_16Q14 * scale
            };
            sensorhub.debugPrintf("RV [xyzw]: %5.3f %5.3f %5.3f %5.3f\r\n", q[0], q[1], q[2], q[3]);
        }
        break;
    }
}

static void dispatchEvent(const sensorhub_Event_t * event)
{
    switch (event->sensor) {
        case SENSORHUB_ROTATION_VECTOR:
        {
            BNO070_Report[1] = event->sequenceNumber;
            memcpy(&BNO070_Report[2], &event->un.rotationVector.i_16Q14, 8); // copy quaternion data
            udi_hid_generic_send_report_in(BNO070_Report);
        }
        break;

        case SENSORHUB_GAME_ROTATION_VECTOR:
        {
            BNO070_Report[1] = event->sequenceNumber;
            memcpy(&BNO070_Report[2], &event->un.gameRotationVector.i_16Q14, 8); // copy quaternion data
            udi_hid_generic_send_report_in(BNO070_Report);
        }
        break;
    }
}


bool init_BNO070(void)
{
    int result;

    ioport_configure_pin(BNO_070_Reset_Pin,IOPORT_DIR_OUTPUT | IOPORT_INIT_HIGH); // reset is active low
    ioport_configure_pin(Side_by_side_A,IOPORT_DIR_OUTPUT | IOPORT_INIT_HIGH); 
    ioport_configure_pin(Int_BNO070, IOPORT_DIR_INPUT|IOPORT_MODE_PULLUP);

    if (!TWI_BNO070_PORT_initialized)
    {
        twi_master_options_t opt_BNO070;

        opt_BNO070.speed = BNO_TWI_SPEED;
        opt_BNO070.chip  = BNO070_ADDR;
        result=twi_master_setup(TWI_BNO070_PORT, &opt_BNO070);
        if (result==STATUS_OK)
        {
            TWI_BNO070_PORT_initialized=true;
            //WriteLn("Init success");
        }
        else
            return false;
    }

    //BNO hostif testing    
    if (sensorhub_probe(&sensorhub) != SENSORHUB_STATUS_SUCCESS) {
        return false;
    }

    // restore normal setting
    startRotationVector();
    configureARVRStabilizationFRS();

    // setup report
    BNO070_Report[0]=1; // this indicates the version number of the report
    BNO070_Report[1]=0; // this indicates the sequence number

    return true;
}

bool Check_BNO070(void)
{
#define MAX_EVENTS_AT_A_TIME 5
    sensorhub_Event_t shEvents[MAX_EVENTS_AT_A_TIME];
    int numEvents = 0;
    int i;

    /* Get the shEvents - we may get 0 */
    sensorhub_poll(&sensorhub, shEvents, MAX_EVENTS_AT_A_TIME, &numEvents);

    for (i = 0; i < numEvents; i++) {
        dispatchEvent(&shEvents[i]);
        printEvent(&shEvents[i]);
    }
    
    return numEvents > 0;
}    

bool Tare_BNO070(void)
{
    // execute tare commands

    // In HID parlance, this is a write to the command register, where the command is set output report. The 07 byte is the axes (P2) and the preceding 00 byte is the tare basis.

    const uint8_t tare_now[] = {0x3f,0x03, 0x05 ,0x00 ,0x2F ,0x03 ,0x87 ,0x06 ,0x00 ,0x0D ,0x00 ,0x00 ,0x03 ,0x00 ,0x07 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};

    twi_package_t packet_write = {
        .addr[0]      = 5, //regNum,      // TWI slave memory address data
        .addr[1]      = 0, //regNum,      // TWI slave memory address data
        .addr_length  = sizeof (uint16_t),    // TWI slave memory address data size
        .chip         = BNO070_ADDR,      // TWI slave bus address
        .buffer       = (void *)tare_now, // transfer data source buffer
        .length       = sizeof(tare_now)  // transfer data size (bytes)
    };

    if (twi_master_write(TWI_BNO070_PORT, &packet_write)!=STATUS_OK)
        return false;

    const uint8_t persist_tare[] = {0x3f,0x03 ,0x05 ,0x00 ,0x2F ,0x03 ,0x87 ,0x06 ,0x00 ,0x0D ,0x00 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};

    packet_write.addr[0]      = 5; //regNum,      // TWI slave memory address data
    packet_write.addr[1]      = 0; //regNum,      // TWI slave memory address data
    packet_write.addr_length  = sizeof (uint16_t);    // TWI slave memory address data size
    packet_write.chip         = BNO070_ADDR;      // TWI slave bus address
    packet_write.buffer       = (void *)persist_tare; // transfer data source buffer
    packet_write.length       = sizeof(persist_tare);  // transfer data size (bytes)

    if (twi_master_write(TWI_BNO070_PORT, &packet_write)!=STATUS_OK)
        return false;

    WriteLn("Tare completed");
    return true;
}

bool dfu_BNO070(void) {
    checkDfu();
    return true;
}


#endif
