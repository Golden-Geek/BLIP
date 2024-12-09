#ifdef USE_STREAMING
class LedStreamListener
{
public:
    virtual void onLedStreamReceived(uint16_t universe, const uint8_t* data, uint16_t len) = 0;
};
#endif

#ifdef USE_ESPNOW
class ESPNowStreamReceiver
{
public:
  virtual void onStreamReceived(const uint8_t *data, int len) = 0;
};
#endif