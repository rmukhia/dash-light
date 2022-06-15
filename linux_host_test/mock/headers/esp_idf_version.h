//
// Created by rmukhia on 13/6/22.
//

#ifndef TEST_APP_ESP_IDF_VERSION_H
#define TEST_APP_ESP_IDF_VERSION_H


/** Major version number (X.x.x) */
#define ESP_IDF_VERSION_MAJOR   4
/** Minor version number (x.X.x) */
#define ESP_IDF_VERSION_MINOR   4
/** Patch version number (x.x.X) */
#define ESP_IDF_VERSION_PATCH   3

/**
 * Macro to convert IDF version number into an integer
 *
 * To be used in comparisons, such as ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
 */
#define ESP_IDF_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

/**
 * Current IDF version, as an integer
 *
 * To be used in comparisons, such as ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
 */
#define ESP_IDF_VERSION  ESP_IDF_VERSION_VAL(ESP_IDF_VERSION_MAJOR, \
                                             ESP_IDF_VERSION_MINOR, \
                                             ESP_IDF_VERSION_PATCH)


#endif //TEST_APP_ESP_IDF_VERSION_H
