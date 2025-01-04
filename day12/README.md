```
bash make.sh
sudo mount -o uid=1000,gid=1000 /dev/sdb1 /mnt/rp2040
cp build/blink.uf2 /mnt/rp2040
sudo sync
sudo umount /mnt/rp2040
```

https://github.com/user-attachments/assets/4ab19b4c-5141-488b-8d5f-678d9a26ed1b


