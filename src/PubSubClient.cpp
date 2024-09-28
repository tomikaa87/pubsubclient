/*

  PubSubClient.cpp - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include "Arduino.h"

PubSubClient::PubSubClient(Client& client)
    : _client{ client }
{
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Client& client)
    : _client{ client }
{
    setServer(addr, port);
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Client& client, Stream& stream)
    : _client{ client }
{
    setServer(addr, port);
    setStream(stream);
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Callback callback, Client& client)
    : _client{ client }
{
    setServer(addr, port);
    setCallback(callback);
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Callback callback, Client& client, Stream& stream)
    : _client{ client }
{
    setServer(addr, port);
    setCallback(callback);
    setStream(stream);
}

PubSubClient::PubSubClient(uint8_t* ip, uint16_t port, Client& client)
    : _client{ client }
{
    setServer(ip, port);
}

PubSubClient::PubSubClient(uint8_t* ip, uint16_t port, Client& client, Stream& stream)
    : _client{ client }
{
    setServer(ip, port);
    setStream(stream);
}

PubSubClient::PubSubClient(uint8_t* ip, uint16_t port, Callback callback, Client& client)
    : _client{ client }
{
    setServer(ip, port);
    setCallback(callback);
}

PubSubClient::PubSubClient(uint8_t* ip, uint16_t port, Callback callback, Client& client, Stream& stream)
    : _client{ client }
{
    setServer(ip, port);
    setCallback(callback);
    setStream(stream);
}

PubSubClient::PubSubClient(const char* domain, uint16_t port, Client& client)
    : _client{ client }
{
    setServer(domain, port);
}

PubSubClient::PubSubClient(const char* domain, uint16_t port, Client& client, Stream& stream)
    : _client{ client }
{
    setServer(domain, port);
    setStream(stream);
}

PubSubClient::PubSubClient(const char* domain, uint16_t port, Callback callback, Client& client)
    : _client{ client }
{
    setServer(domain, port);
    setCallback(callback);
}

PubSubClient::PubSubClient(const char* domain, uint16_t port, Callback callback, Client& client, Stream& stream)
    : _client{ client }
{
    setServer(domain, port);
    setCallback(callback);
    setStream(stream);
}

bool PubSubClient::connect(const char* id)
{
    return connect(id, nullptr, nullptr, 0, 0, 0, 0, 1);
}

bool PubSubClient::connect(const char* id, const char* user, const char* pass)
{
    return connect(id, user, pass, 0, 0, 0, 0, 1);
}

bool PubSubClient::connect(const char* id, const char* willTopic, uint8_t willQos, bool willRetain, const char* willMessage)
{
    return connect(id, nullptr, nullptr, willTopic, willQos, willRetain, willMessage, 1);
}

bool PubSubClient::connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, bool willRetain, const char* willMessage)
{
    return connect(id, user, pass, willTopic, willQos, willRetain, willMessage, 1);
}

bool PubSubClient::connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, bool willRetain, const char* willMessage, bool cleanSession)
{
    if (!connected()) {
        int result = 0;

        if (_client.connected()) {
            result = 1;
        }
        else {
            if (_domain) {
                result = _client.connect(_domain, _port);
            }
            else {
                result = _client.connect(_ip, _port);
            }
        }

        if (result == 1) {
            _nextMsgId = 1;
            // Leave room in the buffer for header and variable length field
            uint16_t length = MQTT_MAX_HEADER_SIZE;
            unsigned int j;

#if MQTT_VERSION == MQTT_VERSION_3_1
            uint8_t d[9] = { 0x00, 0x06, 'M', 'Q', 'I', 's', 'd', 'p', MQTT_VERSION };
#define MQTT_HEADER_VERSION_LENGTH 9
#elif MQTT_VERSION == MQTT_VERSION_3_1_1
            uint8_t d[7] = { 0x00, 0x04, 'M', 'Q', 'T', 'T', MQTT_VERSION };
#define MQTT_HEADER_VERSION_LENGTH 7
#endif
            for (j = 0; j < MQTT_HEADER_VERSION_LENGTH; j++) {
                _buffer[length++] = d[j];
            }

            uint8_t v;
            if (willTopic) {
                v = 0x04 | (willQos << 3) | (willRetain << 5);
            }
            else {
                v = 0x00;
            }
            if (cleanSession) {
                v = v | 0x02;
            }

            if (user) {
                v = v | 0x80;

                if (pass) {
                    v = v | (0x80 >> 1);
                }
            }
            _buffer[length++] = v;

            _buffer[length++] = ((_keepAlive) >> 8);
            _buffer[length++] = ((_keepAlive) & 0xFF);

            CHECK_STRING_LENGTH(length, id)
                length = writeString(id, _buffer, length);
            if (willTopic) {
                CHECK_STRING_LENGTH(length, willTopic)
                    length = writeString(willTopic, _buffer, length);
                CHECK_STRING_LENGTH(length, willMessage)
                    length = writeString(willMessage, _buffer, length);
            }

            if (user) {
                CHECK_STRING_LENGTH(length, user)
                    length = writeString(user, _buffer, length);
                if (pass) {
                    CHECK_STRING_LENGTH(length, pass)
                        length = writeString(pass, _buffer, length);
                }
            }

            write(MQTTCONNECT, _buffer, length - MQTT_MAX_HEADER_SIZE);

            _lastInActivity = _lastOutActivity = millis();

            while (!_client.available()) {
                unsigned long t = millis();
                if (t - _lastInActivity >= ((int32_t)_socketTimeout * 1000UL)) {
                    _state = MQTT_CONNECTION_TIMEOUT;
                    _client.stop();
                    return false;
                }
            }
            uint8_t llen;
            uint32_t len = readPacket(&llen);

            if (len == 4) {
                if (_buffer[3] == 0) {
                    _lastInActivity = millis();
                    pingOutstanding = false;
                    _state = MQTT_CONNECTED;
                    return true;
                }
                else {
                    _state = _buffer[3];
                }
            }
            _client.stop();
        }
        else {
            _state = MQTT_CONNECT_FAILED;
        }
        return false;
    }
    return true;
}

// reads a byte into result
bool PubSubClient::readByte(uint8_t* result)
{
    uint32_t previousMillis = millis();
    while (!_client.available()) {
        yield();
        uint32_t currentMillis = millis();
        if (currentMillis - previousMillis >= ((int32_t)_socketTimeout * 1000)) {
            return false;
        }
    }
    *result = _client.read();
    return true;
}

// reads a byte into result[*index] and increments index
bool PubSubClient::readByte(uint8_t* result, uint16_t* index)
{
    uint16_t current_index = *index;
    uint8_t* write_address = &(result[current_index]);
    if (readByte(write_address)) {
        *index = current_index + 1;
        return true;
    }
    return false;
}

uint32_t PubSubClient::readPacket(uint8_t* lengthLength)
{
    uint16_t len = 0;
    if (!readByte(_buffer, &len))
        return 0;
    bool isPublish = (_buffer[0] & 0xF0) == MQTTPUBLISH;
    uint32_t multiplier = 1;
    uint32_t length = 0;
    uint8_t digit = 0;
    uint16_t skip = 0;
    uint32_t start = 0;

    do {
        if (len == 5) {
            // Invalid remaining length encoding - kill the connection
            _state = MQTT_DISCONNECTED;
            _client.stop();
            return 0;
        }
        if (!readByte(&digit))
            return 0;
        _buffer[len++] = digit;
        length += (digit & 127) * multiplier;
        multiplier <<= 7; // multiplier *= 128
    } while ((digit & 128) != 0);
    *lengthLength = len - 1;

    if (isPublish) {
        // Read in topic length to calculate bytes to skip over for Stream writing
        if (!readByte(_buffer, &len))
            return 0;
        if (!readByte(_buffer, &len))
            return 0;
        skip = (_buffer[*lengthLength + 1] << 8) + _buffer[*lengthLength + 2];
        start = 2;
        if (_buffer[0] & MQTTQOS1) {
            // skip message id
            skip += 2;
        }
    }
    uint32_t idx = len;

    for (uint32_t i = start; i < length; i++) {
        if (!readByte(&digit))
            return 0;
        if (_stream) {
            if (isPublish && idx - *lengthLength - 2 > skip) {
                _stream->write(digit);
            }
        }

        if (len < sizeof(_buffer)) {
            _buffer[len] = digit;
            len++;
        }
        idx++;
    }

    if (!_stream && idx > sizeof(_buffer)) {
        len = 0; // This will cause the packet to be ignored.
    }
    return len;
}

bool PubSubClient::loop()
{
    if (connected()) {
        unsigned long t = millis();
        if ((t - _lastInActivity > _keepAlive * 1000UL) || (t - _lastOutActivity > _keepAlive * 1000UL)) {
            if (pingOutstanding) {
                _state = MQTT_CONNECTION_TIMEOUT;
                _client.stop();
                return false;
            }
            else {
                _buffer[0] = MQTTPINGREQ;
                _buffer[1] = 0;
                _client.write(_buffer, 2);
                _lastOutActivity = t;
                _lastInActivity = t;
                pingOutstanding = true;
            }
        }
        if (_client.available()) {
            uint8_t llen;
            uint16_t len = readPacket(&llen);
            uint16_t msgId = 0;
            uint8_t* payload;
            if (len > 0) {
                _lastInActivity = t;
                uint8_t type = _buffer[0] & 0xF0;
                if (type == MQTTPUBLISH) {
                    if (_callback) {
                        uint16_t tl = (_buffer[llen + 1] << 8) + _buffer[llen + 2]; /* topic length in bytes */
                        memmove(_buffer + llen + 2, _buffer + llen + 3, tl);        /* move topic inside buffer 1 byte to front */
                        _buffer[llen + 2 + tl] = 0;                                      /* end the topic as a 'C' string with \x00 */
                        char* topic = (char*)_buffer + llen + 2;
                        // msgId only present for QOS>0
                        if ((_buffer[0] & 0x06) == MQTTQOS1) {
                            msgId = (_buffer[llen + 3 + tl] << 8) + _buffer[llen + 3 + tl + 1];
                            payload = _buffer + llen + 3 + tl + 2;
                            _callback(topic, payload, len - llen - 3 - tl - 2);

                            _buffer[0] = MQTTPUBACK;
                            _buffer[1] = 2;
                            _buffer[2] = (msgId >> 8);
                            _buffer[3] = (msgId & 0xFF);
                            _client.write(_buffer, 4);
                            _lastOutActivity = t;
                        }
                        else {
                            payload = _buffer + llen + 3 + tl;
                            _callback(topic, payload, len - llen - 3 - tl);
                        }
                    }
                }
                else if (type == MQTTPINGREQ) {
                    _buffer[0] = MQTTPINGRESP;
                    _buffer[1] = 0;
                    _client.write(_buffer, 2);
                }
                else if (type == MQTTPINGRESP) {
                    pingOutstanding = false;
                }
            }
            else if (!connected()) {
                // readPacket has closed the connection
                return false;
            }
        }
        return true;
    }
    return false;
}

