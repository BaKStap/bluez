/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Nokia Corporation
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdlib.h>
#include <glib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/uuid.h>
#include <bluetooth/sdp.h>

#include "gattrib.h"
#include "gatt.h"
#include "btio.h"
#include "gatttool.h"

/* Minimum MTU for ATT connections */
#define ATT_MIN_MTU_LE		23
#define ATT_MIN_MTU_L2CAP	48

GIOChannel *gatt_connect(const gchar *src, const gchar *dst,
				const gchar *sec_level, int psm, int mtu,
				BtIOConnect connect_cb)
{
	GIOChannel *chan;
	bdaddr_t sba, dba;
	GError *err = NULL;
	BtIOSecLevel sec;
	int minimum_mtu;

	/* This check is required because currently setsockopt() returns no
	 * errors for MTU values smaller than the allowed minimum. */
	minimum_mtu = psm ? ATT_MIN_MTU_L2CAP : ATT_MIN_MTU_LE;
	if (mtu != 0 && mtu < minimum_mtu) {
		g_printerr("MTU cannot be smaller than %d\n", minimum_mtu);
		return NULL;
	}

	/* Remote device */
	if (dst == NULL) {
		g_printerr("Remote Bluetooth address required\n");
		return NULL;
	}
	str2ba(dst, &dba);

	/* Local adapter */
	if (src != NULL) {
		if (!strncmp(src, "hci", 3))
			hci_devba(atoi(src + 3), &sba);
		else
			str2ba(src, &sba);
	} else
		bacpy(&sba, BDADDR_ANY);

	if (strcmp(sec_level, "medium") == 0)
		sec = BT_IO_SEC_MEDIUM;
	else if (strcmp(sec_level, "high") == 0)
		sec = BT_IO_SEC_HIGH;
	else
		sec = BT_IO_SEC_LOW;

	if (psm == 0)
		chan = bt_io_connect(BT_IO_L2CAP, connect_cb, NULL, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &sba,
				BT_IO_OPT_DEST_BDADDR, &dba,
				BT_IO_OPT_CID, GATT_CID,
				BT_IO_OPT_OMTU, mtu,
				BT_IO_OPT_SEC_LEVEL, sec,
				BT_IO_OPT_INVALID);
	else
		chan = bt_io_connect(BT_IO_L2CAP, connect_cb, NULL, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &sba,
				BT_IO_OPT_DEST_BDADDR, &dba,
				BT_IO_OPT_PSM, psm,
				BT_IO_OPT_OMTU, mtu,
				BT_IO_OPT_SEC_LEVEL, sec,
				BT_IO_OPT_INVALID);

	if (err) {
		g_printerr("%s\n", err->message);
		g_error_free(err);
		return NULL;
	}

	return chan;
}

size_t gatt_attr_data_from_string(const char *str, uint8_t **data)
{
	char tmp[3];
	size_t size, i;

	size = strlen(str) / 2;
	*data = g_try_malloc0(size);
	if (*data == NULL)
		return 0;

	tmp[2] = '\0';
	for (i = 0; i < size; i++) {
		memcpy(tmp, str + (i * 2), 2);
		(*data)[i] = (uint8_t) strtol(tmp, NULL, 16);
	}

	return size;
}
