/**
 * @file	adc_inputs.cpp
 * @brief	Low level ADC code
 *
 * rusEfi uses two ADC devices on the same 16 pins at the moment. Two ADC devices are used in orde to distinguish between
 * fast and slow devices. The idea is that but only having few channels in 'fast' mode we can sample those faster?
 *
 * At the moment rusEfi does not allow to have more than 16 ADC channels combined. At the moment there is no flexibility to use
 * any ADC pins, only the hardcoded choice of 16 pins.
 *
 * Slow ADC group is used for IAT, CLT, AFR, VBATT etc - this one is currently sampled at 500Hz
 *
 * Fast ADC group is used for MAP, MAF HIP - this one is currently sampled at 10KHz
 *  We need frequent MAP for map_averaging.cpp
 *
 * 10KHz equals one measurement every 3.6 degrees at 6000 RPM
 *
 * @date Jan 14, 2013
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#include "pch.h"

float __attribute__((weak)) getAnalogInputDividerCoefficient(adc_channel_e) {
    return engineConfiguration->analogInputDividerCoefficient;
}

#if HAL_USE_ADC

#include "adc_subscription.h"
#include "AdcConfiguration.h"
#include "mpu_util.h"
#include "periodic_thread_controller.h"
#include "protected_gpio.h"

static AdcChannelMode adcHwChannelEnabled[HW_MAX_ADC_INDEX];

// Board voltage, with divider coefficient accounted for
float getVoltageDivided(const char *msg, adc_channel_e hwChannel) {
	return getVoltage(msg, hwChannel) * getAnalogInputDividerCoefficient(hwChannel);
}

// voltage in MCU universe, from zero to VDD
float getVoltage(const char *msg, adc_channel_e hwChannel) {
	return adcToVolts(getSlowAdcValue(msg, hwChannel));
}

#if EFI_USE_FAST_ADC
AdcDevice::AdcDevice(ADCConversionGroup* hwConfig)
	: m_hwConfig(hwConfig)
{
	m_hwConfig->sqr1 = 0;
	m_hwConfig->sqr2 = 0;
	m_hwConfig->sqr3 = 0;
#if ADC_MAX_CHANNELS_COUNT > 16
	m_hwConfig->sqr4 = 0;
	m_hwConfig->sqr5 = 0;
#endif /* ADC_MAX_CHANNELS_COUNT */
	memset(internalAdcIndexByHardwareIndex, 0xFF, sizeof(internalAdcIndexByHardwareIndex));
}

#endif // EFI_USE_FAST_ADC

static uint32_t slowAdcCounter = 0;

static float mcuTemperature;

float getMCUInternalTemperature() {
	return mcuTemperature;
}

int getSlowAdcValue(const char *msg, adc_channel_e hwChannel) {
	if (!isAdcChannelValid(hwChannel)) {
		warning(ObdCode::CUSTOM_OBD_ANALOG_INPUT_NOT_CONFIGURED, "ADC: %s input is not configured", msg);
		return -1;
	}

	return getSlowAdcSample(hwChannel);
}

#if EFI_USE_FAST_ADC

int AdcDevice::size() const {
	return channelCount;
}

void AdcDevice::init() {
	m_hwConfig->num_channels = size();
}

void AdcDevice::enableChannel(adc_channel_e hwChannel) {
	if ((channelCount + 1) >= ADC_MAX_CHANNELS_COUNT) {
		firmwareError(ObdCode::OBD_PCM_Processor_Fault, "Too many ADC channels configured");
		return;
	}

	// hwChannel = which external pin are we using
	// adcChannelIndex = which ADC channel are we using
	// adcIndex = which index does that get in sampling order
	size_t adcChannelIndex = hwChannel - EFI_ADC_0;
	size_t adcIndex = channelCount++;

	internalAdcIndexByHardwareIndex[hwChannel] = adcIndex;

	if (adcIndex < 6) {
		m_hwConfig->sqr3 |= adcChannelIndex << (5 * adcIndex);
	} else if (adcIndex < 12) {
		m_hwConfig->sqr2 |= adcChannelIndex << (5 * (adcIndex - 6));
	} else if (adcIndex < 18) {
		m_hwConfig->sqr1 |= adcChannelIndex << (5 * (adcIndex - 12));
	}
}

#endif // EFI_USE_FAST_ADC

void waitForSlowAdc(uint32_t lastAdcCounter) {
	// we use slowAdcCounter instead of slowAdc.conversionCount because we need ADC_COMPLETE state
	while (slowAdcCounter <= lastAdcCounter) {
		chThdSleepMilliseconds(1);
	}
}

void updateSlowAdc(efitick_t nowNt) {
	{
		ScopePerf perf(PE::AdcConversionSlow);

		if (!readSlowAnalogInputs()) {
			return;
		}

		// Ask the port to sample the MCU temperature
		mcuTemperature = getMcuTemperature();
	}

	{
		ScopePerf perf(PE::AdcProcessSlow);

		slowAdcCounter++;

		AdcSubscription::UpdateSubscribers(nowNt);

		protectedGpio_check(nowNt);
	}
}

#if EFI_USE_FAST_ADC
extern AdcDevice fastAdc;
#endif // EFI_USE_FAST_ADC

static void addFastAdcChannel(const char* /*name*/, adc_channel_e setting) {
	if (!isAdcChannelValid(setting)) {
		return;
	}

	adcHwChannelEnabled[setting] = AdcChannelMode::Fast;

#if EFI_USE_FAST_ADC
	fastAdc.enableChannel(setting);
#endif
}

void initAdcInputs() {
	efiPrintf("initAdcInputs()");

	memset(adcHwChannelEnabled, 0, sizeof(adcHwChannelEnabled));

	addFastAdcChannel("MAP", engineConfiguration->map.sensor.hwChannel);

#if EFI_INTERNAL_ADC
#if EFI_USE_FAST_ADC
	fastAdc.init();
#endif // EFI_USE_FAST_ADC
#endif
}

#else /* not HAL_USE_ADC */

__attribute__((weak)) float getVoltageDivided(const char*, adc_channel_e) {
	return 0;
}

// voltage in MCU universe, from zero to VDD
__attribute__((weak)) float getVoltage(const char*, adc_channel_e) {
	return 0;
}

#endif