bool PubSubClient::publish(const char* topic, const char* payload)
{
    return publish(topic, (const uint8_t*)payload, payload ? strnlen(payload, sizeof(_buffer)) : 0, false);
}

bool PubSubClient::publish(const char* topic, const char* payload, bool retained)
{
    return publish(topic, (const uint8_t*)payload, payload ? strnlen(payload, sizeof(_buffer)) : 0, retained);
}

bool PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength)
{
    return publish(topic, payload, plength, false);
}

bool PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength, bool retained)
{
    if (connected()) {
        if (sizeof(_buffer) < MQTT_MAX_HEADER_SIZE + 2 + strnlen(topic, sizeof(_buffer)) + plength) {
            // Too long
            return false;
        }
        // Leave room in the buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, _buffer, length);

        // Add payload
        uint16_t i;
        for (i = 0; i < plength; i++) {
            _buffer[length++] = payload[i];
        }

        // Write the header
        uint8_t header = MQTTPUBLISH;
        if (retained) {
            header |= 1;
        }
        return write(header, _buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

bool PubSubClient::publish_P(const char* topic, const char* payload, bool retained)
{
    return publish_P(topic, (const uint8_t*)payload, payload ? strnlen(payload, sizeof(_buffer)) : 0, retained);
}

bool PubSubClient::publish_P(const char* topic, const uint8_t* payload, unsigned int plength, bool retained)
{
    uint8_t llen = 0;
    unsigned int rc = 0;
    uint16_t tlen;
    unsigned int pos = 0;
    unsigned int i;
    uint8_t header;
    unsigned int len;

    if (!connected()) {
        return false;
    }

    tlen = strnlen(topic, sizeof(_buffer));

    header = MQTTPUBLISH;
    if (retained) {
        header |= 1;
    }
    _buffer[pos++] = header;
    len = plength + 2 + tlen;
    do {
        uint8_t digit = len & 127; // digit = len %128
        len >>= 7;         // len = len / 128
        if (len > 0) {
            digit |= 0x80;
        }
        _buffer[pos++] = digit;
        llen++;
    } while (len > 0);

    pos = writeString(topic, _buffer, pos);

    rc += _client.write(_buffer, pos);

    for (i = 0; i < plength; i++) {
        rc += _client.write((char)pgm_read_byte_near(payload + i));
    }

    _lastOutActivity = millis();

    const auto expectedLength = 1u + llen + 2u + tlen + plength;

    return (rc == expectedLength);
}

bool PubSubClient::beginPublish(const char* topic, unsigned int plength, bool retained)
{
    if (connected()) {
        // Send the header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic, _buffer, length);
        uint8_t header = MQTTPUBLISH;
        if (retained) {
            header |= 1;
        }
        size_t hlen = buildHeader(header, _buffer, plength + length - MQTT_MAX_HEADER_SIZE);
        uint16_t rc = _client.write(_buffer + (MQTT_MAX_HEADER_SIZE - hlen), length - (MQTT_MAX_HEADER_SIZE - hlen));
        _lastOutActivity = millis();
        return (rc == (length - (MQTT_MAX_HEADER_SIZE - hlen)));
    }
    return false;
}

int PubSubClient::endPublish()
{
    return 1;
}

size_t PubSubClient::write(uint8_t data)
{
    _lastOutActivity = millis();
    return _client.write(data);
}

size_t PubSubClient::write(const uint8_t* buffer, size_t size)
{
    _lastOutActivity = millis();
    return _client.write(buffer, size);
}

size_t PubSubClient::buildHeader(uint8_t header, uint8_t* buf, uint16_t length)
{
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint16_t len = length;
    do {

        digit = len & 127; // digit = len %128
        len >>= 7;         // len = len / 128
        if (len > 0) {
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while (len > 0);

    buf[4 - llen] = header;
    for (int i = 0; i < llen; i++) {
        buf[MQTT_MAX_HEADER_SIZE - llen + i] = lenBuf[i];
    }
    return llen + 1; // Full header size is variable length bit plus the 1-byte fixed header
}

bool PubSubClient::write(uint8_t header, uint8_t* buf, uint16_t length)
{
    uint16_t rc;
    uint8_t hlen = buildHeader(header, buf, length);

#ifdef MQTT_MAX_TRANSFER_SIZE
    uint8_t* writeBuf = buf + (MQTT_MAX_HEADER_SIZE - hlen);
    uint16_t bytesRemaining = length + hlen; // Match the length type
    uint8_t bytesToWrite;
    bool result = true;
    while ((bytesRemaining > 0) && result) {
        bytesToWrite = (bytesRemaining > MQTT_MAX_TRANSFER_SIZE) ? MQTT_MAX_TRANSFER_SIZE : bytesRemaining;
        rc = _client.write(writeBuf, bytesToWrite);
        result = (rc == bytesToWrite);
        bytesRemaining -= rc;
        writeBuf += rc;
    }
    return result;
#else
    rc = _client.write(buf + (MQTT_MAX_HEADER_SIZE - hlen), length + hlen);
    _lastOutActivity = millis();
    return (rc == hlen + length);
#endif
}

bool PubSubClient::subscribe(const char* topic)
{
    return subscribe(topic, 0);
}

bool PubSubClient::subscribe(const char* topic, uint8_t qos)
{
    size_t topicLength = strnlen(topic, sizeof(_buffer));
    if (topic == 0) {
        return false;
    }
    if (qos > 1) {
        return false;
    }
    if (sizeof(_buffer) < 9 + topicLength) {
        // Too long
        return false;
    }
    if (connected()) {
        // Leave room in the buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        _nextMsgId++;
        if (_nextMsgId == 0) {
            _nextMsgId = 1;
        }
        _buffer[length++] = (_nextMsgId >> 8);
        _buffer[length++] = (_nextMsgId & 0xFF);
        length = writeString((char*)topic, _buffer, length);
        _buffer[length++] = qos;
        return write(MQTTSUBSCRIBE | MQTTQOS1, _buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

bool PubSubClient::unsubscribe(const char* topic)
{
    size_t topicLength = strnlen(topic, sizeof(_buffer));
    if (topic == 0) {
        return false;
    }
    if (sizeof(_buffer) < 9 + topicLength) {
        // Too long
        return false;
    }
    if (connected()) {
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        _nextMsgId++;
        if (_nextMsgId == 0) {
            _nextMsgId = 1;
        }
        _buffer[length++] = (_nextMsgId >> 8);
        _buffer[length++] = (_nextMsgId & 0xFF);
        length = writeString(topic, _buffer, length);
        return write(MQTTUNSUBSCRIBE | MQTTQOS1, _buffer, length - MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

void PubSubClient::disconnect()
{
    _buffer[0] = MQTTDISCONNECT;
    _buffer[1] = 0;
    _client.write(_buffer, 2);
    _state = MQTT_DISCONNECTED;
    _client.flush();
    _client.stop();
    _lastInActivity = _lastOutActivity = millis();
}

uint16_t PubSubClient::writeString(const char* string, uint8_t* buf, uint16_t pos)
{
    const char* idp = string;
    uint16_t i = 0;
    pos += 2;
    while (*idp) {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos - i - 2] = (i >> 8);
    buf[pos - i - 1] = (i & 0xFF);
    return pos;
}

bool PubSubClient::connected()
{
    bool rc = (int)_client.connected();
    if (!rc) {
        if (_state == MQTT_CONNECTED) {
            _state = MQTT_CONNECTION_LOST;
            _client.flush();
            _client.stop();
        }
    }
    else {
        return _state == MQTT_CONNECTED;
    }
    return rc;
}

PubSubClient& PubSubClient::setServer(uint8_t* ip, uint16_t port)
{
    IPAddress addr(ip[0], ip[1], ip[2], ip[3]);
    return setServer(addr, port);
}

PubSubClient& PubSubClient::setServer(IPAddress ip, uint16_t port)
{
    _ip = ip;
    _port = port;
    _domain = nullptr;
    return *this;
}

PubSubClient& PubSubClient::setServer(const char* domain, uint16_t port)
{
    _domain = domain;
    _port = port;
    return *this;
}

PubSubClient& PubSubClient::setCallback(Callback callback)
{
    callback = callback;
    return *this;
}

PubSubClient& PubSubClient::setClient(Client& client)
{
    _client = client;
    return *this;
}

PubSubClient& PubSubClient::setStream(Stream& stream)
{
    _stream = &stream;
    return *this;
}

int PubSubClient::state() const
{
    return _state;
}

uint16_t PubSubClient::getBufferSize()
{
    return sizeof(_buffer);
}

PubSubClient& PubSubClient::setKeepAlive(uint16_t keepAlive)
{
    _keepAlive = keepAlive;
    return *this;
}

PubSubClient& PubSubClient::setSocketTimeout(uint16_t timeout)
{
    _socketTimeout = timeout;
    return *this;
}
