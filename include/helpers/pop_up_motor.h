#pragma once

#include "helpers/DRV8243.h"
#include "helpers/motor_controller.h"

class PopUpMotor {
public:
    virtual ~PopUpMotor() = default;

    virtual void run(float duty_percent = 100.0f) = 0;
    virtual void coast() = 0;
    virtual void brake() = 0;
    virtual bool is_braking() const = 0;
    virtual bool has_fault() const = 0;
};

class MotorControllerPopUpMotor : public PopUpMotor {
public:
    explicit MotorControllerPopUpMotor(MotorController* motor_controller);

    void run(float duty_percent = 100.0f) override;
    void coast() override;
    void brake() override;
    bool is_braking() const override;
    bool has_fault() const override;

private:
    MotorController* motor_controller_;
};

class DRV8243PopUpMotor : public PopUpMotor {
public:
    explicit DRV8243PopUpMotor(DRV8243* motor_driver);

    void run(float duty_percent = 100.0f) override;
    void coast() override;
    void brake() override;
    bool is_braking() const override;
    bool has_fault() const override;

private:
    DRV8243* motor_driver_;
};
