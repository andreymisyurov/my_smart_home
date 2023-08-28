#define F_CPU 16000000UL
#define LED_PIN 2
#define LOCK_PIN 3

#include <avr/io.h>
#include <util/delay.h>
#include <Ethernet.h>

void setup();
void loop();

int main(void) {
    setup();

    while(1) {
        loop();
    }
    return 0;
}

void setup() {
    DDRD |= (1 << LED_PIN);
    DDRD &= ~(1 << LOCK_PIN);
    PORTD |= (1 << LOCK_PIN);
}

void loop() {
    uint8_t gercon_state = (PIND & (1 << LOCK_PIN)) == 0;
    if(0 == gercon_state) {
        PORTD |= (1 << LED_PIN);
    } else {
        PORTD &= ~(1 << LED_PIN);
    }

    _delay_ms(100);
}
