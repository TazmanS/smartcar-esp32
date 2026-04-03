set(app_sources
	main.c
	app/app_controller.c
	services/wifi_service.c
	services/mqtt_service.c
	services/camera_service.c
	net/stream_server.c
)

idf_component_register(
	SRCS ${app_sources}
	INCLUDE_DIRS "." "../include"
	REQUIRES
		esp_event
		esp_http_server
		esp_netif
		esp_wifi
		mqtt
		nvs_flash
		esp_timer
		esp32-camera
	PRIV_REQUIRES
		driver
)
