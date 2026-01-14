#if defined USE_DMX || defined USE_ARTNET
class DMXListener
{
public:
  virtual void onDMXReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len) = 0;
};

#define DMXListenerDerive , public DMXListener
#else
#define DMXListenerDerive
#endif

#ifdef USE_ESPNOW
class ESPNowStreamReceiver
{
public:
  virtual void onStreamReceived(const uint8_t *data, int len) = 0;
};

#ifdef ESPNOW_BRIDGE
#define ESPNowDerive
#else
#define ESPNowDerive , public ESPNowStreamReceiver
#endif
#else
#define ESPNowDerive
#endif