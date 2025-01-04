```
bash make.sh
sudo mount -o uid=1000,gid=1000 /dev/sdb1 /mnt/rp2040
cp build/blink.uf2 /mnt/rp2040
sudo sync
sudo umount /mnt/rp2040
picocom /dev/ttyACM0 -b 115200
```

https://github.com/user-attachments/assets/84b88584-d28d-4acf-81f6-04424bda084d

