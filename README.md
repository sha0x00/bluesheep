# bluesheep
Quick run down of the environment to work with:

Debian 9
Kernel 4.9.0
Bluetooth modules in /net/bluetooth/

Pi Zero + BT mod + GPS mod + PW
L1: HCI event log.
L2: HCI event + GPS coord log.
L3: HCI event + GPS coord log + M/S switch + HCI service poll of attack node."

Level 1 Goals: 

1. Find where connection requests are being handle
    a. Build the Bluetooth module 
    b. Recommend running the same distro & kernel in QEMU.
    c. Debug Kernel modules with gdb or other tracer
2. Log the event with the requesters MAC
3. Reject the HCI connection

# Enable Bluetooth Dongle

Getting Bluetooth running in QEMU is a little tricky. Remember that every Bluetooth dongle, internal/external is considered a USB device.

1. Build Bluez. A lot of times the tools are disabled by default or the distro doesn't provide all of the Bluetooth tools: http://www.bluez.org/download/

When building, make sure you enable the tools:
```
./configure --enable-testing --enable-experimental --enable-deprecated

make

make install (optional)
```

2. Make sure Bluetooth dongle is up and running. Assuming you only have one BT device
```
sudo hciconfigure hci0
```

3. Find the vendor and product ID of your bluetooth device:
```
lsusb
```
My Bluetooth device is a built in module of my laptop:
```
Bus 001 Device 003: ID 8087:07dc Intel Corp.
```
The VendorID is 8087 and ProductID is 07dc.
If you can't determine what your Bluetooth device is, running lsusb in verbose should tell you
```
lsusb -v
```

3. Run QEMU with the following. :
```
sudo qemu-system-x86_64 -s -m 2048 -usb -device usb-host,vendorid=0x8087,productid=0x07dc -bt hci,host:hci0 -enable-kvm -drive if=virtio,file=bluesheep.qcow2,cache=none
```

# Debug Bluetooth 

We're going to be working with the Bluetooth kernel module. Instead of building the whole kernel, we can build just the kernel module.

https://askubuntu.com/questions/515407/how-recipe-to-build-only-one-kernel-module
```
sudo hciconfigure hci0
```

1. Ensure you have the "bluetooth.o" object file in the source under "/net/bluetooth/" in the linux source you used.
2. If you look at one of the opts during the execution of qemu, you'll see the "-s" opt. This is a shorthand for "-gdb tcp::1234", which essentially creates a gdbserver on TCP port 1234
3. Determine where in memory your Bluetooth kernel module is loaded:
```
cat /sys/module/bluetooth/sections/.text
```
If you don't see that your bluetooth module is loaded, make sure you've loaded the module with
```
sudo hciconfigure hci0 up
```  
4. Connect to the gdb server from the host:
```
gdb
target remote :1234
add-symbol-file $linux_src/net/bluetooth/bluetooth.o 0xaddress_loaded_kernel
```
5. Confirm that the symbols are loaded in gdb. Should see a lot of Bluetooth functions:
```
info functions
````

# Meeting Notes 07/08/18:

Hardware Reqs:
1. BlueZ Driver support.
2. Storage
    Log format:
        - Event Timestamp
    	- BT protocol type + version
    	- BT Address
    	- Event GPS coords*
    	- HCI capabilities**
3. Power
	- 16 hours run time, based on radios + cpu drain.
	- sd write frequency optimization.
	- gps poll frequency optimization.

//*  Stretch Goal 1: GPS integration.
//** Stretch Goal 2: HCI role switch + HCI capabilities query against adversary.


TODO:
1. Define supported protocol scope for Defcon 26/BlueSheep v1 (BT Classic/BLE Versions)
2. Finalize hardware BoM by 07/13/18.
3. Identify best practices in IPC from kernel space.
    * BlueZ -> user_land_logger (proc_fs/netlink_soc).
4. Identify options for hyper-performant in memory message queuing.
