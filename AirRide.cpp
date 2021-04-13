#include <AirRide.h>

/* NOTES
 * Need to do EEPROM logic for compressor class
 * Finished eeprom logic for both analog sensor types
 * Initializer and terminators are now FE, because factory EEPROM comes filled with FF
 */

// EEPROM functions

// update int at eeprom address in *addr with value val
// int pointed to by *addr is automatically incremented
void ee_write_int(uint16_t *addr, int val)
{
    // if (*addr >= EEPROM.length() - sizeof(int)) // FAULT FOR LATER
    EEPROM.put(*addr, val); // write val at EEPROM address addr
    *addr += sizeof(int);   // increment addr to next available position
}

// returns int value at eeprom address in *addr
// int pointed to by *addr is automatically incremented
int ee_read_int(uint16_t *addr)
{
    // if (addr >= EEPROM.length() - sizeof(int)) // FAULT FOR LATER
    int val;                // memory to store int on read
    EEPROM.get(*addr, val); // read integer at address
    *addr += sizeof(int);   // increment addr to next unread space
    return val;             // return integer value read
}

// ------------------- PRESSURE SENSOR FUNCTIONS -----------------------

// return PSI of sensor, using currently loaded settings
int psensor::psi()
{
    // scale output using settings in airsettings.h
    return (int)map((float)analogRead(_pin), low_v, hi_v, low_p, hi_p);
}

void psensor::setPin(uint8_t pin)
{
    _pin = pin;
    pinMode(pin, INPUT);
}

// pass starting address, returns finishing address
void psensor::eeprom_store(uint16_t *addr)
{
    EEPROM.update(*addr++, 0xFE); // initializer

    // bulk of data stored
    ee_write_int(addr, hi_v);  // high analogRead calibration value
    ee_write_int(addr, low_v); // low analogRead calibration value
    ee_write_int(addr, hi_p);  // high psi calibration value
    ee_write_int(addr, low_p); // low psi calibration value

    // spare integers
    ee_write_int(addr, 0);
    ee_write_int(addr, 0);
    ee_write_int(addr, 0);
    ee_write_int(addr, 0);

    EEPROM.update(*addr++, 0xFE); // close with FF byte
}

// pass starting address, returns finishing address
void psensor::eeprom_load(uint16_t *addr)
{
    if (EEPROM.read(*addr++) != 0xFE) // load defaults case, if not initialized
    {
        hi_v = P_HI_VOLTS;
        low_v = P_LOW_VOLTS;
        hi_p = P_HIGH_PRES;
        low_p = P_LOW_PRES;
        *addr += (sizeof(int) * 8) + 1; // need to increment address pointer past data, spares and terminator
        return;                         // once defaults are loaded, dont bother reading rest of EEPROM
    }

    hi_v = ee_read_int(addr);  // high analogRead calibration value
    low_v = ee_read_int(addr); // low analogRead calibration value
    hi_p = ee_read_int(addr);  // high psi calibration value
    low_p = ee_read_int(addr); // low psi calibration value

    addr += (sizeof(int) * 4) + 1; // need to increment address pointer past spares and terminator
}

// ------------------- HEIGHT SENSOR FUNCTIONS -----------------------


// return PSI of sensor, using mapping in airsettings.h
int hsensor::h_mm()
{
    // scale output using settings in airsettings.h
    return (int)map((float)analogRead(_pin), low_v, hi_v, low_h, hi_h);
}

void hsensor::setPin(uint8_t pin)
{
    pinMode(pin, INPUT);
    _pin = pin;
}

// pass starting address, returns finishing address
void hsensor::eeprom_store(uint16_t *addr)
{
    EEPROM.update(*addr++, 0xFE); // initializer

    // bulk of data stored
    ee_write_int(addr, hi_v);  // high analogRead calibration value
    ee_write_int(addr, low_v); // low analogRead calibration value
    ee_write_int(addr, hi_h);  // high psi calibration value
    ee_write_int(addr, low_h); // low psi calibration value

    // spare integers
    ee_write_int(addr, 0);
    ee_write_int(addr, 0);
    ee_write_int(addr, 0);
    ee_write_int(addr, 0);

    EEPROM.update(*addr++, 0xFE); // close with FF byte
}

// pass starting address, returns finishing address
void hsensor::eeprom_load(uint16_t *addr)
{
    if (EEPROM.read(*addr++) != 0xFE) // load defaults case, if not initialized
    {
        hi_v = H_HI_VOLTS;
        low_v = H_LOW_VOLTS;
        hi_h = H_HIGH_HEIGHT;
        low_h = H_LOW_HEIGHT;
        *addr += (sizeof(int) * 8) + 1; // need to increment address pointer past data, spares and terminator
        return;                         // once defaults are loaded, dont bother reading rest of EEPROM
    }

    hi_v = ee_read_int(addr);  // high analogRead calibration value
    low_v = ee_read_int(addr); // low analogRead calibration value
    hi_h = ee_read_int(addr);  // high psi calibration value
    low_h = ee_read_int(addr); // low psi calibration value

    addr += (sizeof(int) * 4) + 1; // need to increment address pointer past spares and terminator
}

