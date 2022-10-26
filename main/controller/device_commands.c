#include "esp_err.h"
#include "esp_console.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "device_commands.h"
#include "peripherals/digout.h"
#include "peripherals/digin.h"
#include "model/model.h"
#include "configuration.h"
#include "sensors.h"


static int command_read_sensors(int argc, char **argv);
static int command_read_min_pressure(int argc, char **argv);
static int command_set_min_pressure(int argc, char **argv);
static int command_read_max_pressure(int argc, char **argv);
static int command_set_max_pressure(int argc, char **argv);
static int device_commands_read_inputs(int argc, char **argv);
static int device_commands_read_minimum_pressure_message(int argc, char **argv);
static int device_commands_set_minimum_pressure_message(int argc, char **argv);
static int device_commands_read_maximum_pressure_message(int argc, char **argv);
static int device_commands_set_maximum_pressure_message(int argc, char **argv);


static model_t *model_ref = NULL;


void device_commands_register(model_t *pmodel) {
    model_ref = pmodel;

    const esp_console_cmd_t signal_cmd = {
        .command = "ReadSignals",
        .help    = "Read signals levels",
        .hint    = NULL,
        .func    = &device_commands_read_inputs,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&signal_cmd));

    const esp_console_cmd_t read_sensors = {
        .command = "ReadSensors",
        .help    = "Read the currently read sensors values",
        .hint    = NULL,
        .func    = &command_read_sensors,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&read_sensors));

    const esp_console_cmd_t read_min_pressure = {
        .command = "ReadMinPressure",
        .help    = "Read the configured minimum pressure",
        .hint    = NULL,
        .func    = &command_read_min_pressure,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&read_min_pressure));

    const esp_console_cmd_t set_min_pressure = {
        .command = "SetMinPressure",
        .help    = "Set the configured minimum pressure",
        .hint    = NULL,
        .func    = &command_set_min_pressure,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_min_pressure));

    const esp_console_cmd_t read_max_pressure = {
        .command = "ReadMaxPressure",
        .help    = "Read the configured maximum pressure",
        .hint    = NULL,
        .func    = &command_read_max_pressure,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&read_max_pressure));

    const esp_console_cmd_t set_max_pressure = {
        .command = "SetMaxPressure",
        .help    = "Set the configured maximum pressure",
        .hint    = NULL,
        .func    = &command_set_max_pressure,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_max_pressure));

    const esp_console_cmd_t read_minimum_pressure_message = {
        .command = "ReadMinPressureMessage",
        .help    = "Print the configured minimum pressure warning",
        .hint    = NULL,
        .func    = &device_commands_read_minimum_pressure_message,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&read_minimum_pressure_message));

    const esp_console_cmd_t set_minimum_pressure_message = {
        .command = "SetMinPressureMessage",
        .help    = "Set a new minimum pressure warning",
        .hint    = NULL,
        .func    = &device_commands_set_minimum_pressure_message,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_minimum_pressure_message));

    const esp_console_cmd_t read_maximum_pressure_message = {
        .command = "ReadMaxPressureMessage",
        .help    = "Print the configured maximum pressure warning",
        .hint    = NULL,
        .func    = &device_commands_read_maximum_pressure_message,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&read_maximum_pressure_message));

    const esp_console_cmd_t set_maximum_pressure_message = {
        .command = "SetMaxPressureMessage",
        .help    = "Set a new maximum pressure warning",
        .hint    = NULL,
        .func    = &device_commands_set_maximum_pressure_message,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_maximum_pressure_message));
}


static int command_read_sensors(int argc, char **argv) {
    struct arg_end *end;
    void           *argtable[] = {
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        double temperature = 0;
        double pressure    = 0;

        sensors_read(&temperature, &pressure);

        printf("%4.2f C\n%4.2f mBar\n", temperature, pressure);
    } else {
        arg_print_errors(stdout, end, "Read sensors values");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int command_set_max_pressure(int argc, char **argv) {
    struct arg_end *end;
    struct arg_int *press;
    void           *argtable[] = {
        press = arg_int1(NULL, NULL, "<int>", "Maximum pressure"),
        end   = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        if (configuration_save_maximum_pressure(model_ref, press->ival[0])) {
            printf("Invalid value!\n");
        }
    } else {
        arg_print_errors(stdout, end, "Set maximum pressure");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int command_read_max_pressure(int argc, char **argv) {
    struct arg_end *end;
    void           *argtable[] = {
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        printf("%i mBar\n", model_get_maximum_pressure(model_ref));
    } else {
        arg_print_errors(stdout, end, "Read maximum pressure");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int command_set_min_pressure(int argc, char **argv) {
    struct arg_end *end;
    struct arg_int *press;
    void           *argtable[] = {
        press = arg_int1(NULL, NULL, "<int>", "Minimum pressure"),
        end   = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        if (configuration_save_minimum_pressure(model_ref, press->ival[0])) {
            printf("Invalid value!\n");
        }
    } else {
        arg_print_errors(stdout, end, "Set minimum pressure");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int command_read_min_pressure(int argc, char **argv) {
    struct arg_end *end;
    void           *argtable[] = {
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        printf("%i mBar\n", model_get_minimum_pressure(model_ref));
    } else {
        arg_print_errors(stdout, end, "Read minimum pressure");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int device_commands_read_inputs(int argc, char **argv) {
    struct arg_end *end;
    /* the global arg_xxx structs are initialised within the argtable */
    void *argtable[] = {
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        uint8_t value = (uint8_t)digin_get_inputs();
        printf("Safety=%i\n", (value & 0x01) > 0);
    } else {
        arg_print_errors(stdout, end, "Read device inputs");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int device_commands_read_minimum_pressure_message(int argc, char **argv) {
    struct arg_end *end;
    void           *argtable[] = {
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        char minimum_pressure_message[EASYCONNECT_MESSAGE_SIZE + 1] = {0};
        model_get_minimum_pressure_message(model_ref, minimum_pressure_message);
        printf("%s\n", minimum_pressure_message);
    } else {
        arg_print_errors(stdout, end, "Read minimum pressure message");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int device_commands_set_minimum_pressure_message(int argc, char **argv) {
    struct arg_str *sm;
    struct arg_end *end;
    void           *argtable[] = {
        sm  = arg_str1(NULL, NULL, "<minimum pressure message>", "minimum pressure message"),
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        configuration_save_minimum_pressure_message(model_ref, sm->sval[0]);
    } else {
        arg_print_errors(stdout, end, "Set minimum pressure message");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int device_commands_read_maximum_pressure_message(int argc, char **argv) {
    struct arg_end *end;
    void           *argtable[] = {
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        char maximum_pressure_message[EASYCONNECT_MESSAGE_SIZE + 1] = {0};
        model_get_maximum_pressure_message(model_ref, maximum_pressure_message);
        printf("%s\n", maximum_pressure_message);
    } else {
        arg_print_errors(stdout, end, "Read maximum pressure message");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int device_commands_set_maximum_pressure_message(int argc, char **argv) {
    struct arg_str *sm;
    struct arg_end *end;
    void           *argtable[] = {
        sm  = arg_str1(NULL, NULL, "<maximum pressure message>", "maximum pressure message"),
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        configuration_save_maximum_pressure_message(model_ref, sm->sval[0]);
    } else {
        arg_print_errors(stdout, end, "Set maximum pressure message");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}
