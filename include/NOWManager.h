#ifndef NOWMANAGER_H
#define NOWMANAGER_H

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Struct.h>

// Define the receiver's MAC address (broadcast address in this case).
const uint8_t RECEIVER_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

class NOWManager
{
public:
    // Constructor:
    // - Initializes the system data pointer.
    // - Sets the ring buffer indices for message handling.
    // - Stores the initial release status.
    // - Assigns the static instance pointer for use in callbacks.
    NOWManager(SysData::SysData &Data)
        : sysData(&Data), msgWriteIndex(0), msgReadIndex(0), lastReleaseStatus(2)
    {
        instance = this;
    }

    // Initialize ESPNOW and Wi-Fi functionality.
    // This function prints the MAC address, initializes ESPNOW, and registers
    // the send and receive callbacks.
    void init()
    {
        printMacAddress();

        // Attempt to initialize ESPNOW. If initialization fails, retry once.
        if (!initESPNOW())
        {
            initESPNOW();
        }

        // Register ESPNOW callback functions for receiving and sending data.
        esp_now_register_recv_cb(NOWManager::onReceive);
        esp_now_register_send_cb(NOWManager::onSent);
    }

    // Callback function called when a message is sent.
    // The status parameter indicates if the transmission was successful.
    static void onSent(const uint8_t *mac_addr, esp_now_send_status_t status)
    {
        if (status != ESP_NOW_SEND_SUCCESS)
        {
            // Optionally handle transmission failures here.
        }
    }

    // Callback function called when a message is received.
    // Delegates processing to the instance method handleOnReceive.
    static void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len)
    {
        instance->handleOnReceive(data, len);
    }

    // Processes all messages stored in the ring buffer.
    // This function reads messages sequentially and handles each message based
    // on its packet type.
    void processReceivedMessages()
    {
        while (msgReadIndex != msgWriteIndex)
        {
            ESPNOWMessage &msg = messageBuffer[msgReadIndex];
            processMessage(msg.data, msg.len);
            msgReadIndex = (msgReadIndex + 1) % RING_BUFFER_SIZE;
        }
    }

    // Sends the current release status via ESPNOW.
    // - 'STATUS' is converted to bytes and sent with a packet type identifier.
    void sendReleaseStatus(int &STATUS)
    {
        lastReleaseStatus = STATUS;
        uint8_t bufferTelemetry[5];
        bufferTelemetry[0] = 'R'; // Packet type identifier for release status.
        int index = 1;
        intToBytes(STATUS, bufferTelemetry, index);
        esp_now_send(RECEIVER_MAC, bufferTelemetry, sizeof(bufferTelemetry));
    }

    // Checks if the release status has changed, and re-sends the status if necessary.
    void checkIfSent()
    {
        if (sysData->droneReleaseStatus != lastReleaseStatus)
        {
            sendReleaseStatus(lastReleaseStatus);
        }
    }

