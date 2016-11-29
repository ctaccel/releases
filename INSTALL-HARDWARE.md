# Hardware Installation

1. Shutdown server, unplug power cable and open server case.

2. Choose a PCIe x8 or x16 slot.

3. If there is insufficient space, the fan attached to the FPGA can be removed.

4. Insert FPGA into PCIe slot.

5. Close the server case, plug in power cable and start server.

6. Login to a terminal and check the FPGA cards can be detected:

    lspci | grep -i xilinx

Should show:

    Memory controller: Xilinx Corporation Device ...

# 硬件安装

1. 关掉服务器，拔掉电源插头然后打开服务器机箱。

2. 选择一个 PCIe x8 或 x16 槽。

3. 如果空间不够，可以拆掉 FPGA 上的散热风扇。

4. 把 FPGA 插入 PCIe 槽。

5. 关起服务器机箱，插上电源插头然后启动服务器。

6. 登入终端然后检查系统是否侦测的到 FPGA：

    lspci | grep -i xilinx

应该能见到：

    Memory controller: Xilinx Corporation Device ...

