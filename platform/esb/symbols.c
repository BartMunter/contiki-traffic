#include "loader/symbols.h"
const struct symbols symbols[] = {
{"InterruptVectors", (char *)0x0000ffe0},
{"Timer_B1", (char *)0x000042ec},
{"__bss_end", (char *)0x00000808},
{"__bss_start", (char *)0x000002fa},
{"__ctors_end", (char *)0x0000113a},
{"__ctors_start", (char *)0x0000113a},
{"__data_start", (char *)0x00000200},
{"__divmodhi4", (char *)0x0000977e},
{"__dtors_end", (char *)0x0000113a},
{"__dtors_start", (char *)0x0000113a},
{"__stack", (char *)0x00000a00},
{"__udivmodhi4", (char *)0x00009762},
{"__udivmodsi4", (char *)0x000097b4},
{"__umulsi3hw", (char *)0x0000973a},
{"_edata", (char *)0x000002fa},
{"_etext", (char *)0x000097de},
{"_unexpected_1_", (char *)0x0000113a},
{"_vectors_end", (char *)0x00010000},
{"arg_alloc", (char *)0x00001254},
{"arg_free", (char *)0x0000125a},
{"arg_init", (char *)0x00001258},
{"autostart_exit", (char *)0x00001e32},
{"autostart_processes", (char *)0x00000200},
{"autostart_start", (char *)0x00001e02},
{"battery_sensor", (char *)0x000040d4},
{"beep", (char *)0x000044f2},
{"beep_alarm", (char *)0x00004474},
{"beep_beep", (char *)0x000044d8},
{"beep_down", (char *)0x000044fc},
{"beep_long", (char *)0x000045c6},
{"beep_off", (char *)0x00004538},
{"beep_on", (char *)0x00004532},
{"beep_quick", (char *)0x0000459e},
{"beep_spinup", (char *)0x0000453e},
{"button_sensor", (char *)0x00003d7c},
{"cfs_eeprom_process", (char *)0x000002d4},
{"cfs_eeprom_service", (char *)0x000002cc},
{"cfs_find_service", (char *)0x00001e8a},
{"cfs_nullservice", (char *)0x0000024c},
{"clock_delay", (char *)0x00004aba},
{"clock_init", (char *)0x00004a92},
{"clock_seconds", (char *)0x00004ade},
{"clock_set", (char *)0x00004a7c},
{"clock_set_seconds", (char *)0x00004adc},
{"clock_time", (char *)0x00004a76},
{"clock_wait", (char *)0x00004ac0},
{"codeprop_process", (char *)0x0000022e},
{"codeprop_set_rate", (char *)0x00001298},
{"codeprop_start_broadcast", (char *)0x000019bc},
{"codeprop_start_program", (char *)0x000019d8},
{"contiki_esb_main_init_process", (char *)0x00000224},
{"crc16_add", (char *)0x00008cee},
{"ctsrts_rts_clear", (char *)0x00003fb4},
{"ctsrts_rts_set", (char *)0x00003fbc},
{"ctsrts_sensor", (char *)0x00004070},
{"ds1629_init", (char *)0x000047bc},
{"ds1629_start", (char *)0x000047c2},
{"ds1629_temperature", (char *)0x000047c8},
{"eeprom_read", (char *)0x00008702},
{"eeprom_write", (char *)0x0000877e},
{"elfloader_arch_allocate_ram", (char *)0x00006612},
{"elfloader_arch_allocate_rom", (char *)0x00006618},
{"elfloader_arch_relocate", (char *)0x000066a0},
{"elfloader_arch_write_text", (char *)0x00006630},
{"elfloader_autostart_processes", (char *)0x0000077a},
{"elfloader_init", (char *)0x0000529c},
{"elfloader_load", (char *)0x000052da},
{"elfloader_unknown", (char *)0x0000075c},
{"esb_sensors_init", (char *)0x000085d4},
{"esb_sensors_off", (char *)0x000085ea},
{"esb_sensors_on", (char *)0x000085e2},
{"etimer_adjust", (char *)0x000020a0},
{"etimer_expiration_time", (char *)0x000020ba},
{"etimer_expired", (char *)0x000020ac},
{"etimer_next_expiration_time", (char *)0x000020d4},
{"etimer_pending", (char *)0x000020c8},
{"etimer_process", (char *)0x00000254},
{"etimer_request_poll", (char *)0x00002030},
{"etimer_reset", (char *)0x0000207c},
{"etimer_restart", (char *)0x0000208e},
{"etimer_set", (char *)0x0000206a},
{"etimer_start_time", (char *)0x000020c4},
{"etimer_stop", (char *)0x000020e6},
{"flash_clear", (char *)0x00008852},
{"flash_done", (char *)0x00008842},
{"flash_setup", (char *)0x0000880e},
{"flash_write", (char *)0x00008884},
{"hc_compress", (char *)0x00008d30},
{"hc_inflate", (char *)0x00008db8},
{"hc_init", (char *)0x00008d2e},
{"htons", (char *)0x0000318c},
{"init_apps", (char *)0x00004848},
{"init_lowlevel", (char *)0x00004832},
{"init_net", (char *)0x0000484a},
{"ir_data", (char *)0x000042b0},
{"ir_event_received", (char *)0x0000075a},
{"ir_irq", (char *)0x000043cc},
{"ir_poll", (char *)0x000042b6},
{"ir_process", (char *)0x00000292},
{"ir_send", (char *)0x00004216},
{"irq_adc", (char *)0x00003b34},
{"irq_adc12_activate", (char *)0x00003bfc},
{"irq_adc12_active", (char *)0x00003ca8},
{"irq_adc12_deactivate", (char *)0x00003c52},
{"irq_init", (char *)0x00003b5e},
{"irq_p1", (char *)0x00003adc},
{"irq_p2", (char *)0x00003b08},
{"leds_arch_get", (char *)0x00004f1a},
{"leds_arch_init", (char *)0x00004f0c},
{"leds_arch_set", (char *)0x00004f48},
{"leds_blink", (char *)0x00004ce2},
{"leds_get", (char *)0x00004cf8},
{"leds_green", (char *)0x00004ed0},
{"leds_init", (char *)0x00004cb4},
{"leds_invert", (char *)0x00004e28},
{"leds_off", (char *)0x00004d94},
{"leds_on", (char *)0x00004d00},
{"leds_red", (char *)0x00004ef8},
{"leds_toggle", (char *)0x00004e22},
{"leds_yellow", (char *)0x00004ee4},
{"lpm_off", (char *)0x00004afc},
{"lpm_on", (char *)0x00004af6},
{"lpm_status", (char *)0x000002ba},
{"main", (char *)0x00001210},
{"me_decode16", (char *)0x000088c0},
{"me_decode8", (char *)0x000088de},
{"me_decode_tab", (char *)0x00008aee},
{"me_encode", (char *)0x000088b6},
{"me_encode_tab", (char *)0x000088ee},
{"me_valid", (char *)0x000088e6},
{"me_valid_tab", (char *)0x00008bee},
{"memchr", (char *)0x00009462},
{"memcmp", (char *)0x000094d4},
{"memcpy", (char *)0x00009508},
{"memmove", (char *)0x000095e4},
{"memset", (char *)0x000096c0},
{"msp430_cpu_init", (char *)0x00004a18},
{"msp430_init_dco", (char *)0x00004996},
{"node_id", (char *)0x00006a46},
{"pir_sensor", (char *)0x00003df8},
{"printf", (char *)0x00008e52},
{"process_alloc_event", (char *)0x00001ac4},
{"process_current", (char *)0x0000023a},
{"process_exit", (char *)0x00001c12},
{"process_init", (char *)0x00001c1c},
{"process_list", (char *)0x00000238},
{"process_poll", (char *)0x00001dba},
{"process_post", (char *)0x00001d48},
{"process_post_synch", (char *)0x00001da8},
{"process_run", (char *)0x00001d0c},
{"process_start", (char *)0x00001ace},
{"procinit", (char *)0x00000216},
{"procinit_init", (char *)0x00001dd0},
{"putchar", (char *)0x0000125c},
{"puts", (char *)0x00009434},
{"radio_off", (char *)0x00007f22},
{"radio_on", (char *)0x00007f30},
{"radio_sensor", (char *)0x00003f82},
{"radio_sensor_signal", (char *)0x00000758},
{"random_init", (char *)0x00006716},
{"random_rand", (char *)0x0000671c},
{"recir_getAddress", (char *)0x00004262},
{"recir_getCode", (char *)0x00004258},
{"recir_getError", (char *)0x00004294},
{"recir_getToggle", (char *)0x0000427c},
{"rs232_init", (char *)0x000048de},
{"rs232_print", (char *)0x0000496e},
{"rs232_rx_usart1", (char *)0x000048a0},
{"rs232_send", (char *)0x00004904},
{"rs232_set_input", (char *)0x0000498a},
{"rs232_set_speed", (char *)0x00004916},
{"sensors", (char *)0x00000204},
{"sensors_add_irq", (char *)0x000038fa},
{"sensors_changed", (char *)0x0000396a},
{"sensors_event", (char *)0x00000755},
{"sensors_find", (char *)0x0000397e},
{"sensors_first", (char *)0x00003958},
{"sensors_flags", (char *)0x000005ba},
{"sensors_handle_irq", (char *)0x0000391a},
{"sensors_next", (char *)0x0000395e},
{"sensors_process", (char *)0x00000288},
{"sensors_remove_irq", (char *)0x0000390a},
{"sensors_select", (char *)0x000039d4},
{"sensors_selecting_proc", (char *)0x000005a8},
{"sensors_unselect", (char *)0x00003a04},
{"service_find", (char *)0x00007e22},
{"service_register", (char *)0x00007db6},
{"service_remove", (char *)0x00007dde},
{"slip_arch_writeb", (char *)0x00004990},
{"slip_input_byte", (char *)0x00006972},
{"slip_process", (char *)0x000002de},
{"slip_send", (char *)0x0000674c},
{"sound_pause", (char *)0x00000756},
{"sound_sensor", (char *)0x00003f1e},
{"splhigh_", (char *)0x00004a26},
{"splx_", (char *)0x00004a2e},
{"strcmp", (char *)0x0000947e},
{"strncmp", (char *)0x000094a2},
{"symbols", (char *)0x0000793a},
{"symtab_lookup", (char *)0x000066e4},
{"tcp_attach", (char *)0x0000329c},
{"tcp_connect", (char *)0x0000321a},
{"tcp_listen", (char *)0x00003270},
{"tcp_unlisten", (char *)0x00003244},
{"tcpip_event", (char *)0x00000754},
{"tcpip_input", (char *)0x00003444},
{"tcpip_output", (char *)0x00003206},
{"tcpip_poll_tcp", (char *)0x00003464},
{"tcpip_poll_udp", (char *)0x00003456},
{"tcpip_process", (char *)0x00000266},
{"tcpip_set_forwarding", (char *)0x000031ba},
{"tcpip_uipcall", (char *)0x00003472},
{"temperature_sensor", (char *)0x00004154},
{"timer_expired", (char *)0x00001ece},
{"timer_reset", (char *)0x00001eb6},
{"timer_restart", (char *)0x00001ebe},
{"timer_set", (char *)0x00001ea2},
{"timera1", (char *)0x00004a32},
{"tr1001_clear_packets", (char *)0x00008594},
{"tr1001_default_rxhandler_pt", (char *)0x00008086},
{"tr1001_drv_process", (char *)0x000002ec},
{"tr1001_drv_request_poll", (char *)0x00007eb8},
{"tr1001_drv_send", (char *)0x00007ec2},
{"tr1001_drv_set_slip_dump", (char *)0x00007ed4},
{"tr1001_init", (char *)0x0000800a},
{"tr1001_packets_dropped", (char *)0x0000858e},
{"tr1001_packets_ok", (char *)0x00008588},
{"tr1001_poll", (char *)0x000084c2},
{"tr1001_rxbuf", (char *)0x0000077c},
{"tr1001_rxhandler", (char *)0x0000805e},
{"tr1001_rxstate", (char *)0x000002f8},
{"tr1001_send", (char *)0x00008430},
{"tr1001_set_speed", (char *)0x0000852c},
{"tr1001_set_txpower", (char *)0x00007fb4},
{"tr1001_sstrength", (char *)0x00008582},
{"tr1001_sstrength_value", (char *)0x0000859e},
{"udp_attach", (char *)0x000032a8},
{"udp_broadcast_new", (char *)0x000032d2},
{"udp_new", (char *)0x000032b4},
{"uip_acc32", (char *)0x000005c8},
{"uip_add32", (char *)0x0000213c},
{"uip_appdata", (char *)0x000005d0},
{"uip_broadcast_addr", (char *)0x0000212e},
{"uip_buf", (char *)0x00000670},
{"uip_chksum", (char *)0x000021e4},
{"uip_conn", (char *)0x000005d2},
{"uip_connect", (char *)0x000022c6},
{"uip_conns", (char *)0x000005d4},
{"uip_draddr", (char *)0x00000664},
{"uip_ethaddr", (char *)0x0000025e},
{"uip_flags", (char *)0x000005cc},
{"uip_fw_default", (char *)0x00003840},
{"uip_fw_forward", (char *)0x00003772},
{"uip_fw_init", (char *)0x0000356e},
{"uip_fw_output", (char *)0x0000371c},
{"uip_fw_periodic", (char *)0x00003846},
{"uip_fw_process", (char *)0x0000027e},
{"uip_fw_register", (char *)0x00003834},
{"uip_fw_service", (char *)0x00000276},
{"uip_hostaddr", (char *)0x0000066c},
{"uip_init", (char *)0x0000225a},
{"uip_ipchksum", (char *)0x000021f6},
{"uip_len", (char *)0x000005c4},
{"uip_listen", (char *)0x000024d4},
{"uip_listenports", (char *)0x00000654},
{"uip_log", (char *)0x0000127a},
{"uip_netmask", (char *)0x00000668},
{"uip_process", (char *)0x00002536},
{"uip_sappdata", (char *)0x000005c6},
{"uip_send", (char *)0x00003190},
{"uip_setipid", (char *)0x00002136},
{"uip_slen", (char *)0x000006fe},
{"uip_tcpchksum", (char *)0x00002250},
{"uip_udp_conn", (char *)0x000005ce},
{"uip_udp_conns", (char *)0x00000700},
{"uip_udp_new", (char *)0x000023ca},
{"uip_unlisten", (char *)0x000024ac},
{"vector_ffe2", (char *)0x00003b08},
{"vector_ffe6", (char *)0x000048a0},
{"vector_ffe8", (char *)0x00003adc},
{"vector_ffea", (char *)0x00004a32},
{"vector_ffee", (char *)0x00003b34},
{"vector_fff2", (char *)0x0000805e},
{"vector_fff8", (char *)0x000042ec},
{"vib_sensor", (char *)0x00003e86},
{"vuprintf", (char *)0x00008ec8},
{(void *)0, 0} };
