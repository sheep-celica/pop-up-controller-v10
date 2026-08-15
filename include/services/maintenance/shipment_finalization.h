#pragma once

// Returns true after the shipment-finalization command has completed.
// This state is intentionally RAM-only and is cleared by reboot.
bool is_shipment_finalization_mode();

// Clears manufacturing-test data while preserving manufacturing identity and
// all electrical and pop-up calibration data. Enters read-only maintenance
// mode only if every reset operation succeeds.
bool finalize_for_shipment();
