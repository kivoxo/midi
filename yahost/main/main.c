/**
 * Copyright (c) 13 Jan 2026 kivoxo / xenddorf
 * All rights reserved
 */
 
//Audio OUT: 0x07
//Audio IN: 0x86
//MIDI OUT: 0x03
//MIDI IN: 0x82

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "usb/usb_host.h"
#include "usb/usb_types_ch9.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "os/os_mbuf.h"

// ================= TAG =================
static const char *TAG = "MIDI_BLE";

// ================= BLE STATE =================
static uint8_t own_addr_type;
static uint16_t conn_handle = 0;
static uint16_t char_handle;
static bool ble_ready = false;
static bool subscribed = false;
                                                                            
// ================= USB STATE =================

//
typedef struct {
	uint8_t cin;
    uint8_t status;
    uint8_t note;
    uint8_t vel;
} midi_evt_t;
typedef struct {
    uint8_t data[4];
} midi_pkt_t;

typedef struct {
    uint8_t ep_in;
    uint8_t ep_out;
    bool ready;
} midi_usb_t;
//
static usb_host_client_handle_t client;
static usb_device_handle_t midi_device = NULL;
static usb_transfer_t *midi_transfer = NULL;
static bool midi_ready = false;
QueueHandle_t midi_queue;
static midi_usb_t midi = {0};
static void advertise(void);

static void log_midi_notes(const uint8_t *buf, size_t len) {
    for (int i = 0; i + 3 < len; i += 4) {

        const uint8_t *pkt = &buf[i];

        uint8_t cin   = pkt[0];
        uint8_t status= pkt[1];
        uint8_t note  = pkt[2];
        uint8_t vel   = pkt[3];

        // filtre NOTE ON / OFF
        switch (status & 0xF0) {

            case 0x90: // NOTE ON
                if (vel > 0) {
                    ESP_LOGI(TAG, "NOTE ON  : %d vel=%d", note, vel);
                } else {
                    ESP_LOGI(TAG, "NOTE OFF : %d", note);
                }
                break;

            case 0x80: // NOTE OFF
                ESP_LOGI(TAG, "NOTE OFF : %d", note);
                break;

            default:
                ESP_LOGI(TAG, "MIDI RAW : %02X %d %d",
                         status, note, vel);
                break;
        }
    }
}

static void midi_out_cb(usb_transfer_t *transfer) {
    // juste libération implicite
    if (transfer->status != USB_TRANSFER_STATUS_COMPLETED) {
        ESP_LOGW(TAG, "MIDI OUT error: %d", transfer->status);
    }

    usb_host_transfer_free(transfer);
}

static void midi_send_note_on(uint8_t note, uint8_t velocity) {
    uint8_t packet[4];
    packet[0] = 0x09;        // USB MIDI CIN (Note On)
    packet[1] = 0x90;        // Note On channel 1
    packet[2] = note;
    packet[3] = velocity;
    usb_transfer_t *t = NULL;
    if (usb_host_transfer_alloc(64, 0, &t) == ESP_OK) {
        t->device_handle = midi_device;
        t->bEndpointAddress = midi.ep_out;
        t->num_bytes = 4;
        t->callback = midi_out_cb;
		memcpy(t->data_buffer, packet, 4);
		esp_err_t err = usb_host_transfer_submit(t);
		if (err != ESP_OK) {
		    usb_host_transfer_free(t);
		}
    }
}

static void midi_send_note_off(uint8_t note) {
    uint8_t packet[4] = {
        0x08,
        0x80,
        note,
        0
    };
    usb_transfer_t *t;
    if (usb_host_transfer_alloc(64, 0, &t) == ESP_OK) {
        t->device_handle = midi_device;
        t->bEndpointAddress = midi.ep_out;
        t->num_bytes = 4;
        t->callback = midi_out_cb;

		memcpy(t->data_buffer, packet, 4);

		esp_err_t err = usb_host_transfer_submit(t);

		if (err != ESP_OK) {
		    usb_host_transfer_free(t);
		}
    }
}

