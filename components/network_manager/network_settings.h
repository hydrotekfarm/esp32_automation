/*
 * network_settings.h
 *
 *  Created on: Dec 23, 2020
 *      Author: rnagp
 */

#ifndef COMPONENTS_NETWORK_MANAGER_NETWORK_SETTINGS_H_
#define COMPONENTS_NETWORK_MANAGER_NETWORK_SETTINGS_H_

struct Network_Settings {
	char wifi_ssid[50];
	char wifi_pw[50];
	char broker_ip[20];
	char device_id[5];
};

#endif /* COMPONENTS_NETWORK_MANAGER_NETWORK_SETTINGS_H_ */

struct Network_Settings network_settings;
