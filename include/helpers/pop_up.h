#ifndef POP_UP_H
#define POP_UP_H

#include <Arduino.h>
#include "types/pop_up_timing_calibration.h"
#include "types/pop_up_state.h"
#include "helpers/motor_controller.h"


enum class PopUpId : uint8_t
{
    RH,
    LH
};

/**
 * @brief PopUp control class
 */
class PopUp
{
public:
    /**
     * @brief Constructor
     *
     * @param motor_controller      pointer to a motor controller object that represents the motor used to move this pop-up
     * @param sensing_pin           GPIO pin used to sense pop-up position
     * @param pop_up_id             ID of the PopUp. Either RH or LH
     */
    PopUp(MotorController* motor_controller, int sensing_pin, PopUpId pop_up_id);

    /**
     * @brief Command the pop-up to move to a target position
     *
     * @param target Desired target state (UP or DOWN)
     */
    void set_target(PopUpState target);

    /**
     * @brief Update function, must be called regularly from loop()
     */
    void update();

    /**
     * @brief activates or deactivates the sleepy eye mode
     * 
     * @param active whenever the sleepy eye mode should be active or not
     */
    void set_sleepy_eye_mode(bool active);

    /**
     * @brief winks the pop-up.
     */
    void wink_pop_up();

    /**
     * @brief Get the current state of the pop-up
     *
     * @return PopUpState Current state
     */
    PopUpState get_state() const;

    /**
     * @brief Get the current target state of the pop-up
     *
     * @return PopUpState Current target state
     */
    PopUpState get_target() const;
    
    /**
     * @brief Get the previous target state of the pop-up
     *
     * @return PopUpState Current target state
     */
    PopUpState get_previous_target() const;

    /**
     * @brief Gets sleepy eye mode value
     *
     * @return bool Current value of sleepy_eye_mode
     */
    bool get_sleepy_eye_mode() const;

    /**
     * @brief Gets pop-up name
     *
     * @return const char* of PopUpId
     */
    const char* name() const;

    // Publicly accesible enum
    PopUpId pop_up_id;

private:
    // Configuration
    MotorController *motor_controller;
    int sensing_pin;
    bool is_winking;
    bool auto_toggle_target;
    bool sleepy_eye_mode;
    int sleepy_eye_move_time;

    // State tracking
    PopUpState current_target;
    PopUpState previous_target;

    int movement_start_time;
    bool is_moving;

    PopUpTimingCalibration timing_calibration;


    // Internal helpers
    void _start_pop_up();
    void _stop_motor(bool timed_out);
    int _get_sleepy_eye_move_time();
    
};

#endif