static void ble_send_text(const char *txt) {
    if (!ble_ready || conn_handle == 0 || char_handle == 0 || !subscribed) {
		ESP_LOGI(TAG, "BL msg not send");
		if(!ble_ready){
			ESP_LOGI(TAG, "ble not ready");
		}
		if(!conn_handle){
			ESP_LOGI(TAG, "conn_handle not ready");
		}
		if(!char_handle){
			ESP_LOGI(TAG, "char_handle not ready");
		}
		if(!subscribed){
			ESP_LOGI(TAG, "subscribed not ready");
		}
        return;
    }
    struct os_mbuf *om = ble_hs_mbuf_from_flat(txt, strlen(txt));
    if (!om)  {
		ESP_LOGI(TAG, "BL msg not om");
		return;
	}
  	int rc =  ble_gatts_notify_custom(conn_handle, char_handle, om);
	if (rc != 0) {
	    os_mbuf_free_chain(om);
	}
    ESP_LOGI(TAG, "BLE: %s", txt);
}

static void midi_blt_queue_task(void *arg) {
     midi_pkt_t pkt;
	 struct os_mbuf *om;
     while (1) {
         if (xQueueReceive(midi_queue, &pkt, portMAX_DELAY)) {
             om = ble_hs_mbuf_from_flat(pkt.data, 4);
             if (om) {
                 ble_gatts_notify_custom(conn_handle, char_handle, om);
             }
         }
     }
}

static void midi_transfer_cb(usb_transfer_t *transfer) {
	//    ESP_LOGI(TAG, "CB status=%d actual=%d", transfer->status, transfer->actual_num_bytes);
    if (transfer->status == USB_TRANSFER_STATUS_COMPLETED &&
        transfer->actual_num_bytes > 0) {
			uint8_t *buf = transfer->data_buffer;
			for (int i = 0; i + 3 < transfer->actual_num_bytes; i += 4) {
				if(buf[i+1]==0xFE) {
					continue;
				}
				midi_pkt_t pkt;
			    memcpy(pkt.data, &buf[i], 4);
			    xQueueSend(midi_queue, &pkt, 0);
			}
    } else if (transfer->status != USB_TRANSFER_STATUS_COMPLETED) {
        // NORMAL USB idle (NAK)
        ESP_LOGI(TAG, "USB idle (NAK)");
    } else if (transfer->actual_num_bytes == 0) {
        // NORMAL empty transfer
        ESP_LOGI(TAG, "Empty transfer");
    }
    else {
        ESP_LOGI(TAG, "USB unexpected status=%d", transfer->status);
    }
    // ALWAYS resubmit
    esp_err_t err = usb_host_transfer_submit(transfer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "resubmit failed: %d", err);
    }
}

static esp_err_t start_midi_stream(usb_device_handle_t dev) {
	ESP_LOGI(TAG, "start_midi_stream");
    if (midi.ep_in == 0) {
        ESP_LOGE(TAG, "=========================> NO EP IN");
        return ESP_FAIL;
    }
	int max = 64;
    esp_err_t err = usb_host_transfer_alloc(max, 0, &midi_transfer);
    if (err != ESP_OK){
		ESP_LOGE(TAG, "=========================>  usb_host_transfer_alloc KO ");
		return err;
	} 
    midi_transfer->device_handle = dev;
    midi_transfer->bEndpointAddress = 0x82;//midi.ep_in;
    midi_transfer->callback = midi_transfer_cb;
    midi_transfer->num_bytes =max;
    err = usb_host_transfer_submit(midi_transfer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "submit failed");
        return err;
    }
    ESP_LOGI(TAG, "MIDI RX STARTED on 0x%02X", midi.ep_in);
    return ESP_OK;
}

