#include "helpers/pop_up_motor.h"

MotorControllerPopUpMotor::MotorControllerPopUpMotor(MotorController* motor_controller)
    : motor_controller_(motor_controller)
{
}

void MotorControllerPopUpMotor::run(float duty_percent)
{
    (void)duty_percent;
    motor_controller_->set_run(true);
}

void MotorControllerPopUpMotor::coast()
{
    motor_controller_->set_coast();
}

void MotorControllerPopUpMotor::brake()
{
    motor_controller_->set_brake(true);
}

bool MotorControllerPopUpMotor::is_braking() const
{
    return motor_controller_->is_braking();
}

bool MotorControllerPopUpMotor::has_fault() const
{
    return false;
}

DRV8243PopUpMotor::DRV8243PopUpMotor(DRV8243* motor_driver)
    : motor_driver_(motor_driver)
{
}

void DRV8243PopUpMotor::run(float duty_percent)
{
    motor_driver_->run(duty_percent);
}

void DRV8243PopUpMotor::coast()
{
    motor_driver_->coast();
}

void DRV8243PopUpMotor::brake()
{
    motor_driver_->brake();
}

bool DRV8243PopUpMotor::is_braking() const
{
    return motor_driver_->mode() == DRV8243::Mode::Brake;
}

bool DRV8243PopUpMotor::has_fault() const
{
    return false;
}
