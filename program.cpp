#include <cmath>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>

#include <wiringPi.h>

using namespace std;

// Build : Shift + Cmd + b
// https://github.com/WiringPi/WiringPi.git

const int LED_PIN           = 16;
const int PWM0_PIN          = 12;
const double PWM_FREQ       = 19.2e6;
const int FREQ              = 25000;
const int RANGE = 128;
const int PRESCALE = PWM_FREQ / static_cast<double>(FREQ * RANGE);  // Formula to calculate PWM freq
                                                                    // Pwmf = 19.2e6 (kHz) / PwmClockDivisor / PwmRange
                                                                    // 1920 / 200
auto run_time = 1000000 * 60 * 30; // five minutes
const double SET_POINT = 40.0;          // degrees Celsius
const int SAMPLE_PERIOD = 1000;         // milliseconds
const double MIN_DUTY_CYCLE = 30.0;
const double MAX_DUTY_CYCLE = 95.0;


double read_temp() {
    auto fs = ifstream("/sys/class/thermal/thermal_zone0/temp");

    fs.seekg(0, fs.end);
    size_t size = fs.tellg();
    fs.seekg(0, fs.beg);
    
    if (size) {
        auto val = string(size + 1, '\0');
        fs.read(&val[0], size);
        return atof(val.c_str()) / 1000.0;
    }

    return nan("0");
}

double calc_control(double temp) {
    return 0.0;
}

int calc_duty_cycle(int range, double duty_cycle) {
    return range * duty_cycle / 100.0;
}

void signal_handler(int sig) {
    cout << "signal received: "  << signal << endl;
    pinMode(PWM0_PIN, OUTPUT);
    pwmWrite (PWM0_PIN, LOW);
    
    // terminate program   
    exit(sig); 
}

int main()
{
    // Initialize
    wiringPiSetupPhys();

    for (auto sig = SIGHUP; sig <= SIGRTMAX; ++sig) {
        signal(sig, signal_handler);
    }

    pinMode(PWM0_PIN, PWM_OUTPUT);
    
    // [2, 4095]
    pwmSetClock(PRESCALE);
    
    // [0, 4096]
    pwmSetRange(RANGE);

    auto duty_cycle = 30.0;

    pwmSetMode(PWM_MODE_MS);
    
    pwmWrite(
        PWM0_PIN,
        calc_duty_cycle(RANGE, duty_cycle)
    );

    // control loop
    for (auto temp = read_temp(); /* forever */; delay(SAMPLE_PERIOD), temp = read_temp()) {
        
        if (temp > SET_POINT) {
            duty_cycle = min(MAX_DUTY_CYCLE, duty_cycle + 5.0);
        } else {
            duty_cycle = max(MIN_DUTY_CYCLE, duty_cycle - 5.0);
        }

        // cout << "temp:" << temp << "\tduty cycle: " << duty_cycle << endl;

        pwmWrite(
            PWM0_PIN,
            calc_duty_cycle(RANGE, duty_cycle)
        );
    }
}