static esp_err_t setup_and_find_midi(usb_device_handle_t dev) {
    const usb_config_desc_t *config;
    ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(dev, &config));
    const uint8_t *p = (const uint8_t *)config;
    int offset = 0;
    int midi_intf = -1;
    bool is_midi = false;
    midi.ep_in = 0;
    midi.ep_out = 0;
    ESP_LOGI(TAG, "===== MIDI SCAN =====");
    while (offset < config->wTotalLength) {
        const usb_standard_desc_t *desc = (const usb_standard_desc_t *)(p + offset);
        if (desc->bDescriptorType == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
            const usb_intf_desc_t *intf = (const usb_intf_desc_t *)desc;
            is_midi =(intf->bInterfaceClass == USB_CLASS_AUDIO && intf->bInterfaceSubClass == 0x03);
            ESP_LOGI(TAG, "INTF %d class=%02X sub=%02X %s",intf->bInterfaceNumber,intf->bInterfaceClass,intf->bInterfaceSubClass,is_midi ? "MIDI" : "");			 
            if (is_midi && midi_intf < 0) {
                midi_intf = intf->bInterfaceNumber;
                esp_err_t err = usb_host_interface_claim( client, dev, midi_intf, 0);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "claim failed: %s",
                             esp_err_to_name(err));
                    return err;
                }
            } 
        }

        else if (desc->bDescriptorType == USB_B_DESCRIPTOR_TYPE_ENDPOINT) {
            const usb_ep_desc_t *ep = (const usb_ep_desc_t *)desc;	
			 ESP_LOGI(TAG,
			          "EP addr=0x%02X  MPS=%d attr=0x%02X is_midi=%d",
			          ep->bEndpointAddress,
					  ep->wMaxPacketSize,
			          ep->bmAttributes,
			          is_midi);
            if (!is_midi) {
                offset += desc->bLength;
                continue;
            }
			if ((ep->bEndpointAddress & 0x80)) {
				if((ep->bEndpointAddress == 0x82 )) {
				//	if(  ((ep->bmAttributes & 0x03) == USB_XFER_BULK)){
				    midi.ep_in = ep->bEndpointAddress;
					ESP_LOGI(TAG, "MIDI IN: 0x%02x", midi.ep_in);
				} else {
					ESP_LOGI(TAG, "MIDI other IN: 0x%02x", ep->bEndpointAddress);
				}
			} else 	if (!(ep->bEndpointAddress & 0x80)) {
				uint8_t ep_addr  = ep->bEndpointAddress;
				// priorité Yamaha standard = OUT 0x01 ou 0x03
				if (ep_addr  == 0x01 || ep_addr  == 0x03) {
				    midi.ep_out = ep->bEndpointAddress;
					ESP_LOGI(TAG, "MIDI OUT: 0x%02x", midi.ep_out);
				} else {
					ESP_LOGI(TAG, "MIDI other OUT: 0x%02x", ep->bEndpointAddress);
				}
			}	
        }
        offset += desc->bLength;
    }
	ESP_LOGI(TAG, "FINAL IN=0x%02X OUT=0x%02X", midi.ep_in, midi.ep_out);
    return (midi.ep_in && midi.ep_out) ? ESP_OK : ESP_FAIL;
}

static void device_event_cb(const usb_host_client_event_msg_t *event, void *arg) {
    if (event->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        ESP_LOGI(TAG, "USB device connected");
        uint8_t addr = event->new_dev.address;
        usb_device_handle_t dev_hdl;
        ESP_ERROR_CHECK(usb_host_device_open(client, addr, &dev_hdl));
        usb_device_info_t info;
        ESP_ERROR_CHECK(usb_host_device_info(dev_hdl, &info));
		const usb_device_desc_t *dev_desc = NULL;
		ESP_ERROR_CHECK(
		    usb_host_get_device_descriptor(dev_hdl, &dev_desc)
		);
		ESP_LOGI(TAG, "VID:PID = %04x:%04x",
		         dev_desc->idVendor,
		         dev_desc->idProduct);
        midi_device = dev_hdl;
		ble_send_text("USB DEVICE CONNECTED");
        if (setup_and_find_midi(midi_device) == ESP_OK) {
			if (start_midi_stream(midi_device) == ESP_OK) {
			    midi_ready = true;
			} else {
			    ESP_LOGE(TAG, "FAILED TO START MIDI STREAM");
				midi_ready = false;
			}
        }
    } else if (event->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        ESP_LOGI(TAG, "USB device removed");
		ble_send_text("USB device removed");
        // Correct union field here:
        usb_device_handle_t dev_hdl = event->dev_gone.dev_hdl;
        if (midi_device == dev_hdl) {
            midi_device = NULL;
            midi_ready = false;
        }
    } else if (event->event == USB_HOST_CLIENT_EVENT_DEV_RESUMED) {
		ESP_LOGI(TAG, "USB device RESUMED");
		ble_send_text("USB device RESUMED");
	} else {
		ESP_LOGI(TAG, "USB device EVENT");
			ble_send_text("USB device EVENT");
	}
	
}

