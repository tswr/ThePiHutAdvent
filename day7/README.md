```
bash make.sh
sudo mount -o uid=1000,gid=1000 /dev/sdb1 /mnt/rp2040
cp build/blink.uf2 /mnt/rp2040
sudo sync
sudo umount /mnt/rp2040
```

https://github.com/user-attachments/assets/3fdf140e-8528-4da2-a258-a625046980fa

