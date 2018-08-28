# allwinner-H3-iar-project
全智H3芯片iar 裸机开发工程
1.四核CORTEX-A7无系统裸机程序。用于在某些应用中替代DSP。
2.开发环境IAR 仿真器jlink或CMSIS-DAP都可调试
3.img文件修改的uboot，主要作用为使能H3芯片的JTAG口，初始化SDRAM。镜像文件写入TF卡，插入目标板。上电自动加载运行。然后可向普通单片机一样下载调试。.
4.目前存在很多问题，个人精力有限，有兴趣的一起完善。