static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg) {
	ESP_LOGI(TAG, "USB HOST: Event received");
}

static void usb_host_task(void *arg) {
    usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = device_event_cb,
            .callback_arg = NULL,
        }
    };

    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &client));
    ESP_LOGI(TAG, "USB host started");
    while (1) {
		uint32_t event_flags;
		// 1. Global USB system events
		esp_err_t err = usb_host_lib_handle_events(200, &event_flags);
		if (err != ESP_OK) {
			//ESP_LOGE(TAG, "usb_host_lib_handle_events ko");
		}
    }
}

static void usb_midi_in_task(void *arg) {
    while (1) {
		//  Client-specific events
		esp_err_t	 err = usb_host_client_handle_events(client, 10);
		if (err != ESP_OK) {
			//ESP_LOGE(TAG, "usb_host_client_handle_events ko");
		}
    }
}

static int gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
		ESP_LOGI(TAG, "BLE_GAP_EVENT_CONNECT");
            conn_handle = event->connect.conn_handle;
            break;
        case BLE_GAP_EVENT_DISCONNECT:
		ESP_LOGI(TAG, "BLE_GAP_EVENT_DISCONNECT");
            conn_handle = 0;
            subscribed = false;
			vTaskDelay(pdMS_TO_TICKS(200));
			advertise();   // 🔥 IMPORTANT : restart advertising
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
		    subscribed = event->subscribe.cur_notify;
			if(subscribed){
				ESP_LOGI(TAG, "BLE_GAP_EVENT_SUBSCRIBE subscribed ");
			}else {
				ESP_LOGI(TAG, "BLE_GAP_EVENT_SUBSCRIBE not subscribed");
			}
			break;
        default:
            break;
    }
    return 0;
}

static void advertise(void) {
    struct ble_gap_adv_params adv = {0};
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    const char *name = "ESP32_MIDI";
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);
    adv.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv, gap_event, NULL);
}

static int gatt_access(uint16_t conn,
                       uint16_t attr,
                       struct ble_gatt_access_ctxt *ctxt,
                       void *arg) {
    const char msg[] = "HI";
    os_mbuf_append(ctxt->om, msg, strlen(msg));
    return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0xFFF0),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(0xFFF1),
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .access_cb = gatt_access,
                .val_handle = &char_handle,
            },
            {0}
        }
    },
    {0}
};

static void on_sync(void) {
    ble_hs_id_infer_auto(0, &own_addr_type);
    advertise();
    ble_ready = true;
}

static void ble_host_task(void *param) {
    nimble_port_run();
}

static void midi_test_task(void *arg) {
    while (1) {
        if (midi_ready && midi.ep_out != 0) {
        	midi_send_note_on(60, 100);
          	vTaskDelay(pdMS_TO_TICKS(300));
          	midi_send_note_off(60);
          	vTaskDelay(pdMS_TO_TICKS(700));
			
			midi_send_note_on(61, 100);
			vTaskDelay(pdMS_TO_TICKS(300));
			midi_send_note_off(61);
			vTaskDelay(pdMS_TO_TICKS(700));
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

void app_main(void) {
	midi_queue = xQueueCreate(32, sizeof(midi_evt_t));
	if (!midi_queue) {
	    ESP_LOGE(TAG, "Failed to create MIDI queue");
	}
    nvs_flash_init();
    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_svc_gap_device_name_set("ESP32_MIDI");
    ble_hs_cfg.sync_cb = on_sync;
    nimble_port_freertos_init(ble_host_task);
	usb_host_config_t config = {};
	config.intr_flags = ESP_INTR_FLAG_LEVEL1;
	ESP_LOGI(TAG, "Installing USB host...");
	ESP_ERROR_CHECK(usb_host_install(&config));
	ESP_LOGI(TAG, "USB host installed");
	xTaskCreate(usb_host_task, "usb", 8192, NULL, 5, NULL);
	xTaskCreate(usb_midi_in_task, "usb", 8192, NULL, 5, NULL);
	//xTaskCreate(midi_test_task,   "midi_test_task", 4096, NULL, 5,NULL);
	xTaskCreate(midi_blt_queue_task, "midi_queue_task", 4096, NULL, 8, NULL);
}