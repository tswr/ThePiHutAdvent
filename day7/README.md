```
bash make.sh
sudo mount -o uid=1000,gid=1000 /dev/sdb1 /mnt/rp2040
cp build/blink.uf2 /mnt/rp2040
sudo sync
sudo umount /mnt/rp2040
```

https://github.com/user-attachments/assets/398ec9ec-7294-4b33-988d-36ae60870b38

