# Software Installation

1. Install packages:

  - rhel: `sudo rpm -i accel*.rpm`

  - debian: `sudo dpkg -i accel*.deb`

2. FPGA device should appear under /dev, e.g.:

    /dev/xdma_driver0

3. Start/Stop/Check accel-service daemon:

  - rhel: `sudo service acceld [start/stop/status]`

  - debian: `sudo systemctl [start/stop/status] accel-service`

4. Service log should show:

    ```
    cat /var/log/acceld-YYYY-MM-DD.log

    Nov-29 23:53:42 [info] Opened device /dev/xdma_driver0
    Nov-29 23:53:42 [info] Monitoring devices
    Nov-29 23:53:42 [info] Sending to /dev/xdma_driver0
    Nov-29 23:53:42 [info] Getting from /dev/xdma_driver0
    Nov-29 23:53:42 [info] Serving /dev/shm/acceld
    Nov-29 23:53:42 [info] Starting 8 threads
    ... etc
    ```

5. Unintall packages:

  - rhel: `sudo yum remove accel*`

  - debian: `sudo apt-get remove accel*`

# 软件安装

1. 安装软件：

  - rhel: `sudo rpm -i accel*.rpm`

  - debian: `sudo dpkg -i accel*.deb`

2. /dev 下应该可以看到 FPGA, 如：

    /dev/xdma_driver0

3. 启动／停止／检查 后台服务：

  - rhel: `sudo service acceld [start/stop/status]`

  - debian: `sudo systemctl [start/stop/status] accel-service`

4. 后台服务日志应该显示：

    ```
    cat /var/log/acceld-YYYY-MM-DD.log

    Nov-29 23:53:42 [info] Opened device /dev/xdma_driver0
    Nov-29 23:53:42 [info] Monitoring devices
    Nov-29 23:53:42 [info] Sending to /dev/xdma_driver0
    Nov-29 23:53:42 [info] Getting from /dev/xdma_driver0
    Nov-29 23:53:42 [info] Serving /dev/shm/acceld
    Nov-29 23:53:42 [info] Starting 8 threads
    ... etc
    ```

5. 移除软件:

  - rhel: `sudo yum remove accel*`

  - debian: `sudo apt-get remove accel*`

