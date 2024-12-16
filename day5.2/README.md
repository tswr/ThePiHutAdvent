```
bash make.sh
sudo mount -o uid=1000,gid=1000 /dev/sdb1 /mnt/rp2040
cp build/blink.uf2 /mnt/rp2040
sudo sync
sudo umount /mnt/rp2040
```

https://github.com/user-attachments/assets/a10a98e5-d642-4473-ace3-a7cccb0aeb59

