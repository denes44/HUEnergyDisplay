import inspect

import esphome.codegen as cg
from esphome import automation
from esphome.components import esp32
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_ON_ERROR,
    CONF_ON_RESPONSE,
    CONF_TIMEOUT,
    CONF_URL,
)

CODEOWNERS = ["@denes44"]
DEPENDENCIES = ["network"]

CONF_REQUEST_HEADERS = "request_headers"
CONF_BUFFER_SIZE_RX = "buffer_size_rx"
CONF_BUFFER_SIZE_TX = "buffer_size_tx"
CONF_MAX_RESPONSE_SIZE = "max_response_size"
CONF_TASK_STACK_SIZE = "task_stack_size"
CONF_KEEP_ALIVE_TIMEOUT = "keep_alive_timeout"

async_http_ns = cg.esphome_ns.namespace("async_http")
AsyncHttpComponent = async_http_ns.class_("AsyncHttpComponent", cg.Component)
AsyncHttpGetAction = async_http_ns.class_("AsyncHttpGetAction", automation.Action)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AsyncHttpComponent),
            cv.Optional(
                CONF_TIMEOUT, default="5s"
            ): cv.positive_time_period_milliseconds,
            # Close the connection when idle for longer, should be below the
            # server's keep-alive timeout. 0s closes it after every request.
            cv.Optional(
                CONF_KEEP_ALIVE_TIMEOUT, default="50s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_BUFFER_SIZE_RX, default=2048): cv.uint16_t,
            cv.Optional(CONF_BUFFER_SIZE_TX, default=1024): cv.uint16_t,
            cv.Optional(CONF_MAX_RESPONSE_SIZE, default="8kB"): cv.validate_bytes,
            cv.Optional(CONF_TASK_STACK_SIZE, default=10240): cv.int_range(
                min=6144, max=32768
            ),
            cv.Optional(CONF_ON_RESPONSE): automation.validate_automation(single=True),
            cv.Optional(CONF_ON_ERROR): automation.validate_automation(single=True),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_timeout(config[CONF_TIMEOUT]))
    cg.add(var.set_keep_alive_timeout(config[CONF_KEEP_ALIVE_TIMEOUT]))
    cg.add(var.set_buffer_size_rx(config[CONF_BUFFER_SIZE_RX]))
    cg.add(var.set_buffer_size_tx(config[CONF_BUFFER_SIZE_TX]))
    cg.add(var.set_max_response_size(config[CONF_MAX_RESPONSE_SIZE]))
    cg.add(var.set_task_stack_size(config[CONF_TASK_STACK_SIZE]))

    esp32.include_builtin_idf_component("esp_http_client")
    esp32.add_idf_sdkconfig_option("CONFIG_MBEDTLS_CERTIFICATE_BUNDLE", True)

    if response_conf := config.get(CONF_ON_RESPONSE):
        await automation.build_automation(
            var.get_response_trigger(),
            [(cg.int_, "status_code"), (cg.std_string_ref, "body")],
            response_conf,
        )
    if error_conf := config.get(CONF_ON_ERROR):
        await automation.build_automation(var.get_error_trigger(), [], error_conf)


ASYNC_HTTP_GET_ACTION_SCHEMA = automation.maybe_conf(
    CONF_URL,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(AsyncHttpComponent),
            cv.Required(CONF_URL): cv.templatable(cv.string),
            cv.Optional(CONF_REQUEST_HEADERS): cv.Schema(
                {cv.string: cv.templatable(cv.string)}
            ),
        }
    ),
)


# The action only queues the request, so it never defers the next action.
# Older ESPHome versions don't know the synchronous= parameter.
_ACTION_KWARGS = (
    {"synchronous": True}
    if "synchronous" in inspect.signature(automation.register_action).parameters
    else {}
)


@automation.register_action(
    "async_http.get",
    AsyncHttpGetAction,
    ASYNC_HTTP_GET_ACTION_SCHEMA,
    **_ACTION_KWARGS,
)
async def async_http_get_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)

    template_ = await cg.templatable(config[CONF_URL], args, cg.std_string)
    cg.add(var.set_url(template_))

    for key, value in config.get(CONF_REQUEST_HEADERS, {}).items():
        template_ = await cg.templatable(value, args, cg.std_string)
        cg.add(var.add_request_header(key, template_))

    return var