private:
    // Define the size of the ring buffer for incoming messages.
    static const int RING_BUFFER_SIZE = 10;

    // Structure to hold an ESPNOW message.
    struct ESPNOWMessage
    {
        uint8_t data[256]; // Buffer to store the message data.
        int len;           // Length of the message data.
    };

    volatile int msgWriteIndex;                    // Write index for the ring buffer.
    volatile int msgReadIndex;                     // Read index for the ring buffer.
    ESPNOWMessage messageBuffer[RING_BUFFER_SIZE]; // Ring buffer to store incoming messages.

    int lastReleaseStatus; // Stores the last transmitted release status.

    // Static instance pointer used to access instance methods from static callbacks.
    static NOWManager *instance;

    // Pointer to the system data structure.
    SysData::SysData *sysData;

    // Prints the device's MAC address in hexadecimal format to the serial monitor.
    void printMacAddress()
    {
        uint8_t ownMacAddress[6];
        WiFi.macAddress(ownMacAddress);
        Serial.print("Own MAC Address: ");
        for (int i = 0; i < 6; i++)
        {
            // Print leading zero for single digit values.
            if (ownMacAddress[i] < 0x10)
            {
                Serial.print("0");
            }
            Serial.print(ownMacAddress[i], HEX);
            if (i < 5)
            {
                Serial.print(":");
            }
        }
        Serial.println();
    }

    // Initializes Wi-Fi and ESPNOW.
    // Sets the Wi-Fi mode to station, starts Wi-Fi, enables long range mode,
    // initializes ESPNOW, and adds a peer for communication.
    bool initESPNOW()
    {
        // Initialize network interface.
        esp_netif_init();

        // Configure and initialize Wi-Fi with default settings.
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_err_t wifiInitResult = esp_wifi_init(&cfg);
        if (wifiInitResult != ESP_OK)
        {
            Serial.printf("Failed to initialize Wi-Fi. Error code: %d\n", wifiInitResult);
            return false;
        }

        // Set device to station mode.
        esp_err_t wifiModeResult = esp_wifi_set_mode(WIFI_MODE_STA);
        if (wifiModeResult != ESP_OK)
        {
            Serial.printf("Failed to set Wi-Fi mode. Error code: %d\n", wifiModeResult);
            return false;
        }

        // Start the Wi-Fi driver.
        esp_err_t wifiStartResult = esp_wifi_start();
        if (wifiStartResult != ESP_OK)
        {
            Serial.printf("Failed to start Wi-Fi. Error code: %d\n", wifiStartResult);
            return false;
        }

        // Enable Long Range (LR) mode for extended communication range.
        esp_err_t resultProtocol = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
        if (resultProtocol == ESP_OK)
        {
            Serial.println("Long Range (LR) mode enabled.");
        }
        else
        {
            Serial.printf("Failed to enable Long Range mode. Error code: %d\n", resultProtocol);
            return false;
        }

        // Set the TX power to the maximum allowed value.
        // The value 78 is specified in quarter dBm units (78/4 = 19.5 dBm).
        // This maximizes the transmission strength.
        esp_err_t txPowerResult = esp_wifi_set_max_tx_power(78);
        if (txPowerResult == ESP_OK)
        {
            Serial.println("TX power set to maximum.");
        }
        else
        {
            Serial.printf("Failed to set TX power to maximum. Error code: %d\n", txPowerResult);
            return false;
        }

        // Initialize ESPNOW protocol.
        if (esp_now_init() != ESP_OK)
        {
            Serial.println("ESP-NOW initialization failed");
            return false;
        }

        // Set up the peer information using the defined receiver MAC address.
        esp_now_peer_info_t peerInfo;
        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, RECEIVER_MAC, sizeof(RECEIVER_MAC));
        peerInfo.channel = 0;     // Use the default Wi-Fi channel.
        peerInfo.encrypt = false; // Disable encryption.

        // Add the peer for ESPNOW communication.
        esp_err_t result = esp_now_add_peer(&peerInfo);
        if (result != ESP_OK)
        {
            Serial.print("Failed to add peer. Error code: ");
            Serial.println(result);
            // Provide detailed error messages based on the error code.
            if (result == ESP_ERR_ESPNOW_NOT_INIT)
            {
                Serial.println("ESP-NOW is not initialized. Please call esp_now_init() first.");
            }
            else if (result == ESP_ERR_ESPNOW_ARG)
            {
                Serial.println("Invalid argument passed to esp_now_add_peer(). Check the peerInfo structure.");
            }
            else if (result == ESP_ERR_ESPNOW_FULL)
            {
                Serial.println("Peer list is full. Try removing a peer or increasing the peer list size.");
            }
            else if (result == ESP_ERR_ESPNOW_NO_MEM)
            {
                Serial.println("Out of memory. Make sure your ESP32 has enough memory.");
            }
            else if (result == ESP_ERR_ESPNOW_EXIST)
            {
                Serial.println("The peer already exists in the peer list.");
            }
            else
            {
                Serial.println("Unknown error occurred.");
            }
            return false;
        }
        return true;
    }

    // Callback handler for received messages.
    // This function stores the incoming data into the ring buffer.
    void handleOnReceive(const uint8_t *data, int len)
    {
        int nextIndex = (msgWriteIndex + 1) % RING_BUFFER_SIZE;
        // If the ring buffer is full, discard the incoming message.
        if (nextIndex == msgReadIndex)
        {
            return;
        }
        memcpy(messageBuffer[msgWriteIndex].data, data, len);
        messageBuffer[msgWriteIndex].len = len;
        msgWriteIndex = nextIndex;
    }

    // Processes a single ESPNOW message based on its packet type.
    // The first byte of the message indicates the packet type.
    void processMessage(const uint8_t *buffer, int len)
    {
        if (len < 1)
            return;

        // Read the packet type identifier.
        char packetType = (char)buffer[0];
        switch (packetType)
        {
        case 'R':
            // Process release status packet.
            handleRecv(buffer);
            break;
        default:
            // Unknown packet type received; no action taken.
            break;
        }
    }

    // Handles incoming release status packets.
    // Converts the subsequent bytes to an integer and updates the system data.
    void handleRecv(const uint8_t *buffer)
    {
        int recv = bytesToInt(buffer + 1);
        // Serial.println(recv);
        sysData->droneReleaseStatus = bytesToInt(buffer + 1);
    }

    // Converts an integer value to a byte array and appends it to the provided buffer.
    // 'index' is used to track the current position in the byte array.
    static inline void intToBytes(const int value, uint8_t *byteArray, int &index)
    {
        uint8_t *valuePtr = (uint8_t *)&value;
        for (int i = 0; i < 4; i++)
        {
            byteArray[index++] = valuePtr[i];
        }
    }

    // Converts a byte array to an integer value.
    static inline const int bytesToInt(const uint8_t *byteArray)
    {
        int value;
        uint8_t *valuePtr = (uint8_t *)&value;
        for (int i = 0; i < 4; i++)
        {
            valuePtr[i] = byteArray[i];
        }
        return value;
    }
};

// Initialize the static instance pointer to null.
NOWManager *NOWManager::instance = nullptr;

#endif // NOWMANAGER_H
