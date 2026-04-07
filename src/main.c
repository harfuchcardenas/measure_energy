#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

#define SLEEP_TIME_MS 500
#define R_SHUNT 0.1f  

/* Devicetree aliases */
#define VOLTAGE_CH DT_ALIAS(voltage_channel)
#define CURRENT_CH DT_ALIAS(current_channel)
static const struct device *adc = DEVICE_DT_GET(DT_ALIAS(my_adc));
static const struct adc_channel_cfg voltage_cfg = ADC_CHANNEL_CFG_DT(VOLTAGE_CH);
static const struct adc_channel_cfg current_cfg = ADC_CHANNEL_CFG_DT(CURRENT_CH);

void main(void)
{
    int ret;
    uint16_t voltage_buf;
    uint16_t current_buf;
    float voltage_v, current_a, power_w;

    int32_t vref_voltage = DT_PROP(VOLTAGE_CH, zephyr_vref_mv);
    int32_t vref_current = DT_PROP(CURRENT_CH, zephyr_vref_mv);

    struct adc_sequence seq_voltage = {
        .channels = BIT(voltage_cfg.channel_id),
        .buffer = &voltage_buf,
        .buffer_size = sizeof(voltage_buf),
        .resolution = DT_PROP(VOLTAGE_CH, zephyr_resolution)
    };

    struct adc_sequence seq_current = {
        .channels = BIT(current_cfg.channel_id),
        .buffer = &current_buf,
        .buffer_size = sizeof(current_buf),
        .resolution = DT_PROP(CURRENT_CH, zephyr_resolution)
    };

    if (!device_is_ready(adc)) {
        printk("ADC device not ready!\n");
        return;
    }

    ret = adc_channel_setup(adc, &voltage_cfg);
    if (ret < 0) {
        printk("Failed to setup voltage channel: %d\n", ret);
        return;
    }

    ret = adc_channel_setup(adc, &current_cfg);
    if (ret < 0) {
        printk("Failed to setup current channel: %d\n", ret);
        return;
    }

    while (1) {
        /* Read voltage */
        ret = adc_read(adc, &seq_voltage);
        if (ret < 0) {
            printk("Failed to read voltage: %d\n", ret);
            k_msleep(SLEEP_TIME_MS);
            continue;
        }

        /* Read current */
        ret = adc_read(adc, &seq_current);
        if (ret < 0) {
            printk("Failed to read current: %d\n", ret);
            k_msleep(SLEEP_TIME_MS);
            continue;
        }

        /* Convert ADC to voltage (V) */
        voltage_v = (float)voltage_buf * vref_voltage / (float)(1 << seq_voltage.resolution) / 1000.0f;
        float shunt_voltage_v = (float)current_buf * vref_current / (float)(1 << seq_current.resolution) / 1000.0f;

        /* Convert shunt voltage to current (A) */
        current_a = shunt_voltage_v / R_SHUNT;

        /* Compute power (W) */
        power_w = voltage_v * current_a;

        printf("Voltage: %.3f V, Current: %.3f A, Power: %.3f W\n",
               voltage_v, current_a, power_w);

        k_msleep(SLEEP_TIME_MS);
    }
}