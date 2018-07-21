#Build LKM
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# Load LKM
sudo insmod bluesheep_lkm.ko


