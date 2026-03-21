# I/O and Connectors

This page describes the controller from the firmware's point of view rather than as a full harness or schematic document.

## Main Input And Output Groups

The firmware currently works with logical groups such as:

- right-hand and left-hand pop-up motor control
- right-hand and left-hand pop-up sensing
- light-switch inputs
- local buttons
- sleepy-eye position input
- illumination output
- power-latch control
- optional remote inputs

## Internal Expander Signals

The internal ADS7138 is used for a mix of analog inputs, digital inputs, and LED outputs, including:

- battery-voltage measurement
- LED brightness adjustment input
- pop-up offset adjustment input
- debug button input
- status, input, error, and sleepy-eye indicator LEDs

## Optional External Remote Inputs

When the external expander is present, the firmware can register four remote inputs.

Those remote inputs can be mapped in software and used for controller actions such as wink commands and sleepy-eye control.

## What This Page Does Not Cover

- full connector pinout drawings
- harness color information
- installation photos
- unpublished schematic details

Those details are intentionally kept out of this repo for now.
