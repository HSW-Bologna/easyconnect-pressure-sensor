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
#include "rele.h"


static int device_commands_set_rele(int argc, char **argv);
static int device_commands_read_inputs(int argc, char **argv);
static int device_commands_read_inputs(int argc, char **argv);
static int device_commands_read_feedback(int argc, char **argv);
static int device_commands_set_feedback(int argc, char **argv);


static model_t *model_ref = NULL;


void device_commands_register(model_t *pmodel) {
    model_ref = pmodel;

    const esp_console_cmd_t rele_cmd = {
        .command = "SetRele",
        .help    = "Set rele' level",
        .hint    = NULL,
        .func    = &device_commands_set_rele,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&rele_cmd));

    const esp_console_cmd_t signal_cmd = {
        .command = "ReadSignals",
        .help    = "Read signals levels",
        .hint    = NULL,
        .func    = &device_commands_read_inputs,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&signal_cmd));

    const esp_console_cmd_t read_feedback = {
        .command = "ReadFB",
        .help    = "Print the current device feedback configuration",
        .hint    = NULL,
        .func    = &device_commands_read_feedback,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&read_feedback));

    const esp_console_cmd_t set_feedback = {
        .command = "SetFB",
        .help    = "Set a new feedback configuration",
        .hint    = NULL,
        .func    = &device_commands_set_feedback,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_feedback));
}


static int device_commands_set_rele(int argc, char **argv) {
    struct arg_end *end;
    struct arg_int *rele;
    /* the global arg_xxx structs are initialised within the argtable */
    void *argtable[] = {
        rele = arg_int1(NULL, NULL, "<value>", "Rele' level"),
        end  = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        rele_update(rele->ival[0]);
    } else {
        arg_print_errors(stdout, end, "Set rele' level");
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
        printf("Safety=%i, Signal=%i\n", (value & 0x01) > 0, (value & 0x02) > 0);
    } else {
        arg_print_errors(stdout, end, "Read device inputs");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int device_commands_read_feedback(int argc, char **argv) {
    struct arg_end *end;
    /* the global arg_xxx structs are initialised within the argtable */
    void *argtable[] = {
        end = arg_end(1),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        printf("Feedback direction=%i, Activation attempts=%i, Feedback delay=%i\n",
               model_get_feedback_level(model_ref), model_get_output_attempts(model_ref),
               model_get_feedback_delay(model_ref));
    } else {
        arg_print_errors(stdout, end, "Read feedback parameters");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}


static int device_commands_set_feedback(int argc, char **argv) {
    struct arg_int *dir, *att, *del;
    struct arg_lit *help;
    struct arg_end *end;
    void           *argtable[] = {
        help = arg_litn(NULL, "help", 0, 1, "display this help and exit"),
        dir  = arg_int1(NULL, NULL, "<feedback direction>",
                                  "Direction of the feedback signal (0=active low, 1=active high)"),
        att = arg_int1(NULL, NULL, "<activation attempts>", "Number of attempts to activate the output (1-8)"),
        del = arg_int1(NULL, NULL, "<feedback delay>", "Delay of the feedback check (1-8 seconds)"),
        end = arg_end(8),
    };

    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors == 0) {
        if (help->count > 0) {
            printf("Usage:");
            arg_print_syntax(stdout, argtable, "\n");
            printf("\n");
            arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        } else {
            uint8_t feedback_direction = (uint8_t)dir->ival[0];
            if (feedback_direction > EASYCONNECT_PARAMETER_MAX_FEEDBACK_DIRECTION) {
                printf("Invalid feedback direction value: %i\n", feedback_direction);
            } else {
                configuration_save_feedback_direction(model_ref, feedback_direction);
            }

            uint8_t activation_attempts = (uint8_t)att->ival[0];
            if (activation_attempts > EASYCONNECT_PARAMETER_MAX_ACTIVATION_ATTEMPTS) {
                printf("Invalid activation attempts value: %i\n", activation_attempts);
            } else {
                configuration_save_activation_attempts(model_ref, activation_attempts);
            }

            uint8_t feedback_delay = (uint8_t)del->ival[0];
            if (feedback_delay > EASYCONNECT_PARAMETER_MAX_FEEDBACK_DELAY) {
                printf("Invalid feedback delay value: %i\n", feedback_delay);
            } else {
                configuration_save_feedback_delay(model_ref, feedback_delay);
            }
        }
    } else {
        arg_print_errors(stdout, end, "Set feedback");
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return nerrors ? -1 : 0;
}