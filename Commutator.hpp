#include <Arduino.h>
#include <Preferences.h>

#ifndef COMMUTATOR_H
#define COMMUTATOR_H
#define rele1 21
#define rele2 19
#define rele3 18
#define rele4 5

class Commutator
{

    public:
        Commutator()
        {
            const size_t n = relayCount();
            for (size_t i = 0; i < n; i++) {
                pinMode(myPins[i], OUTPUT);
            }
        }

        /** NVS и восстановление реле — только после старта Arduino (вызов из setup). */
        void begin()
        {
            if (prefsStarted_) {
                return;
            }
            prefsStarted_ = prefs_.begin("relay", false);
            if (!prefsStarted_) {
                Serial.println(F("Commutator: NVS namespace \"relay\" не открыт"));
                return;
            }
            const size_t n = relayCount();
            for (size_t i = 0; i < n; i++) {
                bool v = prefs_.getBool(nvsKey(i), false);
                digitalWrite(myPins[i], v);
            }
        }

        void write(int relayNumber, bool value)
        {
            digitalWrite(myPins[relayNumber - 1], value);
            if (prefsStarted_) {
                prefs_.putBool(nvsKey(static_cast<size_t>(relayNumber - 1)), value);
            }
        }

        void blink(int relayNumber, int period, int count)
        {
            for(int i=0; i<count; i++)
            {
                write(relayNumber, true);
                delay(period/2);
                write(relayNumber, false);
                delay(period/2);
            }
        }

        int getMaxRelayNumberValue()
        {
            return static_cast<int>(relayCount());
        }

        bool isRelayNumberValid(int relayNumber)
        {
            return relayNumber >= 1 && relayNumber <= getMaxRelayNumberValue();
        }

    private:
        static size_t relayCount() { return sizeof(myPins) / sizeof(myPins[0]); }

        static const char *nvsKey(size_t index) {
            static const char *const keys[4] = {"r1", "r2", "r3", "r4"};
            return keys[index];
        }

        int myPins[4] = {rele1, rele2, rele3, rele4};
        Preferences prefs_;
        bool prefsStarted_{false};
};
#endif