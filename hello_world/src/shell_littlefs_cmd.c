
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
LOG_MODULE_REGISTER(littlefs_cmd, LOG_LEVEL_INF);

// 定义挂载点
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(littlefs);
static struct fs_mount_t lfs_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &littlefs,
    .storage_dev = (void *)FIXED_PARTITION_ID(littlefs_partition),
    .mnt_point = "/lfs",
};


int cmd_mount(const struct shell *shell, size_t argc, char **argv)
{
    int rc = fs_mount(&lfs_mnt);
    if (rc != 0) {
        shell_error(shell, "Failed to mount filesystem: %d", rc);
        return -ENOEXEC;
    }
    shell_print(shell, "Filesystem mounted at %s", lfs_mnt.mnt_point);
    return 0;
}

int cmd_open_write(const struct shell *shell, size_t argc, char **argv)
{ 
    struct fs_file_t file;
    int rc;

    // 初始化文件结构
    fs_file_t_init(&file);

    char filename[64];
    sprintf(filename, "/lfs/%s", argv[1]);

    // 打开文件（创建或覆盖）
    rc = fs_open(&file, filename, FS_O_CREATE | FS_O_WRITE);
    if (rc != 0) {
        LOG_ERR("Failed to open file: %d", rc);
        return -1;
    }
    char buf[64];
    sprintf(buf, "%s", argv[2]);
    rc = fs_write(&file, buf, strlen(buf));
    if (rc < 0) {
        LOG_ERR("Failed to write file: %d", rc);
        fs_close(&file);
        return -1;
    }
    LOG_INF("Wrote %d bytes", rc);
    fs_close(&file);
}
int cmd_open_read(const struct shell *shell, size_t argc, char **argv)
{ 
    struct fs_file_t file;
    int rc;

    // 初始化文件结构
    fs_file_t_init(&file);

    char filename[64];
    sprintf(filename, "/lfs/%s", argv[1]);

    // 打开文件（创建或覆盖）
    rc = fs_open(&file, filename, FS_O_READ);
    if (rc != 0) {
        LOG_ERR("Failed to open file: %d", rc);
        return -1;
    }
    char read_buf[64];
    rc = fs_read(&file, read_buf, sizeof(read_buf) - 1);
    if (rc < 0) {
        LOG_ERR("Failed to read file: %d", rc);
        fs_close(&file);
        return -1;
    }
    read_buf[rc] = '\0';
    LOG_INF("Read: %s", read_buf);
    fs_close(&file);
}
int cmd_umount(const struct shell *shell, size_t argc, char **argv)
{
    // 检查挂载状态
    if (lfs_mnt.mnt_point == NULL) {
        shell_print(shell, "Filesystem not mounted");
        return 0;
    }
    
    int rc = fs_unmount(&lfs_mnt);
    if (rc != 0) {
        shell_error(shell, "Failed to umount filesystem: %d", rc);
        return -ENOEXEC;
    }
    shell_print(shell, "Filesystem unmounted");
    return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(sub_lfs,
        SHELL_CMD(mount,NULL,"mount littlefs",cmd_mount),
        SHELL_CMD(open_write,NULL,"open file in littlefs",cmd_open_write),
        SHELL_CMD(open_read,NULL,"open file in littlefs",cmd_open_read),
        SHELL_CMD(umount,NULL,"umount littlefs",cmd_umount),
        SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(lfs_cmd,&sub_lfs,"littlefs controller",NULL);
