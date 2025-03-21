/**
 * @file	engine_configuration.h
 * @brief	Main engine configuration data structure.
 *
 * @date Oct 30, 2013
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#pragma once

#include "persistent_configuration.h"

#ifndef DEFAULT_ENGINE_TYPE
#define DEFAULT_ENGINE_TYPE MINIMAL_PINS
#endif

#define CLT_MANUAL_IDLE_CORRECTION config->cltIdleCorrBins, config->cltIdleCorr, CLT_CURVE_SIZE
#define WARMUP_CLT_EXTRA_FUEL_CURVE config->cltFuelCorrBins, config->cltFuelCorr, CLT_CURVE_SIZE
#define IAT_FUEL_CORRECTION_CURVE config->iatFuelCorrBins, config->iatFuelCorr, IAT_CURVE_SIZE
#define INJECTOR_LAG_CURVE engineConfiguration->injector.battLagCorrBins, engineConfiguration->injector.battLagCorr, VBAT_INJECTOR_CURVE_SIZE

#define MOCK_UNDEFINED -1

void setCrankOperationMode();
void setCamOperationMode();

void prepareVoidConfiguration(engine_configuration_s *activeConfiguration);
void setTargetRpmCurve(int rpm);
void setFuelTablesLoadBin(float minValue, float maxValue);
void setWholeIatCorrTimingTable(float value);
void setWholeTimingTable_d(angle_t value);
#define setWholeTimingTable(x) setWholeTimingTable_d(x);
void setConstantDwell(floatms_t dwellMs);

// needed by bootloader
void setDefaultBasePins();

void setDefaultSdCardParameters();

void onBurnRequest();
void incrementGlobalConfigurationVersion();

void commonFrankensoAnalogInputs();

void emptyCallbackWithConfiguration(engine_configuration_s * engine);

typedef void (*configuration_callback_t)(engine_configuration_s*);

#ifdef __cplusplus
// because of 'Logging' class parameter these functions are visible only to C++ code but C code
void loadConfiguration();
/**
 * boardCallback is invoked after configuration reset but before specific engineType configuration
 */
void resetConfigurationExt(configuration_callback_t boardCallback, engine_type_e engineType);
void resetConfigurationExt(engine_type_e engineType);

void rememberCurrentConfiguration();
#endif /* __cplusplus */

void setBoardDefaultConfiguration();
void setBoardConfigOverrides();
// Init any sensors built in to the board that don't need config
void initBoardSensors();
void boardOnConfigurationChange(engine_configuration_s *previousConfiguration);
Gpio getCommsLedPin();
Gpio getWarningLedPin();
Gpio getRunningLedPin();

#if !EFI_UNIT_TEST
extern persistent_config_container_s persistentState;
static engine_configuration_s * const engineConfiguration =
	&persistentState.persistentConfiguration.engineConfiguration;
static persistent_config_s * const config = &persistentState.persistentConfiguration;
#else // EFI_UNIT_TEST
extern engine_configuration_s *engineConfiguration;
extern persistent_config_s *config;
#endif // EFI_UNIT_TEST

extern engine_configuration_s activeConfiguration;

// We store a special changeable copy of configuration is RAM, so we can just compare them
#define isConfigurationChanged(x) (engineConfiguration->x != activeConfiguration.x)

#define isPinOrModeChanged(pin, mode) (isConfigurationChanged(pin) || isConfigurationChanged(mode))

int getBoardMetaOutputsCount();
Gpio* getBoardMetaOutputs();
