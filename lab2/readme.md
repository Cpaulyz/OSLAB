# OSLAB2

### install

```
make
./main
```

### uninstall

```
make clean
```



## 制作FAT12镜像

1. 在当前目录(.)下创建一个新的软盘镜像a.img

` mkfs.fat -F 12 -C a.img 1440 `

2. 在当前目录下创建一个新目录(./mount)作为挂载点

` mkdir mount `

3. 将镜像./a.img挂载到./mount下

` sudo mount a.img mount`

![image-20201021201413787](C:\Users\admin\AppData\Roaming\Typora\typora-user-images\image-20201021201413787.png)