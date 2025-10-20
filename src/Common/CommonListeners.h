#ifdef USE_STREAMING
class LedStreamListener
{
public:
  virtual void onLedStreamReceived(uint16_t universe, const uint8_t *data, uint16_t startChannel, uint16_t len) = 0;
};

#define LedStreamListenerDerive , public LedStreamListener
#else
#define LedStreamListenerDerive
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