// ------------------- PRESSURE VALVE FUNCTIONS -----------------------

// state 1 = open, 0 = closed
void pvalve::setState(bool state)
{
    _state = state;
    digitalWrite(_pin, !_state); // valves run by relays, so active low
}

bool pvalve::getState()
{
    return _state;
}

void pvalve::setPin(uint8_t pin)
{
    pinMode(pin, OUTPUT);
    _pin = pin;
}

// ------------------- COMPRESSOR MOTOR FUNCTIONS -----------------------

// state 1 = powered, 0 = idle
void motor::setState(bool state)
{
    _state = state;
    digitalWrite(_pin, !_state); // valves run by relays, so active low
}

// state 1 = powered, 0 = idle
bool motor::getState()
{
    return _state;
}

// set motor output pin
void motor::setPin(uint8_t pin)
{
    _pin = pin;
    pinMode(pin, OUTPUT);
}

// ------------------- SHOCK OBJECT FUNCTIONS -----------------------

// initialize a shock, with all pin IO values
shock::shock(uint8_t valve_pin_out, uint8_t height_pin, uint8_t pres_pin)
{
    _valve.setPin(valve_pin_out);
    _pressure.setPin(pres_pin);
    _height.setPin(height_pin);
}

// set target height shock should try to maintain
// also sets shock to maintain a certain height
void shock::setHeight(int h_mm)
{
    _mm_target = h_mm;
    _mode = HEIGHTMODE;
}

// set target pressure that shock should maintain
// sets shock control mode to maintain psi rather than height
void shock::setPressure(int psi)
{
    _psi_target = psi;
    _mode = PSIMODE;
}

// check height on sensor associated with shock
int shock::getHeight()
{
    return _height.h_mm();
}

// check pressure in psi on sensor associated with shock
int shock::getPressure()
{
    return _pressure.psi();
}

void shock::update()
{
    // TODO: use _mode and height/psi values to maintain a target setting
}

// ------------------- COMPRESSOR OBJECT FUNCTIONS -----------------------

// pass a motor pin and pressure sensor analog pin. 
compressor::compressor(uint8_t motor_control_pin, uint8_t pres_sensor_pin)
{
    _pressure.setPin(pres_sensor_pin);
    _mtr.setPin(motor_control_pin);
}

// pressure (PSI) for compressor to maintain when active
void compressor::setPressure(int psi)
{
    _runPres = psi;
}

// pressure (PSI) for compressor to maintain when idle
void compressor::setIdlePressure(int psi)
{
    _idlePres = psi;
}

// return pressure in PSI
int compressor::getPressure()
{
    return _pressure.psi();
}

// fetch motor state
bool compressor::isFilling()
{
    return _mtr.getState();
}

// 0 if off, 1 if active, 2 if idle
int compressor::get_state()
{
    if (!_active) // inactive
        return 0;
    else if (!_idle) // active and not idle
        return 1;
    else // active and idle
        return 2;
}

// start regulating tank pressure at running pressure
void compressor::start()
{
    _active = true;
    _idle = false;
}

// set compressor to idle mode, maintaining stored idle PSI
void compressor::start_idle()
{
    _active = true;
    _idle = true;
}

// stop regulating tank pressure
void compressor::stop()
{
    _active = false;
}

// main update loop
void compressor::update()
{
    // set target PSI based on compressor state
    if (_idle)
        _tgtPres = _idlePres;
    else
        _tgtPres = _runPres;

    // if tank is active, monitor pressure and run motor to maintain target
    if (_active)
    {
        // start motor case, requirements: pres too low, not in cooldown, motor is off
        if (_mtr.getState() == OFF && _pressure.psi() < (_tgtPres - _hysteresis) && !_cooldown)
        {
            _mtr.setState(ON);
            _mtrLastStateChangeTime = millis();
        }
        // stop motor case, requirements: motor is on and pres at spec or max time reached
        if (_mtr.getState() == ON)
        {
            if (_pressure.psi() >= (_tgtPres + _hysteresis) || _pressure.psi() >= COMP_MAXPSI)
            {
                _mtr.setState(OFF);
                _mtrLastStateChangeTime = millis();
            }
            if (millis() - _mtrLastStateChangeTime >= (long)COMP_COOLTIME * 1000)
            {
                _mtr.setState(OFF);
                _mtrLastStateChangeTime = millis();
                _cooldown = true;
            }
        }
    }
    // if inactive, then ensure motor is off
    else
    {
        if (_mtr.getState() == ON)
            _mtr.setState(OFF);
        _mtrLastStateChangeTime = millis();
    }

    // if cooldown was engaged, then disengage it when the timer runs out.
    // Cooldown must finish once started before motor starts again, regardless of machine state.
    if (_cooldown && millis() - _mtrLastStateChangeTime >= (long)COMP_COOLTIME * 1000)
        _cooldown = false;
}