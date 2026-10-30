#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/ring_buffer.h>



extern const struct device *uart;


#define RX_BUF_SIZE 64

RING_BUF_DECLARE(uart_ringbuf, 64); 

static struct k_work uart_rx_work;


static struct k_work_q uart_work_q;
K_THREAD_STACK_DEFINE(uart_work_q_stack, 2048);
#define CDC_ACM_DEV DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0))

static void uart_rx_work_handler(struct k_work *work)
{
    uint8_t buf[32];
    uint32_t len;

    /* 从环形缓冲区中取出数据 */
    while ((len = ring_buf_get(&uart_ringbuf, buf, sizeof(buf))) > 0) {
        printk("Received %d bytes: ", len);
        for (uint32_t i = 0; i < len; i++) {
            printk("%02x ", buf[i]);
        }
        printk("\n");
    }
}

static void uart_callback(const struct device *dev, void *user_data)
{
    uint8_t buf[32];
    int recv_len = 0;

    uart_irq_update(dev);
    if (!uart_irq_rx_ready(dev)) 
        return;


    while (uart_fifo_read(dev, &buf[recv_len], 1) == 1 && recv_len < sizeof(buf)) {
        recv_len++;
    }

    if (recv_len > 0) {
        /* ring_buf_put 是线程和中断安全的 */
        ring_buf_put(&uart_ringbuf, buf, recv_len);
        k_work_submit_to_queue(&uart_work_q, &uart_rx_work);
    }
}
static void cmd_uart_init(void)
{
    if(!device_is_ready(CDC_ACM_DEV)){
        printk("CDC ACM device is not ready\n");
        return;
    }
    usb_enable(NULL);
    k_work_queue_init(&uart_work_q);
    k_work_queue_start(&uart_work_q, uart_work_q_stack,
                   K_THREAD_STACK_SIZEOF(uart_work_q_stack),
                   5, NULL);

    k_work_init(&uart_rx_work, uart_rx_work_handler);
    uart_irq_callback_user_data_set(CDC_ACM_DEV, uart_callback, NULL);
    uart_irq_rx_enable(CDC_ACM_DEV);

}

static int cmd_uart_send(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(sh, "Usage: uart send <string>");
        return -EINVAL;
    }

    
    for (int i = 0; argv[1][i] != '\0'; i++) {
        uart_poll_out(CDC_ACM_DEV, argv[1][i]);
    }
    uart_poll_out(CDC_ACM_DEV, '\r');
    uart_poll_out(CDC_ACM_DEV, '\n');

    shell_print(sh, "Sent: %s", argv[1]);
    return 0;
}



SHELL_STATIC_SUBCMD_SET_CREATE(sub_uart,
        SHELL_CMD(send,NULL,"send data over uart",cmd_uart_send),
        SHELL_CMD(init,NULL,"initialize uart",cmd_uart_init),
        SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(uart,&sub_uart,"uart controller",NULL);
