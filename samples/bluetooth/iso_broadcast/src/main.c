/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

#define BUF_ALLOC_TIMEOUT        (100)               /* milliseconds */
#define BIG_TERMINATE_TIMEOUT_US (60 * USEC_PER_SEC) /* microseconds */
#define BIG_SDU_INTERVAL_US      (10000)

NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, 1, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static K_SEM_DEFINE(sem_big_cmplt, 0, 1);
static K_SEM_DEFINE(sem_big_term, 0, 1);
static K_SEM_DEFINE(sem_iso_data, CONFIG_BT_ISO_TX_BUF_COUNT, CONFIG_BT_ISO_TX_BUF_COUNT);

static uint16_t seq_num;

static void iso_connected(struct bt_iso_chan *chan)
{
	printk("ISO Channel %p connected\n", chan);

	seq_num = 0U;

	k_sem_give(&sem_big_cmplt);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	printk("ISO Channel %p disconnected with reason 0x%02x\n", chan, reason);
	k_sem_give(&sem_big_term);
}

static void iso_sent(struct bt_iso_chan *chan)
{
	k_sem_give(&sem_iso_data);
}

static struct bt_iso_chan_ops iso_ops = {
	.connected = iso_connected,
	.disconnected = iso_disconnected,
	.sent = iso_sent,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = sizeof(uint32_t), /* bytes */
	.rtn = 2,
	.phy = BT_GAP_LE_PHY_1M,
};

static struct bt_iso_chan_qos bis_iso_qos = {
	.tx = &iso_tx_qos,
};

static struct bt_iso_chan bis_iso_chan[] = {
	{
		.ops = &iso_ops,
		.qos = &bis_iso_qos,
	},
};

static struct bt_iso_chan *bis[] = {
	&bis_iso_chan[0],
};

static struct bt_iso_big_create_param big_create_param = {
	.num_bis = 1,
	.bis_channels = bis,
	.interval = BIG_SDU_INTERVAL_US, /* in microseconds */
	.latency = 10,                   /* in milliseconds */
	.packing = 0,                    /* 0 - sequential, 1 - interleaved */
	.framing = 0,                    /* 0 - unframed, 1 - framed */
};

int main(void)
{
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;
	int err;

	uint32_t iso_send_count = 0;
	uint8_t iso_data[sizeof(iso_send_count)] = {0};

	printk("Starting ISO Broadcast Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	/* Create a non-connectable non-scannable advertising set */
	const struct bt_le_adv_param *adv_params = BT_LE_ADV_PARAM(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_NO_2M,
		BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);
	err = bt_le_ext_adv_create(adv_params, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n",
		       err);
		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return 0;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return 0;
	}

	/* Create BIG */
	err = bt_iso_big_create(adv, &big_create_param, &big);
	if (err) {
		printk("Failed to create BIG (err %d)\n", err);
		return 0;
	}

	printk("Waiting for BIG complete...\n");
	err = k_sem_take(&sem_big_cmplt, K_FOREVER);
	if (err) {
		printk("failed (err %d)\n", err);
		return 0;
	}
	printk("BIG create complete chan.\n");

	while (true) {
		struct net_buf *buf;
		int ret;

		buf = net_buf_alloc(&bis_tx_pool, K_MSEC(BUF_ALLOC_TIMEOUT));
		if (!buf) {
			printk("Data buffer allocate timeout on channel\n");
			return 0;
		}

		ret = k_sem_take(&sem_iso_data, K_MSEC(BUF_ALLOC_TIMEOUT));
		if (ret) {
			printk("k_sem_take for ISO data sent failed\n");
			net_buf_unref(buf);
			return 0;
		}

		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
		sys_put_le32(iso_send_count, iso_data);
		net_buf_add_mem(buf, iso_data, sizeof(iso_data));
		ret = bt_iso_chan_send(&bis_iso_chan[0], buf, seq_num);
		if (ret < 0) {
			printk("Unable to broadcast data on channel: %d", ret);
			net_buf_unref(buf);
			return 0;
		}

		if ((iso_send_count % CONFIG_ISO_PRINT_INTERVAL) == 0) {
			printk("Sending value %u\n", iso_send_count);
		}
		iso_send_count++;
		seq_num++;
	}
}
