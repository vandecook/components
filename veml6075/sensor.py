import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_INTEGRATION_TIME,
    DEVICE_CLASS_EMPTY,
    ICON_BRIGHTNESS_5,
    STATE_CLASS_MEASUREMENT,
    UNIT_MILLISECOND,
)

DEPENDENCIES = ["i2c"]

veml6075_ns = cg.esphome_ns.namespace("veml6075")
VEML6075Component = veml6075_ns.class_(
    "VEML6075Component", cg.PollingComponent, i2c.I2CDevice
)

CONF_HIGH_DYNAMIC = "high_dynamic"

CONF_UVA = "uva"
CONF_UVB = "uvb"
CONF_UV_INDEX = "uv_index"

CONF_RAW_1 = "raw1"
CONF_RAW_2 = "raw2"
CONF_RAW_3 = "raw3"
CONF_RAW_4 = "raw4"

UNIT_COUNTS = "counts"
UNIT_UVI = "UVI"


IntegrationTime = veml6075_ns.enum("veml6075_integrationTime")
INTEGRATION_TIMES = {
    50: IntegrationTime.VEML6075_50MS,
    100: IntegrationTime.VEML6075_100MS,
    200: IntegrationTime.VEML6075_200MS,
    400: IntegrationTime.VEML6075_400MS,
    800: IntegrationTime.VEML6075_800MS,
}


def validate_integration_time(value):
    value = cv.positive_time_period_milliseconds(value).total_milliseconds
    return cv.enum(INTEGRATION_TIMES, int=True)(value)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(VEML6075Component),
            cv.Optional(CONF_INTEGRATION_TIME, default="100ms"): validate_integration_time,
            cv.Optional(CONF_HIGH_DYNAMIC, default=False): cv.boolean,
            cv.Optional(CONF_UVA): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_BRIGHTNESS_5,
                accuracy_decimals=6,
            ),
            cv.Optional(CONF_UVB): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_BRIGHTNESS_5,
                accuracy_decimals=6,
            ),
            cv.Optional(CONF_UV_INDEX): sensor.sensor_schema(
                unit_of_measurement=UNIT_UVI,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_BRIGHTNESS_5,
                accuracy_decimals=6,
            ),
            cv.Optional(CONF_RAW_1): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_RAW_2): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_RAW_3): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
                accuracy_decimals=2,
            ),
            cv.Optional(CONF_RAW_4): sensor.sensor_schema(
                unit_of_measurement=UNIT_COUNTS,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
                accuracy_decimals=2,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x10)),
)



async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_UVA in config:
        sens = await sensor.new_sensor(config[CONF_UVA])
        cg.add(var.set_uva_sensor(sens))

    if CONF_UVB in config:
        sens = await sensor.new_sensor(config[CONF_UVB])
        cg.add(var.set_uvb_sensor(sens))

    if CONF_UV_INDEX in config:
        sens = await sensor.new_sensor(config[CONF_UV_INDEX])
        cg.add(var.set_uv_index_sensor(sens))

    if CONF_RAW_1 in config:
        sens = await sensor.new_sensor(config[CONF_RAW_1])
        cg.add(var.set_raw1_sens(sens))
    
    if CONF_RAW_2 in config:
        sens = await sensor.new_sensor(config[CONF_RAW_2])
        cg.add(var.set_raw2_sens(sens))

    if CONF_RAW_3 in config:
        sens = await sensor.new_sensor(config[CONF_RAW_3])
        cg.add(var.set_raw3_sens(sens))

    if CONF_RAW_4 in config:
        sens = await sensor.new_sensor(config[CONF_RAW_4])
        cg.add(var.set_raw4_sens(sens))

    cg.add(var.set_integration_time(config[CONF_INTEGRATION_TIME]))
    cg.add(var.set_high_dynamic(config[CONF_HIGH_DYNAMIC]))



